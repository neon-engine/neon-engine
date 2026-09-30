#include "tree-ui-system.hpp"

#include <algorithm>
#include <format>

#include "ui-declarations.hpp"
#include "ui-properties.hpp"

// Elements as a game and a script reach them: by handles, which name an
// element and never point at one.

namespace neon
{
  namespace
  {
    bool IsInside(const UiElement *element, const UiElement *other)
    {
      for (const UiElement *above = element->GetParent(); above != nullptr; above = above->GetParent())
      {
        if (above == other) { return true; }
      }
      return false;
    }

    /// Puts what the fields hold in the place of every `${name}` of a
    /// description, in every text of it.
    DataValue Filled(const DataValue &description, const UiRow &fields)
    {
      if (std::string text; description.GetText(text))
      {
        std::string filled;

        for (std::size_t i = 0; i < text.size();)
        {
          const std::size_t close = text[i] == '$' && i + 1 < text.size() && text[i + 1] == '{'
            ? text.find('}', i + 2)
            : std::string::npos;

          if (close == std::string::npos)
          {
            filled += text[i];
            i++;
            continue;
          }

          const std::string name = text.substr(i + 2, close - i - 2);

          if (const auto found = fields.find(name); found != fields.end())
          {
            filled += found->second;
          } else
          {
            // what has no field is shown as it is written, so that a name
            // that was misspelled is seen
            filled += text.substr(i, close - i + 1);
          }

          i = close + 1;
        }

        DataValue value = DataValue::Text(filled);
        value.SetLine(description.GetLine());
        return value;
      }

      if (description.IsList())
      {
        DataValue list = DataValue::List();
        list.SetLine(description.GetLine());
        for (const auto &item : description.GetItems()) { list.Add(Filled(item, fields)); }
        return list;
      }

      if (description.IsMap())
      {
        DataValue map = DataValue::Map();
        map.SetLine(description.GetLine());
        for (const auto &[name, value] : description.GetEntries()) { map.Set(name, Filled(value, fields)); }
        return map;
      }

      return description;
    }
  }

  UiHandle Tree_UiSystem::GetRoot(const int document) const
  {
    if (_documents.empty()) { return {}; }

    if (document < 0) { return HandleOf(_documents.back()->root.get()); }

    for (const auto &each : _documents)
    {
      if (each->id == document) { return HandleOf(each->root.get()); }
    }
    return {};
  }

  UiHandle Tree_UiSystem::FindByName(const std::string &name, const UiHandle from) const
  {
    if (name.empty()) { return {}; }

    if (from.IsSet())
    {
      UiElement *start = ElementOf(from);
      if (start == nullptr) { return {}; }

      for (const auto &child : start->GetChildren())
      {
        if (const UiElement *found = FindByName(*child, name); found != nullptr) { return HandleOf(found); }
      }
      return {};
    }

    return HandleOf(Find(name));
  }

  std::vector<UiHandle> Tree_UiSystem::FindByClass(const std::string &name, const UiHandle from) const
  {
    std::vector<UiHandle> found;

    for (const UiHandle handle : Query("*", from))
    {
      if (const UiElement *element = ElementOf(handle); element != nullptr && element->HasClass(name))
      {
        found.push_back(handle);
      }
    }

    return found;
  }

  std::vector<UiHandle> Tree_UiSystem::FindByType(const std::string &type, const UiHandle from) const
  {
    std::vector<UiHandle> found;

    for (const UiHandle handle : Query("*", from))
    {
      if (const UiElement *element = ElementOf(handle); element != nullptr && element->GetType() == type)
      {
        found.push_back(handle);
      }
    }

    return found;
  }

