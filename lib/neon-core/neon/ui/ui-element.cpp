#include "ui-element.hpp"

#include <algorithm>
#include <cmath>

namespace neon
{
  namespace
  {
    Color Faded(const Color &color, const float opacity)
    {
      return {color.r, color.g, color.b, color.a * opacity};
    }
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
    if (index < kStyle_Count) { _styles[index] = style; }
  }

  const UiStyle &UiElement::GetStyle() const
  {
    return _styles[StyleIndex(_states)];
  }

  void UiElement::SetHidden(const UiFlag &hidden)
  {
    _hidden = hidden;
  }

  UiElement &UiElement::AddChild(std::unique_ptr<UiElement> child)
  {
    child->_parent = this;
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
    _states = states;
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
        return Measure(*frame, available_width, available_height);
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
    Update(frame);

    const bool hidden_by_itself = _hidden.Get(*frame.values, false);
    _states.disabled = !IsEnabled();

    LayoutStyle layout = GetStyle().layout;
    if (hidden_by_itself) { layout.display = LayoutDisplay::None; }

    _is_hidden = parent_is_hidden || layout.display == LayoutDisplay::None;

    engine.SetStyle(_node, layout);

    for (const auto &child : _children) { child->Prepare(engine, frame, _is_hidden); }
  }

  void UiElement::Arrange(const LayoutEngine &engine, const float parent_left, const float parent_top)
  {
    _layout_box = engine.GetBox(_node);

    _box.left = parent_left + _layout_box.left;
    _box.top = parent_top + _layout_box.top;
    _box.right = _box.left + _layout_box.width;
    _box.bottom = _box.top + _layout_box.height;

    for (const auto &child : _children) { child->Arrange(engine, _box.left, _box.top); }
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

    const UiStyle &style = GetStyle();
    opacity *= style.opacity;
    if (opacity <= 0.0f) { return; }

    const UiRectangle border_box = ToPixels(_box, frame.scale);
    const UiRectangle padding_box = ToPixels(GetPaddingBox(), frame.scale);

    painter.FillRectangle(border_box, Faded(style.background_color, opacity));

    if (!style.background_image.empty())
    {
      painter.DrawImage(
        frame.resources->GetImage(style.background_image),
        border_box,
        {0.0f, 0.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f, opacity});
    }

    if (!style.border_image_source.empty())
    {
      const auto &slice = style.border_image_slice;
      const auto &width = style.border_image_width;

      // a part that says nothing about its width is as wide as it is in
      // the image
      const LayoutEdges<float> widths{
        std::round((width.top < 0.0f ? slice.top : width.top) * frame.scale),
        std::round((width.right < 0.0f ? slice.right : width.right) * frame.scale),
        std::round((width.bottom < 0.0f ? slice.bottom : width.bottom) * frame.scale),
        std::round((width.left < 0.0f ? slice.left : width.left) * frame.scale)
      };

      painter.DrawNineSlice(
        frame.resources->GetImage(style.border_image_source),
        border_box,
        slice,
        widths,
        {1.0f, 1.0f, 1.0f, opacity});
    }

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

    const bool clips = style.overflow == UiOverflow::Hidden;
    if (clips) { painter.PushClip(padding_box); }

    PaintContent(painter, frame, ToPixels(GetContentBox(), frame.scale), opacity);

    for (UiElement *child : GetChildrenInPaintOrder()) { child->Paint(painter, frame, opacity); }

    if (clips) { painter.PopClip(); }

    if (style.outline_width > 0.0f)
    {
      // around the border box, and as far from it as the offset says
      const float offset = std::round(style.outline_offset * frame.scale);
      const float width = std::max(1.0f, std::round(style.outline_width * frame.scale));

      const UiRectangle outer{
        border_box.left - offset - width,
        border_box.top - offset - width,
        border_box.right + offset + width,
        border_box.bottom + offset + width
      };

      painter.FillBorder(outer, {width, width, width, width}, Faded(style.OutlineColor(), opacity));
    }
  }
} // neon
