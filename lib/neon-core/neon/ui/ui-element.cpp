#include "ui-element.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

#include "ui-cascade.hpp"
#include "ui-properties.hpp"
#include "ui-scrollbars.hpp"

#include "ui-box-paint.hpp"
#include "ui-image-paint.hpp"

namespace neon
{
  // Helpers of UiTextMeasure, for this file alone.
  namespace
  {
    Color Faded(const Color &color, const float opacity)
    {
      return {color.r, color.g, color.b, color.a * opacity};
    }

    // what cuts nothing off
    constexpr float far_away = 1.0e9f;

    UiRectangle Everything()
    {
      return {-far_away, -far_away, far_away, far_away};
    }

    UiRectangle Intersected(const UiRectangle &a, const UiRectangle &b)
    {
      return {
        std::max(a.left, b.left),
        std::max(a.top, b.top),
        std::min(a.right, b.right),
        std::min(a.bottom, b.bottom)
      };
    }

    /// A length with pixels added to it.
    LayoutLength Added(const LayoutLength &length, const float pixels)
    {
      switch (length.unit)
      {
        case LayoutLength::Unit::Pixels: return LayoutLength::Pixels(length.value + pixels);
        case LayoutLength::Unit::Percent: return LayoutLength::Sum(pixels, length.value);
        case LayoutLength::Unit::Sum: return LayoutLength::Sum(length.value + pixels, length.percent);
        default: return LayoutLength::Pixels(pixels);
      }
    }

    bool IsRow(const UiStyle &style)
    {
      return style.layout.flex_direction == FlexDirection::Row ||
             style.layout.flex_direction == FlexDirection::RowReverse;
    }
  }

  void CollectUiElements(UiElement &element, std::vector<UiElement *> &elements)
  {
    elements.push_back(&element);
    for (const auto &child : element.GetChildren()) { CollectUiElements(*child, elements); }
  }

  UiRectangle ToPixels(const UiRectangle &rectangle, const float scale)
  {
    return {
      std::round(rectangle.left * scale),
      std::round(rectangle.top * scale),
      std::round(rectangle.right * scale),
      std::round(rectangle.bottom * scale)
    };
  }

  std::size_t UiElement::StyleIndex(const UiStates &states)
  {
    if (states.disabled) { return kStyle_Count - 1; }

    return (states.focus ? 1 : 0) + (states.hover ? 2 : 0) + (states.active ? 4 : 0);
  }

  void UiElement::ApplyDefaults(UiStyle &style) const {}

  void UiElement::ApplyStateDefaults(UiStyle &style, const UiStates &states) const {}

  void UiElement::ReadAttributes(const DataReader &reader) {}

  bool UiElement::TakesChildren() const
  {
    return false;
  }

  bool UiElement::IsFocusable() const
  {
    return false;
  }

  bool UiElement::IsClickable() const
  {
    return false;
  }

  bool UiElement::ClosesItsFile() const
  {
    return false;
  }

  bool UiElement::WantsFocus() const
  {
    return false;
  }

  bool UiElement::IsEnabled() const
  {
    return true;
  }

  bool UiElement::HasContent() const
  {
    return false;
  }

  LayoutSize UiElement::Measure(const UiFrame &frame, float available_width, float available_height)
  {
    return {};
  }

  void UiElement::Update(const UiFrame &frame) {}

  void UiElement::PaintContent(
    UiPainter &painter,
    const UiFrame &frame,
    const UiRectangle &content_box,
    float opacity) {}

  void UiElement::ApplyPartDefaults(const std::string &part, UiStyle &style) const
  {
    // The parts of a scrollbar take their colours from `scrollbar_color`,
    // and from the colour of the text without one.
    const UiStyle &of_element = GetStyle();
    const Color &text = of_element.color;

    if (part == "tooltip")
    {
      // what says what an element is: small, dark, and on top
      style.background_color = {0.1f, 0.11f, 0.14f, 0.95f};
      style.color = {1.0f, 1.0f, 1.0f, 1.0f};
      style.font_size = 14.0f;
      style.font_weight = 400;
      style.layout.border = {1.0f, 1.0f, 1.0f, 1.0f};
      style.border_color = Color{1.0f, 1.0f, 1.0f, 0.2f};
    } else if (part == "scrollbar-thumb")
    {
      style.background_color = of_element.scrollbar_thumb_color.value_or(Color{text.r, text.g, text.b, text.a * 0.5f});
    } else if (part == "scrollbar-track")
    {
      style.background_color = of_element.scrollbar_track_color.value_or(Color{text.r, text.g, text.b, text.a * 0.12f});
    }
  }

  void UiElement::Interact(UiInteraction &interaction, const UiFrame &frame) {}

  bool UiElement::TakesText() const
  {
    return false;
  }

  bool UiElement::UsesDirection(Key direction) const
  {
    return false;
  }

  bool UiElement::IsChecked() const
  {
    return false;
  }

  bool UiElement::IsInvalid() const
  {
    return false;
  }

  bool UiElement::GetCaretBox(const UiFrame &frame, UiRectangle &box) const
  {
    return false;
  }

