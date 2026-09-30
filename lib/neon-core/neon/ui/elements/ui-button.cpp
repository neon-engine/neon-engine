#include "ui-button.hpp"

#include <format>

namespace neon
{
  void UiButton::ApplyDefaults(UiStyle &style) const
  {
    style.layout.padding = {
      LayoutLength::Pixels(8.0f),
      LayoutLength::Pixels(16.0f),
      LayoutLength::Pixels(8.0f),
      LayoutLength::Pixels(16.0f)
    };

    // what is inside a button is in its middle
    style.layout.justify_content = JustifyContent::Center;
    style.layout.align_items = AlignItems::Center;
    style.text_align = TextAlign::Center;

    style.background_color = {0.231f, 0.259f, 0.322f, 1.0f}; // #3b4252
    style.color = {1.0f, 1.0f, 1.0f, 1.0f};
    style.pointer_events = UiPointerEvents::Auto;
  }

  void UiButton::ApplyStateDefaults(UiStyle &style, const UiStates &states) const
  {
    if (states.disabled)
    {
      style.opacity = 0.5f;
      return;
    }

    if (states.focus)
    {
      style.outline_width = 2.0f;
      style.outline_offset = 2.0f;
      style.outline_color = Color{1.0f, 0.820f, 0.400f, 1.0f}; // #ffd166
    }

    if (states.hover) { style.background_color = {0.298f, 0.337f, 0.416f, 1.0f}; } // #4c566a
    if (states.active) { style.background_color = {0.180f, 0.204f, 0.251f, 1.0f}; } // #2e3440
  }

  void UiButton::ReadAttributes(const DataReader &reader)
  {
    _text.Read(reader);
    reader.Read("autofocus", _autofocus);

    if (const auto *value = reader.ReadValue("enabled"); value != nullptr && !UiFlag::Read(*value, _enabled))
    {
      std::string text;
      reader.Report(*value, std::format(
                      "'enabled' of {} is {}, where true, false, or a value such as \"{{can_start}}\" was expected",
                      reader.GetWhere(),
                      value->GetText(text) ? "'" + text + "'" : DataValue::Describe(value->GetKind())));
    }
  }

  bool UiButton::TakesChildren() const
  {
    return true;
  }

  bool UiButton::IsFocusable() const
  {
    return true;
  }

  bool UiButton::IsClickable() const
  {
    return true;
  }

  bool UiButton::WantsFocus() const
  {
    return _autofocus;
  }

  bool UiButton::IsEnabled() const
  {
    return _is_enabled;
  }

  bool UiButton::HasContent() const
  {
    return _text.IsWritten();
  }

  LayoutSize UiButton::Measure(const UiFrame &frame, const float available_width, float available_height)
  {
    return _text.Measure(GetStyle(), frame, available_width);
  }

  void UiButton::Update(const UiFrame &frame)
  {
    _text.Update(frame);
    _is_enabled = _enabled.Get(*frame.values, true);
  }

  void UiButton::PaintContent(
    UiPainter &painter,
    const UiFrame &frame,
    const UiRectangle &content_box,
    const float opacity)
  {
    if (_text.IsWritten()) { _text.Paint(painter, GetStyle(), frame, content_box, opacity, true); }
  }

  const std::string &UiButton::GetText() const
  {
    return _text.Get();
  }
} // neon
