#include "tree-ui-system.hpp"

#include <neon/reflection/field-text.hpp>

#include "ui-properties.hpp"

// Describing a kind of element through reflection: what an inspector
// shows, and what a binding for scripts is made from.

namespace neon
{
  namespace
  {
    /// What the kinds of the engine are for, in a sentence each.
    std::string DescriptionOf(const std::string &type)
    {
      if (type == "panel") { return "A box that holds other elements"; }
      if (type == "label") { return "A text"; }
      if (type == "image") { return "A picture"; }
      if (type == "button") { return "What a player chooses with the pointer, the keys, or a controller"; }
      if (type == "bar") { return "A bar that is filled to a fraction, such as health"; }
      if (type == "input") { return "A text of one line that is typed into"; }
      if (type == "textarea") { return "A text of several lines that is typed into"; }
      if (type == "checkbox") { return "A box that is ticked or not"; }
      if (type == "toggle") { return "A switch that is on or off"; }
      if (type == "radio") { return "One of several choices, of which one is chosen at a time"; }
      if (type == "slider") { return "A number between two others, chosen by moving a knob"; }
      if (type == "select") { return "One of several choices, chosen from a list that opens"; }
      return "";
    }

    FieldKind KindOf(const UiValueKind kind)
    {
      switch (kind)
      {
        case UiValueKind::Whole: return FieldKind::Whole;
        case UiValueKind::Length: return FieldKind::Length;
        case UiValueKind::Color: return FieldKind::Color;
        case UiValueKind::Keyword: return FieldKind::Choice;
        case UiValueKind::Text: return FieldKind::Text;
        case UiValueKind::Edges: return FieldKind::NumberList;
        case UiValueKind::Other: return FieldKind::Text;
        default: return FieldKind::Number;
      }
    }

    /// The value of a property of a style, as a field holds it.
    FieldValue ValueOf(const UiProperty &property, const UiStyle &style)
    {
      const UiPropertyValue value = property.get(style);

      switch (property.kind)
      {
        case UiValueKind::Whole:
          return static_cast<int>(value.number);
        case UiValueKind::Length:
        {
          FieldLength length;
          length.is_auto = value.length.unit == LayoutLength::Unit::Auto;
          length.pixels = value.length.unit == LayoutLength::Unit::Percent ? 0.0f : value.length.value;
          length.percent = value.length.unit == LayoutLength::Unit::Percent
            ? value.length.value
            : value.length.unit == LayoutLength::Unit::Sum ? value.length.percent : 0.0f;
          return length;
        }
        case UiValueKind::Color:
          return value.color;
        case UiValueKind::Keyword:
          return value.keyword >= 0 && static_cast<std::size_t>(value.keyword) < property.keywords.size()
            ? property.keywords[static_cast<std::size_t>(value.keyword)]
            : std::string();
        case UiValueKind::Text:
        case UiValueKind::Other:
          return property.Format(value);
        case UiValueKind::Edges:
          return std::vector<float>{value.edges[0], value.edges[1], value.edges[2], value.edges[3]};
        default:
          return value.number;
      }
    }

    /// A field that reads and changes a field of an element.
    FieldInfo FieldOf(const UiElement::Field &field)
    {
      FieldInfo info;
      info.name = field.name;
      info.kind = field.kind;
      info.description = field.description;
      info.choices = field.choices;

      const std::string name = field.name;

      info.get = [name](const void *object) -> FieldValue
      {
        FieldValue value;
        (void) static_cast<const UiElement *>(object)->GetField(name, value);
        return value;
      };

      info.set = [name](void *object, const FieldValue &value)
      {
        std::string error;
        (void) static_cast<UiElement *>(object)->SetField(name, value, error);
      };

      return info;
    }

    /// A field that every element has, which the user interface keeps.
    FieldInfo CommonField(
      const std::string &name,
      const FieldKind kind,
      const std::string &description,
      std::function<FieldValue(const UiElement &)> get,
      std::function<void(UiElement &, const FieldValue &)> set)
    {
      FieldInfo info;
      info.name = name;
      info.kind = kind;
      info.description = description;
      info.get = [get](const void *object) { return get(*static_cast<const UiElement *>(object)); };
      info.set = [set](void *object, const FieldValue &value) { set(*static_cast<UiElement *>(object), value); };
      return info;
    }