  bool UiElement::GetContentSize(float &width, float &height) const
  {
    return false;
  }

  bool UiElement::HasTopLayer() const
  {
    return false;
  }

  bool UiElement::TopLayerContains(const UiFrame &frame, const float x, const float y) const
  {
    (void) frame;
    (void) x;
    (void) y;
    return false;
  }

  void UiElement::PaintTopLayer(UiPainter &painter, const UiFrame &frame)
  {
    (void) painter;
    (void) frame;
  }

  bool UiElement::PaintsEveryFrame() const
  {
    return false;
  }

  bool UiElement::TellsWhenItChanged() const
  {
    return false;
  }

  bool UiElement::GetField(const std::string &name, FieldValue &value) const
  {
    return false;
  }

  bool UiElement::SetField(const std::string &name, const FieldValue &value, std::string &error)
  {
    error = "'" + name + "' is not a field of " + Describe();
    return false;
  }

  std::vector<UiElement::Field> UiElement::GetFields() const
  {
    return {};
  }

  bool UiElement::Tick(const UiFrame &frame, float seconds)
  {
    return false;
  }

  const std::string &UiElement::GetType() const
  {
    return _type;
  }

  const std::string &UiElement::GetName() const
  {
    return _name;
  }

  std::size_t UiElement::GetLine() const
  {
    return _line;
  }

  void UiElement::SetIdentity(const std::string &type, const std::string &name, const std::size_t line)
  {
    _type = type;
    _name = name;
    _line = line;
  }

  std::string UiElement::Describe() const
  {
    return _name.empty() ? "a " + _type : _type + " '" + _name + "'";
  }

  void UiElement::SetStyle(const std::size_t index, const UiStyle &style)
  {
    if (index >= kStyle_Count) { return; }

    if (_styles == nullptr) { _styles = std::make_unique<std::array<UiStyle, kStyle_Count>>(); }

    (*_styles)[index] = style;
    _has_style[index] = true;

    Invalidate(UiDirty::Layout | UiDirty::Paint);
  }

  const UiStyle &UiElement::GetStyle() const
  {
    if (const std::size_t index = StyleIndex(_states); _has_style[index]) { return (*_styles)[index]; }

    // Worked out here, when it is asked for, so that whatever asks finds
    // the style of the state the element is in. While it is worked out,
    // what asks for it finds the style it had.
    if (_style_is_dirty && !_is_computing)
    {
      auto *self = const_cast<UiElement *>(this);
      self->_is_computing = true;

      if (_host != nullptr)
      {
        _host->ComputeStyle(*self);
      } else
      {
        UiCascade::Compute(*self, {});
      }

      self->_is_computing = false;
      self->_style_is_dirty = false;
    }

    return _used;
  }

  const UiStyle &UiElement::GetComputedStyle() const
  {
    (void) GetStyle();
    return _computed;
  }

  bool UiElement::HasComputedStyle() const
  {
    return _has_computed_style;
  }

  const UiStyle &UiElement::GetPartStyle(const std::string &part) const
  {
    const UiStyle &style = GetStyle();

    if (const auto found = _part_styles.find(part); found != _part_styles.end()) { return found->second; }

    // a part takes what is inherited from the element it is a part of
    UiStyle of_part;
    for (const auto &property : UiProperties::Get().GetAll())
    {
      if (property.is_inherited) { property.Copy(style, of_part); }
    }

    ApplyPartDefaults(part, of_part);

    if (_host != nullptr) { _host->ComputePartStyle(*this, part, of_part); }

    return _part_styles[part] = of_part;
  }

  void UiElement::SetHost(UiElementHost *host)
  {
    _host = host;
    for (const auto &child : _children) { child->SetHost(host); }
  }

  UiElementHost *UiElement::GetHost() const
  {
    return _host;
  }

  std::uint64_t UiElement::GetId() const
  {
    return _id;
  }

  void UiElement::SetId(const std::uint64_t id)
  {
    _id = id;
  }

  const std::vector<std::string> &UiElement::GetClasses() const
  {
    return _classes;
  }

  void UiElement::SetClasses(const std::vector<std::string> &classes)
  {
    if (classes == _classes) { return; }

    _classes = classes;
    Invalidate(UiDirty::Style);
    if (_host != nullptr) { _host->ClassesChanged(*this); }
  }

  bool UiElement::HasClass(const std::string &name) const
  {
    return std::ranges::find(_classes, name) != _classes.end();
  }

  bool UiElement::AddClass(const std::string &name)
  {
    if (name.empty() || HasClass(name)) { return false; }

    _classes.push_back(name);
    Invalidate(UiDirty::Style);
    if (_host != nullptr) { _host->ClassesChanged(*this); }
    return true;
  }

  bool UiElement::RemoveClass(const std::string &name)
  {
    const auto found = std::ranges::find(_classes, name);
    if (found == _classes.end()) { return false; }

    _classes.erase(found);
    Invalidate(UiDirty::Style);
    if (_host != nullptr) { _host->ClassesChanged(*this); }
    return true;
  }

  const std::string &UiElement::GetTitle() const
  {
    return _title;
  }

  void UiElement::SetTitle(const std::string &title)
  {
    _title = title;
  }