  std::vector<UiHandle> Tree_UiSystem::Query(const std::string &selector, const UiHandle from) const
  {
    std::vector<UiHandle> found;

    std::vector<CssComplexSelector> selectors;
    if (std::string error; !ParseCssSelectors(selector, selectors, error))
    {
      _logger->Warn("The selector '{}' cannot be read: {}", selector, error);
      return found;
    }

    const UiElement *scope = nullptr;
    if (from.IsSet())
    {
      scope = ElementOf(from);
      if (scope == nullptr) { return found; }
    }

    const auto matches = [&](const UiElement &element)
    {
      return std::ranges::any_of(selectors, [&](const CssComplexSelector &each)
      {
        // a part of an element is no element
        return each.pseudo_element.empty() && MatchesCss(each, element, scope);
      });
    };

    if (scope != nullptr)
    {
      std::vector<UiElement *> elements;
      CollectUiElements(*const_cast<UiElement *>(scope), elements);

      for (const UiElement *element : elements)
      {
        // what is inside, and not the element itself
        if (element != scope && matches(*element)) { found.push_back(HandleOf(element)); }
      }
      return found;
    }

    for (std::size_t i = _documents.size(); i > 0; i--)
    {
      std::vector<UiElement *> elements;
      CollectUiElements(*_documents[i - 1]->root, elements);

      for (const UiElement *element : elements)
      {
        if (matches(*element)) { found.push_back(HandleOf(element)); }
      }
    }

    return found;
  }

  UiHandle Tree_UiSystem::QueryFirst(const std::string &selector, const UiHandle from) const
  {
    const auto found = Query(selector, from);
    return found.empty() ? UiHandle{} : found.front();
  }

  bool Tree_UiSystem::Matches(const UiHandle element, const std::string &selector) const
  {
    const UiElement *found = ElementOf(element);
    if (found == nullptr) { return false; }

    std::vector<CssComplexSelector> selectors;
    if (std::string error; !ParseCssSelectors(selector, selectors, error))
    {
      _logger->Warn("The selector '{}' cannot be read: {}", selector, error);
      return false;
    }

    return std::ranges::any_of(selectors, [&](const CssComplexSelector &each)
    {
      return each.pseudo_element.empty() && MatchesCss(each, *found);
    });
  }

  UiHandle Tree_UiSystem::GetParent(const UiHandle element) const
  {
    const UiElement *found = ElementOf(element);
    return found != nullptr ? HandleOf(found->GetParent()) : UiHandle{};
  }

  std::vector<UiHandle> Tree_UiSystem::GetChildren(const UiHandle element) const
  {
    std::vector<UiHandle> children;

    if (const UiElement *found = ElementOf(element); found != nullptr)
    {
      for (const auto &child : found->GetChildren()) { children.push_back(HandleOf(child.get())); }
    }

    return children;
  }

  UiHandle Tree_UiSystem::GetNextSibling(const UiHandle element) const
  {
    const UiElement *found = ElementOf(element);
    return found != nullptr ? HandleOf(found->GetNextSibling()) : UiHandle{};
  }

  UiHandle Tree_UiSystem::GetPreviousSibling(const UiHandle element) const
  {
    const UiElement *found = ElementOf(element);
    return found != nullptr ? HandleOf(found->GetPreviousSibling()) : UiHandle{};
  }

  std::string Tree_UiSystem::GetElementType(const UiHandle element) const
  {
    const UiElement *found = ElementOf(element);
    return found != nullptr ? found->GetType() : "";
  }

  std::string Tree_UiSystem::GetElementName(const UiHandle element) const
  {
    const UiElement *found = ElementOf(element);
    return found != nullptr ? found->GetName() : "";
  }

  bool Tree_UiSystem::Set(const UiHandle element, const std::string &property, const std::string &value)
  {
    UiElement *found = ElementOf(element);
    if (found == nullptr) { return false; }

    // a custom property holds whatever it is given, and taking something
    // back needs no look at it
    if (!property.starts_with("--") && !value.empty())
    {
      std::vector<std::string> problems;
      if (!CheckUiDeclaration(property, value, found->Describe(), problems))
      {
        for (const auto &problem : problems) { _logger->Warn("{}. Nothing is set", problem); }
        return false;
      }
    }

    found->SetProperty(property, value);

    // what is inside takes a custom property from the element
    if (property.starts_with("--")) { found->InvalidateAll(UiDirty::Style); }

    return true;
  }

