#include "ui-checkbox.hpp"

#include <algorithm>
#include <cmath>
#include <format>

#include <neon/ui/ui-fields.hpp>

#include "ui-binding.hpp"

namespace neon
{
  // Helpers of UiCheckbox, for this file alone.
  namespace
  {
    // between the box and the text, in units of the file
    constexpr float gap = 8.0f;

    Color Faded(const Color &color, const float opacity)
    {
      return {color.r, color.g, color.b, color.a * opacity};
    }
  }

  UiCheckbox::UiCheckbox() = default;

  float UiCheckbox::BoxWidth() const
  {
    return 18.0f;
  }

  float UiCheckbox::BoxHeight() const
  {
    return 18.0f;
  }

  void UiCheckbox::SetChecked(const bool checked)
  {
    if (checked == _checked) { return; }

    _checked = checked;
    Invalidate(UiDirty::Style | UiDirty::Paint);
    Notify("changed", checked ? "true" : "false", _binding, UiValue::Flag(checked));
  }

  void UiCheckbox::ApplyDefaults(UiStyle &style) const
  {
    style.pointer_events = UiPointerEvents::Auto;
    style.cursor = UiCursor::Pointer;
    style.layout.align_items = AlignItems::Center;
    style.layout.padding = {
      LayoutLength::Pixels(4.0f), LayoutLength::Pixels(4.0f), LayoutLength::Pixels(4.0f), LayoutLength::Pixels(4.0f)
    };
    style.color = {1.0f, 1.0f, 1.0f, 1.0f};
    style.text_align = TextAlign::Left;
  }

  void UiCheckbox::ApplyStateDefaults(UiStyle &style, const UiStates &states) const
  {
    if (states.disabled)
    {
      style.opacity = 0.5f;
      return;
    }

    if (states.focus)
    {
      style.outline_width = 2.0f;
      style.outline_offset = 0.0f;
      style.outline_color = Color{1.0f, 0.820f, 0.400f, 1.0f}; // #ffd166
    }
  }

  void UiCheckbox::ApplyPartDefaults(const std::string &part, UiStyle &style) const
  {
    UiElement::ApplyPartDefaults(part, style);

    const UiStyle &of_element = GetStyle();

    if (part == "box")
    {
      style.background_color = {1.0f, 1.0f, 1.0f, 0.08f};
      style.border_color = Color{1.0f, 1.0f, 1.0f, 0.5f};
      style.layout.border = {2.0f, 2.0f, 2.0f, 2.0f};
    } else if (part == "mark")
    {
      style.background_color = of_element.accent_color;
    }
  }

  void UiCheckbox::ReadAttributes(const DataReader &reader)
  {
    _text.Read(reader);
    reader.Read("autofocus", _autofocus);

    if (const auto *value = reader.ReadValue("checked"); value != nullptr)
    {
      std::string text;
      const bool is_read = UiBinding::Read(*value, text, _binding) && (text == "true" || text == "false" || !_binding.empty());

      if (is_read)
      {
        _checked = text == "true";
      } else
      {
        reader.Report(*value, std::format(
                        "'checked' of {} is {}, where true, false, or a value such as \"{{fullscreen}}\" was expected",
                        reader.GetWhere(),
                        value->GetText(text) ? "'" + text + "'" : DataValue::Describe(value->GetKind())));
      }
    }

    if (const auto *value = reader.ReadValue("enabled"); value != nullptr && !UiFlag::Read(*value, _enabled))
    {
      std::string text;
      reader.Report(*value, std::format(
                      "'enabled' of {} is {}, where true, false, or a value such as \"{{can_choose}}\" was expected",
                      reader.GetWhere(),
                      value->GetText(text) ? "'" + text + "'" : DataValue::Describe(value->GetKind())));
    }
  }

  bool UiCheckbox::IsFocusable() const
  {
    return true;
  }

  bool UiCheckbox::IsClickable() const
  {
    return true;
  }

  bool UiCheckbox::WantsFocus() const
  {
    return _autofocus;
  }

  bool UiCheckbox::IsEnabled() const
  {
    return _is_enabled;
  }

  bool UiCheckbox::IsChecked() const
  {
    return _checked;
  }

  bool UiCheckbox::HasContent() const
  {
    return true;
  }

  bool UiCheckbox::TellsWhenItChanged() const
  {
    return true;
  }

  LayoutSize UiCheckbox::Measure(const UiFrame &frame, const float available_width, const float available_height)
  {
    (void) available_height;

    LayoutSize size{BoxWidth(), BoxHeight()};
    if (!_text.IsWritten()) { return size; }

    const float room = std::isnan(available_width) ? available_width : std::max(0.0f, available_width - size.width - gap);
    const LayoutSize text = _text.Measure(GetStyle(), frame, room);

    size.width += gap + text.width;
    size.height = std::max(size.height, text.height);
    return size;
  }

  void UiCheckbox::Update(const UiFrame &frame)
  {
    const std::string before = _text.Get();
    _text.Update(frame);
    if (_text.Get() != before) { Invalidate(UiDirty::Layout | UiDirty::Paint); }

    _is_enabled = _enabled.Get(*frame.values, true);

    if (_binding.empty()) { return; }
    if (_has_followed && _followed_revision == frame.values->GetRevision()) { return; }

    _has_followed = true;
    _followed_revision = frame.values->GetRevision();

    if (const UiValue *value = frame.values->Find(_binding); value != nullptr && value->AsFlag() != _checked)
    {
      _checked = value->AsFlag();
      Invalidate(UiDirty::Style | UiDirty::Paint);
    }
  }