  int UiElement::GetTabIndex() const
  {
    return _tab_index;
  }

  void UiElement::SetTabIndex(const int tab_index)
  {
    _tab_index = tab_index;
  }

  void UiElement::SetWritten(const DataValue &written, const std::string &document)
  {
    _written = written;
    _document = document;
    Invalidate(UiDirty::Style);
  }

  const DataValue &UiElement::GetWritten() const
  {
    return _written;
  }

  const std::string &UiElement::GetDocumentName() const
  {
    return _document;
  }

  void UiElement::SetProperty(const std::string &name, const std::string &value)
  {
    const std::string css_name = UiProperties::ToCssName(name);

    std::erase_if(_set_properties, [&css_name](const auto &each) { return each.first == css_name; });

    // what was set last is the last to be read, and wins
    if (!value.empty()) { _set_properties.emplace_back(css_name, value); }

    Invalidate(UiDirty::Style);
  }

  const std::vector<std::pair<std::string, std::string>> &UiElement::GetSetProperties() const
  {
    return _set_properties;
  }

  void UiElement::SetComputedStyle(
    const UiStyle &style,
    const std::shared_ptr<const Variables> &variables,
    const std::size_t fingerprint)
  {
    bool inherited_changed = variables != _variables &&
                             (variables == nullptr || _variables == nullptr || *variables != *_variables);
    bool layout_changed = !(style.layout == _computed.layout) || fingerprint != _style_fingerprint;

    for (const auto &property : UiProperties::Get().GetAll())
    {
      if (property.IsShorthand() || (!property.is_inherited && !property.affects_layout)) { continue; }

      if (property.get(style) == property.get(_computed)) { continue; }

      if (property.is_inherited) { inherited_changed = true; }
      if (property.affects_layout) { layout_changed = true; }
    }

    // what scrolls decides whether what is inside it shrinks
    const bool scrolling_changed = style.overflow_x != _computed.overflow_x ||
                                   style.overflow_y != _computed.overflow_y ||
                                   style.layout.flex_direction != _computed.layout.flex_direction;

    _computed = style;
    _used = style;
    _has_computed_style = true;
    _variables = variables;
    _style_fingerprint = fingerprint;
    _style_is_dirty = false;
    _part_styles.clear();

    Invalidate(layout_changed ? UiDirty::Layout | UiDirty::Paint : UiDirty::Paint);

    for (const auto &child : _children)
    {
      if (inherited_changed) { child->Invalidate(UiDirty::Style); }
      if (scrolling_changed) { child->Invalidate(UiDirty::Layout); }
    }
  }

  void UiElement::SetUsedStyle(const UiStyle &style)
  {
    bool inherited_changed = false;
    bool layout_changed = !(style.layout == _used.layout);

    for (const auto &property : UiProperties::Get().GetAll())
    {
      if (property.IsShorthand() || (!property.is_inherited && !property.affects_layout)) { continue; }

      if (property.get(style) == property.get(_used)) { continue; }

      if (property.is_inherited) { inherited_changed = true; }
      if (property.affects_layout) { layout_changed = true; }
    }

    _used = style;
    _part_styles.clear();

    Invalidate(layout_changed ? UiDirty::Layout | UiDirty::Paint : UiDirty::Paint);

    if (inherited_changed)
    {
      for (const auto &child : _children) { child->Invalidate(UiDirty::Style); }
    }
  }

  const std::shared_ptr<const UiElement::Variables> &UiElement::GetVariables() const
  {
    return _variables;
  }

  void UiElement::Invalidate(const unsigned what)
  {
    if ((what & UiDirty::Style) != 0) { _style_is_dirty = true; }

    _dirty |= what;

    if (_host != nullptr) { _host->Invalidated(*this, what); }
  }

  void UiElement::InvalidateAll(const unsigned what)
  {
    Invalidate(what);
    for (const auto &child : _children) { child->InvalidateAll(what); }
  }

  bool UiElement::IsDirty(const unsigned what) const
  {
    if ((what & UiDirty::Style) != 0 && _style_is_dirty) { return true; }
    return (_dirty & what) != 0;
  }

  void UiElement::ClearDirty(const unsigned what)
  {
    _dirty &= ~what;
  }

  std::vector<UiNotice> UiElement::TakeNotices()
  {
    std::vector<UiNotice> notices;
    notices.swap(_notices);
    return notices;
  }

  void UiElement::Notify(
    const std::string &name,
    const std::string &value,
    const std::string &binding,
    const UiValue &bound_value)
  {
    _notices.push_back({name, value, binding, bound_value});
    Invalidate(UiDirty::Notice);
  }

  UiElement &UiElement::InsertChild(
    std::unique_ptr<UiElement> child,
    const std::size_t index,
    LayoutEngine *engine,
    const UiFrame *frame)
  {
    child->_parent = this;
    child->SetHost(_host);

    UiElement &added = *child;
    const std::size_t place = std::min(index, _children.size());
    _children.insert(_children.begin() + static_cast<std::ptrdiff_t>(place), std::move(child));

    if (engine != nullptr && _node != No_Layout_Node)
    {
      added.CreateLayout(*engine, frame);

      std::vector<LayoutNode> nodes;
      for (const auto &each : _children) { nodes.push_back(each->_node); }
      engine->SetChildren(_node, nodes);

      // a box with something inside is no longer measured by its content
      engine->SetMeasure(_node, nullptr);
    }

    added.InvalidateAll(UiDirty::Style | UiDirty::Layout | UiDirty::Paint);
    Invalidate(UiDirty::Layout | UiDirty::Paint);

    if (_host != nullptr) { _host->Joined(added); }
    return added;
  }