    /// A field that reads a property of the style as it came to be, and
    /// sets it for the element as a script does.
    FieldInfo StyleField(const UiProperty &property)
    {
      FieldInfo info;
      info.name = property.yaml_name;
      info.kind = KindOf(property.kind);
      info.description = property.description;
      info.choices = property.keywords;

      const std::string css_name = property.name;

      info.get = [css_name](const void *object) -> FieldValue
      {
        const UiProperty *known = UiProperties::Get().Find(css_name);
        return known != nullptr ? ValueOf(*known, static_cast<const UiElement *>(object)->GetStyle()) : FieldValue{};
      };

      info.set = [css_name](void *object, const FieldValue &value)
      {
        const UiProperty *known = UiProperties::Get().Find(css_name);
        if (known == nullptr) { return; }

        // as it is written in CSS, which is what SetProperty() reads
        static_cast<UiElement *>(object)->SetProperty(css_name, FormatField(value));
      };

      return info;
    }
  }

  bool Tree_UiSystem::GetStyleField(const UiElement &element, const std::string &path, FieldValue &value)
  {
    if (!path.starts_with("style.")) { return false; }

    const UiProperty *known = UiProperties::Get().Find(path.substr(6));
    if (known == nullptr || known->IsShorthand()) { return false; }

    value = ValueOf(*known, element.GetStyle());
    return true;
  }

  bool Tree_UiSystem::SetStyleField(UiElement &element, const std::string &path, const FieldValue &value)
  {
    if (!path.starts_with("style.")) { return false; }

    const UiProperty *known = UiProperties::Get().Find(path.substr(6));
    if (known == nullptr) { return false; }

    element.SetProperty(known->name, FormatField(value));
    return true;
  }

  bool Tree_UiSystem::DescribeElement(const std::string &type, TypeInfo &description) const
  {
    const std::unique_ptr<UiElement> element = _types.CreateElement(type);
    if (element == nullptr) { return false; }

    description = {};
    description.name = type;
    description.description = DescriptionOf(type);

    // what every element has
    description.fields.push_back(CommonField(
      "name", FieldKind::Text, "What the element is found by",
      [](const UiElement &each) { return each.GetName(); },
      [](UiElement &each, const FieldValue &) { (void) each; }));

    description.fields.push_back(CommonField(
      "class", FieldKind::TextList, "The classes a style sheet asks for",
      [](const UiElement &each) { return each.GetClasses(); },
      [](UiElement &each, const FieldValue &value)
      {
        if (const auto *classes = std::get_if<std::vector<std::string>>(&value)) { each.SetClasses(*classes); }
      }));

    description.fields.push_back(CommonField(
      "title", FieldKind::Text, "What is shown in a tooltip while the pointer rests on it",
      [](const UiElement &each) { return each.GetTitle(); },
      [](UiElement &each, const FieldValue &value)
      {
        if (const auto *text = std::get_if<std::string>(&value)) { each.SetTitle(*text); }
      }));

    description.fields.push_back(CommonField(
      "tab_index", FieldKind::Whole, "Where it comes in the order of the tab key: 0 as the file has it, -1 for never",
      [](const UiElement &each) { return each.GetTabIndex(); },
      [](UiElement &each, const FieldValue &value)
      {
        if (const auto *whole = std::get_if<int>(&value)) { each.SetTabIndex(*whole); }
      }));

    description.fields.push_back(CommonField(
      "hidden", FieldKind::Bool, "Whether it is left out, as if it were not there",
      [](const UiElement &each) { return each.IsHidden(); },
      [](UiElement &each, const FieldValue &value)
      {
        if (const auto *flag = std::get_if<bool>(&value)) { each.SetHidden(UiFlag(*flag)); }
      }));

    // what the kind has
    for (const auto &field : element->GetFields()) { description.fields.push_back(FieldOf(field)); }

    // the style, as one group with every property of the table
    FieldInfo style;
    style.name = "style";
    style.kind = FieldKind::Group;
    style.description = "The properties of CSS as they came to be for the element";

    for (const auto &property : UiProperties::Get().GetAll())
    {
      if (property.IsShorthand()) { continue; }
      style.fields.push_back(StyleField(property));
    }

    description.fields.push_back(style);
    return true;
  }
} // neon