  std::string Tree_UiSystem::GetComputed(const UiHandle element, const std::string &property) const
  {
    const UiElement *found = ElementOf(element);
    if (found == nullptr) { return ""; }

    const UiStyle &style = found->GetStyle();

    if (property.starts_with("--"))
    {
      const auto &variables = found->GetVariables();
      if (variables == nullptr) { return ""; }

      const auto held = variables->find(property);
      return held != variables->end() ? held->second : "";
    }

    const UiProperties &properties = UiProperties::Get();
    const UiProperty *known = properties.Find(property);
    if (known == nullptr) { return ""; }

    if (!known->IsShorthand()) { return known->Format(known->get(style)); }

    // a shorthand is what it stands for, one behind the other
    std::string written;
    for (const UiProperty *each : properties.GetLonghands(*known))
    {
      if (!written.empty()) { written += ' '; }
      written += each->Format(each->get(style));
    }
    return written;
  }

  bool Tree_UiSystem::AddClass(const UiHandle element, const std::string &name)
  {
    UiElement *found = ElementOf(element);
    return found != nullptr && found->AddClass(name);
  }

  bool Tree_UiSystem::RemoveClass(const UiHandle element, const std::string &name)
  {
    UiElement *found = ElementOf(element);
    return found != nullptr && found->RemoveClass(name);
  }

  bool Tree_UiSystem::ToggleClass(const UiHandle element, const std::string &name)
  {
    UiElement *found = ElementOf(element);
    if (found == nullptr) { return false; }

    if (found->HasClass(name))
    {
      found->RemoveClass(name);
      return false;
    }

    return found->AddClass(name);
  }

  bool Tree_UiSystem::HasClass(const UiHandle element, const std::string &name) const
  {
    const UiElement *found = ElementOf(element);
    return found != nullptr && found->HasClass(name);
  }

  std::vector<std::string> Tree_UiSystem::GetClasses(const UiHandle element) const
  {
    const UiElement *found = ElementOf(element);
    return found != nullptr ? found->GetClasses() : std::vector<std::string>{};
  }

  bool Tree_UiSystem::SetField(const UiHandle element, const std::string &name, const FieldValue &value)
  {
    UiElement *found = ElementOf(element);
    if (found == nullptr) { return false; }

    // what every element has
    if (name == "title")
    {
      const auto *text = std::get_if<std::string>(&value);
      if (text == nullptr) { return false; }

      found->SetTitle(*text);
      return true;
    }

    if (name == "tab_index")
    {
      const auto *whole = std::get_if<int>(&value);
      if (whole == nullptr) { return false; }

      found->SetTabIndex(*whole);
      return true;
    }

    if (name == "hidden")
    {
      const auto *flag = std::get_if<bool>(&value);
      return flag != nullptr && SetVisible(element, !*flag);
    }

    if (name.starts_with("style."))
    {
      if (!SetStyleField(*found, name, value))
      {
        const std::string described = found->Describe();
        _logger->Warn("'{}' of {} is no property that is known. Nothing is set", name, described);
        return false;
      }
      return true;
    }

    if (std::string error; !found->SetField(name, value, error))
    {
      _logger->Warn("{}. Nothing is set", error);
      return false;
    }

    // a style sheet may ask for the field, and the element may be
    // something else to the engine by now: one that cannot be used
    ClassesChanged(*found);
    found->Invalidate(UiDirty::Style | UiDirty::Paint);

    if (UiDocument *document = DocumentOf(found); document != nullptr) { document->has_followed = false; }
    return true;
  }