  std::unique_ptr<UiElement> UiElement::RemoveChild(UiElement &child, LayoutEngine *engine)
  {
    const auto found = std::ranges::find_if(_children, [&child](const auto &each) { return each.get() == &child; });
    if (found == _children.end()) { return nullptr; }

    if (_host != nullptr) { _host->Leaving(child); }

    std::unique_ptr<UiElement> removed = std::move(*found);
    _children.erase(found);

    if (engine != nullptr)
    {
      removed->DestroyLayout(*engine);

      if (_node != No_Layout_Node)
      {
        std::vector<LayoutNode> nodes;
        for (const auto &each : _children) { nodes.push_back(each->_node); }
        engine->SetChildren(_node, nodes);
      }
    }

    removed->_parent = nullptr;
    removed->SetHost(nullptr);

    Invalidate(UiDirty::Layout | UiDirty::Paint);
    return removed;
  }

  UiElement *UiElement::GetPreviousSibling() const
  {
    if (_parent == nullptr) { return nullptr; }

    UiElement *before = nullptr;
    for (const auto &child : _parent->_children)
    {
      if (child.get() == this) { return before; }
      before = child.get();
    }
    return nullptr;
  }

  UiElement *UiElement::GetNextSibling() const
  {
    if (_parent == nullptr) { return nullptr; }

    const auto &siblings = _parent->_children;
    for (std::size_t i = 0; i + 1 < siblings.size(); i++)
    {
      if (siblings[i].get() == this) { return siblings[i + 1].get(); }
    }
    return nullptr;
  }

  std::size_t UiElement::GetIndex() const
  {
    if (_parent == nullptr) { return 0; }

    const auto &siblings = _parent->_children;
    for (std::size_t i = 0; i < siblings.size(); i++)
    {
      if (siblings[i].get() == this) { return i; }
    }
    return 0;
  }

  float UiElement::GetScrollX() const
  {
    return _scroll_x;
  }

  float UiElement::GetScrollY() const
  {
    return _scroll_y;
  }

  bool UiElement::SetScroll(const float x, const float y)
  {
    const float new_x = std::clamp(x, 0.0f, GetMaxScrollX());
    const float new_y = std::clamp(y, 0.0f, GetMaxScrollY());

    if (new_x == _scroll_x && new_y == _scroll_y) { return false; }

    _scroll_x = new_x;
    _scroll_y = new_y;

    Invalidate(UiDirty::Arrange | UiDirty::Paint);
    return true;
  }

  float UiElement::GetMaxScrollX() const
  {
    if (!GetStyle().ScrollsX()) { return 0.0f; }
    return std::max(0.0f, _content_width - GetPaddingBox().Width());
  }

  float UiElement::GetMaxScrollY() const
  {
    if (!GetStyle().ScrollsY()) { return 0.0f; }
    return std::max(0.0f, _content_height - GetPaddingBox().Height());
  }

  float UiElement::GetContentWidth() const
  {
    return _content_width;
  }

  float UiElement::GetContentHeight() const
  {
    return _content_height;
  }

  bool UiElement::CanScrollX() const
  {
    return GetMaxScrollX() > 0.0f;
  }

  bool UiElement::CanScrollY() const
  {
    return GetMaxScrollY() > 0.0f;
  }

  float UiElement::GetScrollbarWidth() const
  {
    return UiScrollbars::WidthOf(GetStyle());
  }

  const UiRectangle &UiElement::GetVisibleBox() const
  {
    return _visible_box;
  }

  bool UiElement::IsClippedAway() const
  {
    return _is_clipped_away;
  }

  const std::string &UiElement::GetCssType() const
  {
    return _type;
  }

  const std::string &UiElement::GetCssId() const
  {
    return _name;
  }

  bool UiElement::HasCssClass(const std::string &name) const
  {
    return HasClass(name);
  }

  bool UiElement::GetCssAttribute(const std::string &name, std::string &value) const
  {
    if (name == "class")
    {
      value.clear();
      for (const auto &each : _classes)
      {
        if (!value.empty()) { value += ' '; }
        value += each;
      }
      return !_classes.empty();
    }

    if (name == "name" || name == "id")
    {
      value = _name;
      return !_name.empty();
    }

    if (name == "type")
    {
      value = _type;
      return true;
    }

    if (name == "title")
    {
      value = _title;
      return !_title.empty();
    }

    // what the element holds now, and then what its file wrote
    if (FieldValue field; GetField(name, field))
    {
      if (const auto *text = std::get_if<std::string>(&field)) { value = *text; }
      else if (const auto *flag = std::get_if<bool>(&field)) { value = *flag ? "true" : "false"; }
      else if (const auto *whole = std::get_if<int>(&field)) { value = std::to_string(*whole); }
      else if (const auto *number = std::get_if<float>(&field)) { value = UiValue::Number(*number).AsText(); }
      else { return false; }

      return true;
    }

    const DataValue *written = _written.Find(name);
    if (written == nullptr) { return false; }

    if (written->GetText(value)) { return true; }

    if (bool flag = false; written->GetBool(flag))
    {
      value = flag ? "true" : "false";
      return true;
    }

    if (double number = 0.0; written->GetNumber(number))
    {
      value = UiValue::Number(number).AsText();
      return true;
    }

    return false;
  }