  void UiCheckbox::Interact(UiInteraction &interaction, const UiFrame &frame)
  {
    (void) frame;

    if (interaction.kind == UiInteraction::Kind::Accept)
    {
      SetChecked(!_checked);
      interaction.is_used = true;
    }
  }

  void UiCheckbox::PaintBox(UiPainter &painter, const UiFrame &frame, const UiRectangle &box, const float opacity)
  {
    const UiStyle &of_box = GetPartStyle("box");
    const float border = std::max(1.0f, std::round(of_box.layout.border.left * frame.scale));

    painter.FillRectangle(box, Faded(of_box.background_color, opacity));
    painter.FillBorder(box, {border, border, border, border}, Faded(of_box.BorderColor(), opacity));

    if (!_checked) { return; }

    // the tick fills the middle of the box
    const float inset = std::floor(box.Width() * 0.25f);
    painter.FillRectangle(
      {box.left + inset, box.top + inset, box.right - inset, box.bottom - inset},
      Faded(GetPartStyle("mark").background_color, opacity));
  }

  void UiCheckbox::PaintContent(
    UiPainter &painter,
    const UiFrame &frame,
    const UiRectangle &content_box,
    const float opacity)
  {
    const float width = std::round(BoxWidth() * frame.scale);
    const float height = std::round(BoxHeight() * frame.scale);

    // the box in the middle of the height, and the text behind it
    const float top = std::round(content_box.top + (content_box.Height() - height) / 2.0f);
    PaintBox(painter, frame, {content_box.left, top, content_box.left + width, top + height}, opacity);

    if (!_text.IsWritten()) { return; }

    const float text_left = content_box.left + width + std::round(gap * frame.scale);
    _text.Paint(
      painter, GetStyle(), frame, {text_left, content_box.top, content_box.right, content_box.bottom}, opacity, true);
  }

  bool UiCheckbox::GetField(const std::string &name, FieldValue &value) const
  {
    if (name == "checked")
    {
      value = _checked;
      return true;
    }

    if (name == "text")
    {
      if (!_text.IsWritten()) { return false; }
      value = _text.Get();
      return true;
    }

    if (name == "enabled")
    {
      value = _is_enabled;
      return true;
    }

    if (name == "autofocus")
    {
      value = _autofocus;
      return true;
    }

    return false;
  }

  bool UiCheckbox::SetField(const std::string &name, const FieldValue &value, std::string &error)
  {
    const std::string what = "'" + name + "' of " + Describe();

    if (name == "checked")
    {
      bool flag = false;
      if (!TakeFlag(value, what, flag, error)) { return false; }

      SetChecked(flag);
      return true;
    }

    if (name == "text")
    {
      std::string text;
      if (!TakeText(value, what, text, error)) { return false; }

      if (std::string problem; !_text.Set(text, problem))
      {
        error = what + " cannot be read: " + problem;
        return false;
      }

      Invalidate(UiDirty::Layout | UiDirty::Paint);
      return true;
    }

    if (name == "enabled")
    {
      bool flag = false;
      if (!TakeFlag(value, what, flag, error)) { return false; }

      _enabled = UiFlag(flag);
      _is_enabled = flag;
      return true;
    }

    if (name == "autofocus") { return TakeFlag(value, what, _autofocus, error); }

    return UiElement::SetField(name, value, error);
  }

  std::vector<UiElement::Field> UiCheckbox::GetFields() const
  {
    return {
      {"checked", FieldKind::Bool, "Whether it is ticked, which may follow a value of the game as {name}", {}},
      {"text", FieldKind::Text, "What is written next to it, which may refer to values of the game as {name}", {}},
      {"enabled", FieldKind::Bool, "Whether it can be chosen", {}},
      {"autofocus", FieldKind::Bool, "Whether it has the focus when its file is shown", {}}
    };
  }

  const std::string &UiCheckbox::GetText() const
  {
    return _text.Get();
  }

  // a switch

  float UiToggle::BoxWidth() const
  {
    return 36.0f;
  }

  float UiToggle::BoxHeight() const
  {
    return 20.0f;
  }

  void UiToggle::ApplyPartDefaults(const std::string &part, UiStyle &style) const
  {
    UiCheckbox::ApplyPartDefaults(part, style);

    if (part == "track")
    {
      style.background_color = IsChecked() ? GetStyle().accent_color : Color{1.0f, 1.0f, 1.0f, 0.25f};
    } else if (part == "thumb")
    {
      style.background_color = {1.0f, 1.0f, 1.0f, 1.0f};
    }
  }

  bool UiToggle::UsesDirection(const Key direction) const
  {
    return direction == Key::Left || direction == Key::Right;
  }

  void UiToggle::Interact(UiInteraction &interaction, const UiFrame &frame)
  {
    if (interaction.kind == UiInteraction::Kind::Direction)
    {
      // right is on, and left is off
      SetChecked(interaction.key == Key::Right);
      interaction.is_used = true;
      return;
    }

    UiCheckbox::Interact(interaction, frame);
  }

  void UiToggle::PaintBox(UiPainter &painter, const UiFrame &frame, const UiRectangle &box, const float opacity)
  {
    painter.FillRectangle(box, Faded(GetPartStyle("track").background_color, opacity));

    // the knob, with a rim of the track around it, at the side it is on
    const float rim = std::max(1.0f, std::round(2.0f * frame.scale));
    const float size = box.Height() - 2.0f * rim;
    const float left = IsChecked() ? box.right - rim - size : box.left + rim;

    painter.FillRectangle(
      {left, box.top + rim, left + size, box.top + rim + size},
      Faded(GetPartStyle("thumb").background_color, opacity));
  }
} // neon