  bool Tree_UiSystem::GetField(const UiHandle element, const std::string &name, FieldValue &value) const
  {
    const UiElement *found = ElementOf(element);
    if (found == nullptr) { return false; }

    if (name == "title")
    {
      value = found->GetTitle();
      return true;
    }

    if (name == "tab_index")
    {
      value = found->GetTabIndex();
      return true;
    }

    if (name == "hidden")
    {
      value = found->IsHidden();
      return true;
    }

    if (name.starts_with("style.")) { return GetStyleField(*found, name, value); }

    return found->GetField(name, value);
  }

  bool Tree_UiSystem::SetElementText(const UiHandle element, const std::string &text)
  {
    return SetField(element, "text", FieldValue{text});
  }

  std::string Tree_UiSystem::GetElementText(const UiHandle element) const
  {
    FieldValue value;
    if (!GetField(element, "text", value)) { return ""; }

    const auto *text = std::get_if<std::string>(&value);
    return text != nullptr ? *text : "";
  }

  bool Tree_UiSystem::SetVisible(const UiHandle element, const bool visible)
  {
    UiElement *found = ElementOf(element);
    if (found == nullptr) { return false; }

    found->SetHidden(UiFlag(!visible));

    if (UiDocument *document = DocumentOf(found); document != nullptr) { document->has_followed = false; }
    return true;
  }

  bool Tree_UiSystem::IsVisible(const UiHandle element) const
  {
    const UiElement *found = ElementOf(element);
    return found != nullptr && !found->IsHidden() && found->GetStyle().visibility == UiVisibility::Visible;
  }

  bool Tree_UiSystem::FocusElement(const UiHandle element)
  {
    UiElement *found = ElementOf(element);
    if (found == nullptr || !found->IsFocusable()) { return false; }

    SetFocus(found, true);
    return true;
  }

  UiHandle Tree_UiSystem::GetFocusedElement() const
  {
    return HandleOf(_focused);
  }

  void Tree_UiSystem::Blur()
  {
    SetFocus(nullptr, false);
  }

  bool Tree_UiSystem::GetScroll(const UiHandle element, float &x, float &y) const
  {
    const UiElement *found = ElementOf(element);
    if (found == nullptr) { return false; }

    x = found->GetScrollX();
    y = found->GetScrollY();
    return true;
  }

  bool Tree_UiSystem::SetScroll(const UiHandle element, const float x, const float y)
  {
    UiElement *found = ElementOf(element);
    if (found == nullptr) { return false; }

    // what is inside has to be placed before it is known how far there is
    // to scroll
    Settle();

    ScrollTo(*found, x, y, false);
    return true;
  }

  bool Tree_UiSystem::ScrollIntoView(const UiHandle element)
  {
    UiElement *found = ElementOf(element);
    if (found == nullptr) { return false; }

    Settle();
    ScrollToShow(*found);
    return true;
  }

  bool Tree_UiSystem::GetBox(const UiHandle element, UiBox &box) const
  {
    const UiElement *found = ElementOf(element);
    if (found == nullptr) { return false; }

    const UiRectangle &placed = found->GetBox();
    box.left = placed.left;
    box.top = placed.top;
    box.width = placed.Width();
    box.height = placed.Height();
    box.scale = ScaleOf(found);
    return true;
  }

  UiHandle Tree_UiSystem::PutInto(
    const UiHandle parent,
    const int index,
    const std::function<std::unique_ptr<UiElement>(const std::string &where, std::vector<std::string> &errors)> &make)
  {
    UiElement *into = ElementOf(parent);
    if (into == nullptr)
    {
      _logger->Warn("An element cannot be made inside of one that is gone");
      return {};
    }

    const UiDocument *document = DocumentOf(into);
    const std::string where = document != nullptr ? document->path : "the user interface";

    if (!into->TakesChildren())
    {
      const std::string what = into->Describe();
      _logger->Warn("{}: {} takes nothing inside, and no element is made", where, what);
      return {};
    }

    std::vector<std::string> errors;
    auto element = make(where, errors);

    if (element == nullptr)
    {
      for (const auto &error : errors) { _logger->Error("{}", error); }
      _logger->Error("{}: what describes the element is wrong, and no element is made", where);
      return {};
    }

    const std::size_t place = index < 0 ? into->GetChildren().size() : static_cast<std::size_t>(index);

    const UiElement &added = into->InsertChild(
      std::move(element), place, _layout, document != nullptr ? &document->frame : nullptr);

    return HandleOf(&added);
  }