  bool UiElement::IsInCssState(const std::string &name) const
  {
    if (name == "hover") { return _states.hover; }
    if (name == "active") { return _states.active; }
    if (name == "focus") { return _states.focus; }
    if (name == "focus-within") { return _states.focus_within || _states.focus; }
    if (name == "disabled") { return _states.disabled; }
    if (name == "enabled") { return !_states.disabled; }
    if (name == "checked") { return _states.checked; }
    if (name == "invalid") { return _states.invalid; }
    if (name == "valid") { return !_states.invalid; }
    return false;
  }

  const CssElement *UiElement::GetCssParent() const
  {
    return _parent;
  }

  const CssElement *UiElement::GetCssPreviousSibling() const
  {
    return GetPreviousSibling();
  }

  std::size_t UiElement::GetCssIndex() const
  {
    return GetIndex() + 1;
  }

  std::size_t UiElement::GetCssSiblingCount() const
  {
    return _parent != nullptr ? _parent->_children.size() : 1;
  }

  bool UiElement::HasCssChildren() const
  {
    return !_children.empty();
  }

  void UiElement::SetHidden(const UiFlag &hidden)
  {
    _hidden = hidden;
  }

  UiElement &UiElement::AddChild(std::unique_ptr<UiElement> child)
  {
    child->_parent = this;
    child->SetHost(_host);
    _children.push_back(std::move(child));
    return *_children.back();
  }

  UiElement *UiElement::GetParent() const
  {
    return _parent;
  }

  const std::vector<std::unique_ptr<UiElement>> &UiElement::GetChildren() const
  {
    return _children;
  }

  std::vector<UiElement *> UiElement::GetChildrenInPaintOrder() const
  {
    std::vector<UiElement *> children;
    children.reserve(_children.size());
    for (const auto &child : _children) { children.push_back(child.get()); }

    std::ranges::stable_sort(children, [](const UiElement *a, const UiElement *b)
    {
      return a->GetStyle().z_index < b->GetStyle().z_index;
    });
    return children;
  }

  const UiStates &UiElement::GetStates() const
  {
    return _states;
  }

  void UiElement::SetStates(const UiStates &states)
  {
    if (states == _states) { return; }

    const UiStates before = _states;
    _states = states;
    Invalidate(UiDirty::Style);

    if (_host != nullptr) { _host->StatesChanged(*this, before, states); }
  }

  void UiElement::RefreshStates()
  {
    UiStates states = _states;
    states.disabled = !IsEnabled();
    states.checked = IsChecked();
    states.invalid = IsInvalid();
    SetStates(states);
  }

  void UiElement::Replace()
  {
    Place(_clip);
  }

  void UiElement::UpdateHidden(const bool parent_is_hidden)
  {
    const bool is_hidden = parent_is_hidden || _is_hidden_by_itself ||
                           GetStyle().layout.display == LayoutDisplay::None;

    if (is_hidden != _is_hidden)
    {
      _is_hidden = is_hidden;
      Invalidate(UiDirty::Paint);
    }

    for (const auto &child : _children) { child->UpdateHidden(_is_hidden); }
  }

  bool UiElement::IsHidden() const
  {
    return _is_hidden;
  }

  void UiElement::CreateLayout(LayoutEngine &engine, const UiFrame *frame)
  {
    _node = engine.CreateNode();

    std::vector<LayoutNode> children;
    for (const auto &child : _children)
    {
      child->CreateLayout(engine, frame);
      children.push_back(child->_node);
    }
    engine.SetChildren(_node, children);

    if (HasContent() && _children.empty())
    {
      // the frame outlives the element, and holds the scale of the moment
      engine.SetMeasure(_node, [this, frame](const float available_width, const float available_height)
      {
        const LayoutSize size = Measure(*frame, available_width, available_height);

        _was_measured_by_layout = true;
        _measured_available_width = available_width;
        _measured_available_height = available_height;
        _measured_by_layout = size;
        return size;
      });
    }
  }

  void UiElement::DestroyLayout(LayoutEngine &engine)
  {
    for (const auto &child : _children) { child->DestroyLayout(engine); }

    if (_node != No_Layout_Node) { engine.DestroyNode(_node); }
    _node = No_Layout_Node;
  }

  LayoutNode UiElement::GetLayoutNode() const
  {
    return _node;
  }

  void UiElement::Prepare(LayoutEngine &engine, const UiFrame &frame, const bool parent_is_hidden)
  {
    Follow(frame, parent_is_hidden);

    std::vector<UiElement *> elements;
    CollectUiElements(*this, elements);

    for (UiElement *element : elements) { element->PushLayoutStyle(engine); }
  }

  void UiElement::Arrange(const LayoutEngine &engine, const float parent_left, const float parent_top)
  {
    TakeLayout(engine, true);
    PlaceFrom(parent_left, parent_top, Everything());
  }

  void UiElement::Follow(const UiFrame &frame, const bool parent_is_hidden)
  {
    Update(frame);

    const bool hidden_by_itself = _hidden.Get(*frame.values, false);
    if (hidden_by_itself != _is_hidden_by_itself)
    {
      _is_hidden_by_itself = hidden_by_itself;
      Invalidate(UiDirty::Layout | UiDirty::Paint);
    }

    RefreshStates();

    const bool is_hidden = parent_is_hidden || hidden_by_itself ||
                           GetStyle().layout.display == LayoutDisplay::None;
    if (is_hidden != _is_hidden)
    {
      _is_hidden = is_hidden;
      Invalidate(UiDirty::Paint);
    }

    // An element that does not tell when it changed is asked how large
    // its content is, which is what a change of it would change.
    if (!_is_hidden && HasContent() && _children.empty() && !TellsWhenItChanged())
    {
      const float unlimited = std::numeric_limits<float>::quiet_NaN();
      const LayoutSize size = Measure(frame, unlimited, unlimited);

      if (size.width != _measured.width || size.height != _measured.height)
      {
        _measured = size;
        Invalidate(UiDirty::Layout | UiDirty::Paint);
      }
    }

    for (const auto &child : _children) { child->Follow(frame, _is_hidden); }
  }

  LayoutStyle UiElement::GetLayoutStyle() const
  {
    const UiStyle &style = GetStyle();

    LayoutStyle layout = style.layout;
    if (_is_hidden_by_itself) { layout.display = LayoutDisplay::None; }

    // What is scrolled is as large as it asks to be, and is not made to
    // fit. It is what the least size of a flex item does in CSS, which
    // the layout leaves out.
    if (_parent != nullptr)
    {
      const UiStyle &around = _parent->GetStyle();
      if (IsRow(around) ? around.ScrollsX() : around.ScrollsY()) { layout.flex_shrink = 0.0f; }
    }

    // `scroll` keeps the room of its scrollbar, whether there is
    // something to scroll or not
    if (const float width = UiScrollbars::WidthOf(style); width > 0.0f)
    {
      // The room is taken from what is inside, and the box stays as large
      // as it is written. Where the size measures the content, the
      // content is what gets smaller.
      const bool measures_content = layout.box_sizing == LayoutBoxSizing::ContentBox;

      if (style.OverflowY() == UiOverflow::Scroll)
      {
        layout.padding.right = Added(layout.padding.right, width);
        if (measures_content && !layout.width.IsAuto()) { layout.width = Added(layout.width, -width); }
      }

      if (style.OverflowX() == UiOverflow::Scroll)
      {
        layout.padding.bottom = Added(layout.padding.bottom, width);
        if (measures_content && !layout.height.IsAuto()) { layout.height = Added(layout.height, -width); }
      }
    }

    return layout;
  }

  void UiElement::PushLayoutStyle(LayoutEngine &engine)
  {
    if (_node == No_Layout_Node) { return; }

    _pushed_layout = GetLayoutStyle();
    _has_pushed_layout = true;
    engine.SetStyle(_node, _pushed_layout);
  }

  bool UiElement::LaysOutTheSame(const UiFrame &frame)
  {
    if (!_has_pushed_layout || !(GetLayoutStyle() == _pushed_layout)) { return false; }

    // what holds other elements is asked because of them: one came, went,
    // or changed, which is a layout in every case
    if (!HasContent() || !_children.empty() || !_was_measured_by_layout) { return false; }

    const LayoutSize size = Measure(frame, _measured_available_width, _measured_available_height);
    return size.width == _measured_by_layout.width && size.height == _measured_by_layout.height;
  }

  void UiElement::TakeLayout(const LayoutEngine &engine, const bool with_itself)
  {
    if (with_itself) { _layout_box = engine.GetBox(_node); }

    for (const auto &child : _children) { child->TakeLayout(engine, true); }
  }

  void UiElement::PlaceFrom(const float parent_left, const float parent_top, const UiRectangle &clip)
  {
    _box.left = parent_left + _layout_box.left;
    _box.top = parent_top + _layout_box.top;
    _box.right = _box.left + _layout_box.width;
    _box.bottom = _box.top + _layout_box.height;

    Place(clip);
  }