  UiHandle Tree_UiSystem::CreateFrom(const DataValue &description, const UiHandle parent, const int index)
  {
    return PutInto(parent, index, [&](const std::string &where, std::vector<std::string> &errors)
    {
      return _file.CreateElement(description, where, errors);
    });
  }

  UiHandle Tree_UiSystem::Create(const std::string &description, const UiHandle parent, const int index)
  {
    return PutInto(parent, index, [&](const std::string &where, std::vector<std::string> &errors)
    {
      return _file.CreateElementFromText(description, where, errors);
    });
  }

  UiHandle Tree_UiSystem::CreateFromTemplate(
    const std::string &name,
    const UiHandle parent,
    const UiRow &fields,
    const int index)
  {
    const UiElement *into = ElementOf(parent);
    const UiDocument *document = DocumentOf(into);

    if (document == nullptr)
    {
      _logger->Warn("A copy of the template '{}' cannot be made inside of an element that is gone", name);
      return {};
    }

    const DataValue *description = document->FindTemplate(name);
    if (description == nullptr)
    {
      const std::string path = document->path;
      _logger->Warn("{}: there is no template '{}', and no element is made", path, name);
      return {};
    }

    return CreateFrom(Filled(*description, fields), parent, index);
  }

  bool Tree_UiSystem::Remove(const UiHandle element)
  {
    UiElement *found = ElementOf(element);
    if (found == nullptr) { return false; }

    UiElement *parent = found->GetParent();
    if (parent == nullptr)
    {
      _logger->Warn("The element at the top of a file cannot be removed. Unload() stops showing the file");
      return false;
    }

    // it is destroyed with everything inside it where this ends
    const auto removed = parent->RemoveChild(*found, _layout);
    return removed != nullptr;
  }

  bool Tree_UiSystem::Move(const UiHandle element, const UiHandle parent, const int index)
  {
    UiElement *found = ElementOf(element);
    UiElement *into = ElementOf(parent);

    if (found == nullptr || into == nullptr || found->GetParent() == nullptr) { return false; }

    if (found == into || IsInside(into, found))
    {
      _logger->Warn("An element cannot be moved into itself");
      return false;
    }

    const UiDocument *document = DocumentOf(into);
    if (document != DocumentOf(found))
    {
      _logger->Warn("An element cannot be moved into another file");
      return false;
    }

    if (!into->TakesChildren())
    {
      const std::string what = into->Describe();
      _logger->Warn("{} takes nothing inside, and nothing is moved", what);
      return false;
    }

    const bool had_focus = _focused == found || (_focused != nullptr && IsInside(_focused, found));
    UiElement *focused = _focused;

    auto moved = found->GetParent()->RemoveChild(*found, _layout);
    if (moved == nullptr) { return false; }

    const std::size_t place = index < 0 ? into->GetChildren().size() : static_cast<std::size_t>(index);
    into->InsertChild(std::move(moved), place, _layout, document != nullptr ? &document->frame : nullptr);

    // what is moved is the element it was, and keeps the focus it had
    if (had_focus) { _focused = focused; }
    return true;
  }

  void Tree_UiSystem::SetList(const std::string &name, const std::vector<UiRow> &rows)
  {
    List &list = _lists[name];
    if (list.revision != 0 && list.rows == rows) { return; }

    list.rows = rows;
    list.revision = ++_list_revision;
  }

  std::vector<std::string> Tree_UiSystem::GetElementTypeNames() const
  {
    return _types.GetNames();
  }
} // neon