  void UiElement::Place(const UiRectangle &clip)
  {
    const UiStyle &style = GetStyle();
    const UiRectangle padding_box = GetPaddingBox();

    // How large what is inside is, from the corner of the padding box:
    // as far as the farthest element reaches, and the padding behind it.
    float content_width = padding_box.Width();
    float content_height = padding_box.Height();

    for (const auto &child : _children)
    {
      if (child->_is_hidden) { continue; }

      const LayoutBox &box = child->_layout_box;

      content_width = std::max(
        content_width, box.left + box.width - _layout_box.border.left + _layout_box.padding.right);
      content_height = std::max(
        content_height, box.top + box.height - _layout_box.border.top + _layout_box.padding.bottom);
    }

    // what the element draws itself, such as a text that is typed into
    if (float own_width = 0.0f, own_height = 0.0f; GetContentSize(own_width, own_height))
    {
      content_width = std::max(content_width, own_width + _layout_box.padding.left + _layout_box.padding.right);
      content_height = std::max(content_height, own_height + _layout_box.padding.top + _layout_box.padding.bottom);
    }

    _content_width = content_width;
    _content_height = content_height;

    // what was scrolled to may be gone
    _scroll_x = style.ScrollsX() ? std::clamp(_scroll_x, 0.0f, std::max(0.0f, content_width - padding_box.Width())) : 0.0f;
    _scroll_y = style.ScrollsY() ? std::clamp(_scroll_y, 0.0f, std::max(0.0f, content_height - padding_box.Height())) : 0.0f;

    UiRectangle inside = clip;
    if (style.ClipsX())
    {
      inside.left = std::max(inside.left, padding_box.left);
      inside.right = std::min(inside.right, padding_box.right);
    }
    if (style.ClipsY())
    {
      inside.top = std::max(inside.top, padding_box.top);
      inside.bottom = std::min(inside.bottom, padding_box.bottom);
    }

    // what is drawn of the element and of what is inside it
    const float around = std::max(0.0f, style.outline_width + std::max(0.0f, style.outline_offset));
    _bounds = {_box.left - around, _box.top - around, _box.right + around, _box.bottom + around};

    for (const auto &child : _children)
    {
      child->PlaceFrom(_box.left - _scroll_x, _box.top - _scroll_y, inside);

      if (child->_is_hidden) { continue; }

      const UiRectangle of_child = Intersected(child->_bounds, inside);
      if (of_child.IsEmpty()) { continue; }

      _bounds.left = std::min(_bounds.left, of_child.left);
      _bounds.top = std::min(_bounds.top, of_child.top);
      _bounds.right = std::max(_bounds.right, of_child.right);
      _bounds.bottom = std::max(_bounds.bottom, of_child.bottom);
    }

    _visible_box = Intersected(_box, clip);
    _is_clipped_away = Intersected(_bounds, clip).IsEmpty();
    _clip = clip;

    ClearDirty(UiDirty::Arrange);
  }

  bool UiElement::IsLayoutBoundary() const
  {
    if (_parent == nullptr) { return true; }

    const LayoutStyle &layout = GetStyle().layout;

    // The size is written in pixels, so that nothing inside decides it.
    // A percentage may refer to a size that follows from content.
    const auto is_fixed = [](const LayoutLength &length)
    {
      return length.unit == LayoutLength::Unit::Pixels ||
             (length.unit == LayoutLength::Unit::Sum && length.percent == 0.0f);
    };

    return is_fixed(layout.width) && is_fixed(layout.height) && !_is_hidden;
  }

  const LayoutBox &UiElement::GetLayoutBox() const
  {
    return _layout_box;
  }

  const UiRectangle &UiElement::GetBox() const
  {
    return _box;
  }

  UiRectangle UiElement::GetPaddingBox() const
  {
    return {
      _box.left + _layout_box.border.left,
      _box.top + _layout_box.border.top,
      _box.right - _layout_box.border.right,
      _box.bottom - _layout_box.border.bottom
    };
  }

  UiRectangle UiElement::GetContentBox() const
  {
    const UiRectangle inside = GetPaddingBox();
    return {
      inside.left + _layout_box.padding.left,
      inside.top + _layout_box.padding.top,
      inside.right - _layout_box.padding.right,
      inside.bottom - _layout_box.padding.bottom
    };
  }

  void UiElement::Paint(UiPainter &painter, const UiFrame &frame, float opacity)
  {
    if (_is_hidden) { return; }

    // nothing of it is inside of what it is cut off at
    if (_is_clipped_away) { return; }

    const UiStyle &style = GetStyle();
    opacity *= style.opacity;
    if (opacity <= 0.0f) { return; }

    // What is hidden by `visibility` keeps its room and is not drawn. What
    // is inside it is, where it says that it is visible.
    if (style.visibility == UiVisibility::Hidden)
    {
      for (UiElement *child : GetChildrenInPaintOrder()) { child->Paint(painter, frame, opacity); }
      return;
    }

    const UiRectangle border_box = ToPixels(_box, frame.scale);
    const UiRectangle padding_box = ToPixels(GetPaddingBox(), frame.scale);

    const UiBoxPaint box(style, border_box, padding_box, frame.scale);

    // everything of the element is moved with it, and so is what is
    // inside it
    const bool is_moved = !style.transform.empty();
    if (is_moved) { painter.PushTransform(MatrixOf(style, border_box, frame.scale)); }

    // A shader of its own draws the element itself: its box and its
    // content. What is inside it is drawn as it would be without.
    const int material = style.shader.empty() || frame.resources == nullptr
      ? No_Material
      : frame.resources->GetMaterial(style.shader, Describe());

    if (material != No_Material)
    {
      painter.SetMaterial(material, frame.resources->GetMaterialValues(style, frame.values), border_box);
    }

    const bool is_plain = box.IsPlain();

    if (is_plain)
    {
      painter.FillRectangle(border_box, Faded(style.background_color, opacity));
    } else
    {
      box.PaintBackground(painter, opacity);
    }

    if (!style.background_image.empty() && frame.resources != nullptr)
    {
      PaintBackgroundImage(painter, frame, style, box, border_box, opacity, Describe());
    }

    if (!style.border_image_source.empty())
    {
      const UiImage image =
        frame.resources->GetImageFor(style.border_image_source, 0.0f, 0.0f, frame.scale, Describe());

      // how far the corners reach is said by the file, or by the atlas
      // the image is a part of
      const bool is_sliced = style.border_image_slice.top > 0.0f || style.border_image_slice.right > 0.0f ||
                             style.border_image_slice.bottom > 0.0f || style.border_image_slice.left > 0.0f;

      const auto &slice = !is_sliced && image.has_slice ? image.slice : style.border_image_slice;
      const auto &width = style.border_image_width;

      // a part that says nothing about its width is as wide as it is in
      // the image
      const LayoutEdges<float> widths{
        std::round((width.top < 0.0f ? slice.top : width.top) * frame.scale),
        std::round((width.right < 0.0f ? slice.right : width.right) * frame.scale),
        std::round((width.bottom < 0.0f ? slice.bottom : width.bottom) * frame.scale),
        std::round((width.left < 0.0f ? slice.left : width.left) * frame.scale)
      };

      painter.SetFilter(FilterOf(style));

      if (style.border_image_repeat == UiBorderImageRepeat::Stretch)
      {
        painter.DrawNineSlice(image, border_box, slice, widths, {1.0f, 1.0f, 1.0f, opacity});
      } else
      {
        PaintNineSlice(
          painter, image, border_box, slice, widths, style.border_image_repeat, {1.0f, 1.0f, 1.0f, opacity});
      }

      painter.SetFilter(TextureFilter2D::Smooth);
    }

    if (is_plain)
    {
      // the widths of the border follow from where the padding box landed,
      // so that the border meets it without a gap
      painter.FillBorder(
        border_box,
        {
          padding_box.top - border_box.top,
          border_box.right - padding_box.right,
          border_box.bottom - padding_box.bottom,
          padding_box.left - border_box.left
        },
        Faded(style.BorderColor(), opacity));
    } else
    {
      box.PaintBorder(painter, opacity);
    }

    const bool clips = style.ClipsX() || style.ClipsY();
    if (clips)
    {
      // up to the scrollbars, and inside the round corners
      const UiRectangle clip_box = UiScrollbars::ClipOf(style, padding_box);

      if (box.IsRound())
      {
        painter.PushRoundedClip(clip_box, box.GetInnerRadii());
      } else
      {
        painter.PushClip(clip_box);
      }
    }

    painter.SetFilter(FilterOf(style));
    PaintContent(painter, frame, ToPixels(GetContentBox(), frame.scale), opacity);
    painter.SetFilter(TextureFilter2D::Smooth);

    if (material != No_Material) { painter.SetMaterial(No_Material, {}, {}); }

    for (UiElement *child : GetChildrenInPaintOrder()) { child->Paint(painter, frame, opacity); }

    if (clips)
    {
      if (box.IsRound())
      {
        painter.PopRoundedClip();
      } else
      {
        painter.PopClip();
      }
    }

    UiScrollbars::Paint(*this, painter, frame, opacity);

    if (style.outline_width > 0.0f)
    {
      // around the border box, and as far from it as the offset says
      const float offset = std::round(style.outline_offset * frame.scale);
      const float width = std::max(1.0f, std::round(style.outline_width * frame.scale));

      if (box.IsRound())
      {
        box.PaintOutline(painter, offset, width, Faded(style.OutlineColor(), opacity));
      } else
      {
        const UiRectangle outer{
          border_box.left - offset - width,
          border_box.top - offset - width,
          border_box.right + offset + width,
          border_box.bottom + offset + width
        };

        painter.FillBorder(outer, {width, width, width, width}, Faded(style.OutlineColor(), opacity));
      }
    }

    if (is_moved) { painter.PopTransform(); }
  }

  bool UiElement::ToLocal(const float scale, float &x, float &y) const
  {
    const UiStyle &style = GetStyle();
    if (style.transform.empty()) { return true; }

    UiMatrix back;
    if (!MatrixOf(style, ToPixels(_box, scale), scale).Invert(back)) { return false; }

    back.Apply(x, y);
    return true;
  }

  bool UiElement::Contains(const float scale, const float x, const float y) const
  {
    const UiBoxPaint box(GetStyle(), ToPixels(_box, scale), ToPixels(GetPaddingBox(), scale), scale);
    return box.Contains(x, y);
  }

  bool UiElement::ContainsInPadding(const float scale, const float x, const float y) const
  {
    const UiBoxPaint box(GetStyle(), ToPixels(_box, scale), ToPixels(GetPaddingBox(), scale), scale);
    return box.ContainsInPadding(x, y);
  }
} // neon
