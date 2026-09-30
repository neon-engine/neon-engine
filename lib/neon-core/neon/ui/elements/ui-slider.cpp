#include "ui-slider.hpp"

#include <algorithm>
#include <cmath>
#include <format>

#include <neon/ui/ui-fields.hpp>

#include "ui-binding.hpp"

namespace neon
{
  namespace
  {
    constexpr float thumb_size = 16.0f;
    constexpr float track_height = 4.0f;

    Color Faded(const Color &color, const float opacity)
    {
      return {color.r, color.g, color.b, color.a * opacity};
    }

    /// Reads a number that may be written as text, for `min`, `max`, and
    /// `step`.
    bool ReadNumber(const DataReader &reader, const std::string &name, double &number)
    {
      const DataValue *value = reader.ReadValue(name);
      if (value == nullptr) { return false; }

      if (value->GetNumber(number)) { return true; }

      reader.Report(*value, std::format(
                      "'{}' of {} is {}, where a number was expected",
                      name, reader.GetWhere(), DataValue::Describe(value->GetKind())));
      return false;
    }
  }

  double UiSlider::Snapped(double value) const
  {
    if (_step > 0.0)
    {
      value = _min + std::round((value - _min) / _step) * _step;

      // what floating point leaves behind, such as 0.30000000000000004
      const double digits = std::pow(10.0, 9.0);
      value = std::round(value * digits) / digits;
    }

    return std::clamp(value, std::min(_min, _max), std::max(_min, _max));
  }

  double UiSlider::ValueAt(const float x) const
  {
    const UiRectangle &content = GetContentBox();
    const float room = std::max(1.0f, content.Width() - thumb_size);

    // the middle of the knob follows the pointer
    const float fraction = std::clamp((x - content.left - thumb_size / 2.0f) / room, 0.0f, 1.0f);
    return Snapped(_min + static_cast<double>(fraction) * (_max - _min));
  }

  void UiSlider::Change(const double value)
  {
    const double snapped = Snapped(value);
    if (snapped == _value) { return; }

    _value = snapped;
    Invalidate(UiDirty::Paint);

    const UiValue number = UiValue::Number(snapped);
    Notify("changed", number.AsText(), _binding, number);
  }

  void UiSlider::ApplyDefaults(UiStyle &style) const
  {
    style.pointer_events = UiPointerEvents::Auto;
    style.cursor = UiCursor::Pointer;
    style.layout.min_width = LayoutLength::Pixels(160.0f);
    style.layout.padding = {
      LayoutLength::Pixels(4.0f), LayoutLength::Pixels(0.0f), LayoutLength::Pixels(4.0f), LayoutLength::Pixels(0.0f)
    };
    style.layout.flex_shrink = 0.0f;
  }

  void UiSlider::ApplyStateDefaults(UiStyle &style, const UiStates &states) const
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

  void UiSlider::ApplyPartDefaults(const std::string &part, UiStyle &style) const
  {
    UiElement::ApplyPartDefaults(part, style);

    if (part == "track")
    {
      style.background_color = {1.0f, 1.0f, 1.0f, 0.25f};
    } else if (part == "fill")
    {
      style.background_color = GetStyle().accent_color;
    } else if (part == "thumb")
    {
      style.background_color = {1.0f, 1.0f, 1.0f, 1.0f};
    }
  }

  void UiSlider::ReadAttributes(const DataReader &reader)
  {
    reader.Read("autofocus", _autofocus);

    ReadNumber(reader, "min", _min);
    ReadNumber(reader, "max", _max);

    if (double step = 0.0; ReadNumber(reader, "step", step))
    {
      if (step > 0.0)
      {
        _step = step;
      } else
      {
        reader.Report(*reader.ReadValue("step"), std::format(
                        "'step' of {} is {}, where a number above 0 was expected", reader.GetWhere(), step));
      }
    }

    if (const auto *value = reader.ReadValue("value"); value != nullptr)
    {
      std::string text;
      double number = 0.0;

      if (value->GetNumber(number))
      {
        _value = number;
      } else if (UiBinding::Read(*value, text, _binding) && !_binding.empty())
      {
        _value = _min;
      } else
      {
        reader.Report(*value, std::format(
                        "'value' of {} is {}, where a number or a value such as \"{{volume}}\" was expected",
                        reader.GetWhere(),
                        value->GetText(text) ? "'" + text + "'" : DataValue::Describe(value->GetKind())));
      }
    } else
    {
      _value = _min;
    }

    _value = Snapped(_value);

    if (const auto *value = reader.ReadValue("enabled"); value != nullptr && !UiFlag::Read(*value, _enabled))
    {
      std::string text;
      reader.Report(*value, std::format(
                      "'enabled' of {} is {}, where true, false, or a value such as \"{{can_choose}}\" was expected",
                      reader.GetWhere(),
                      value->GetText(text) ? "'" + text + "'" : DataValue::Describe(value->GetKind())));
    }
  }

  bool UiSlider::IsFocusable() const
  {
    return true;
  }

  bool UiSlider::WantsFocus() const
  {
    return _autofocus;
  }

  bool UiSlider::IsEnabled() const
  {
    return _is_enabled;
  }

  bool UiSlider::UsesDirection(const Key direction) const
  {
    return direction == Key::Left || direction == Key::Right;
  }

  bool UiSlider::HasContent() const
  {
    return true;
  }

  bool UiSlider::TellsWhenItChanged() const
  {
    return true;
  }

  LayoutSize UiSlider::Measure(const UiFrame &frame, const float available_width, const float available_height)
  {
    (void) frame;
    (void) available_width;
    (void) available_height;
    return {0.0f, thumb_size};
  }

  void UiSlider::Update(const UiFrame &frame)
  {
    _is_enabled = _enabled.Get(*frame.values, true);

    if (_binding.empty()) { return; }
    if (_has_followed && _followed_revision == frame.values->GetRevision()) { return; }

    _has_followed = true;
    _followed_revision = frame.values->GetRevision();

    if (const UiValue *value = frame.values->Find(_binding); value != nullptr)
    {
      const double number = Snapped(value->AsNumber(_min));
      if (number != _value)
      {
        _value = number;
        Invalidate(UiDirty::Paint);
      }
    }
  }

  void UiSlider::Interact(UiInteraction &interaction, const UiFrame &frame)
  {
    (void) frame;

    switch (interaction.kind)
    {
      case UiInteraction::Kind::PointerDown:
        _is_dragging = true;
        Change(ValueAt(interaction.x));
        interaction.is_used = true;
        break;

      case UiInteraction::Kind::PointerMove:
        if (!_is_dragging) { break; }
        Change(ValueAt(interaction.x));
        interaction.is_used = true;
        break;

      case UiInteraction::Kind::PointerUp:
        _is_dragging = false;
        interaction.is_used = true;
        break;

      case UiInteraction::Kind::Direction:
        Change(_value + (interaction.key == Key::Right ? _step : -_step));
        interaction.is_used = true;
        break;

      case UiInteraction::Kind::KeyDown:
        switch (interaction.key)
        {
          case Key::Home:
            Change(_min);
            break;
          case Key::End:
            Change(_max);
            break;
          case Key::PageUp:
            Change(_value + 10.0 * _step);
            break;
          case Key::PageDown:
            Change(_value - 10.0 * _step);
            break;
          default:
            return;
        }
        interaction.is_used = true;
        break;

      case UiInteraction::Kind::Wheel:
        // a notch of the wheel is a step
        if (interaction.wheel_y == 0.0f) { break; }
        Change(_value + (interaction.wheel_y < 0.0f ? _step : -_step));
        interaction.is_used = true;
        break;

      case UiInteraction::Kind::Blur:
        _is_dragging = false;
        break;

      default:
        break;
    }
  }

  void UiSlider::PaintContent(
    UiPainter &painter,
    const UiFrame &frame,
    const UiRectangle &content_box,
    const float opacity)
  {
    const float thumb = std::round(thumb_size * frame.scale);
    const float track = std::max(1.0f, std::round(track_height * frame.scale));
    const float middle = std::round(content_box.top + content_box.Height() / 2.0f);

    // the knob moves between the ends, and the track runs under its middle
    const float room = std::max(0.0f, content_box.Width() - thumb);
    const float thumb_left = content_box.left + std::round(room * GetFraction());
    const float track_left = content_box.left + std::round(thumb / 2.0f);
    const float track_right = content_box.right - std::round(thumb / 2.0f);

    painter.FillRectangle(
      {track_left, middle - std::round(track / 2.0f), track_right, middle - std::round(track / 2.0f) + track},
      Faded(GetPartStyle("track").background_color, opacity));

    painter.FillRectangle(
      {track_left, middle - std::round(track / 2.0f), thumb_left + std::round(thumb / 2.0f), middle - std::round(track / 2.0f) + track},
      Faded(GetPartStyle("fill").background_color, opacity));

    painter.FillRectangle(
      {thumb_left, middle - std::round(thumb / 2.0f), thumb_left + thumb, middle - std::round(thumb / 2.0f) + thumb},
      Faded(GetPartStyle("thumb").background_color, opacity));
  }

  bool UiSlider::GetField(const std::string &name, FieldValue &value) const
  {
    if (name == "value")
    {
      value = static_cast<float>(_value);
      return true;
    }

    if (name == "min")
    {
      value = static_cast<float>(_min);
      return true;
    }

    if (name == "max")
    {
      value = static_cast<float>(_max);
      return true;
    }

    if (name == "step")
    {
      value = static_cast<float>(_step);
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

  bool UiSlider::SetField(const std::string &name, const FieldValue &value, std::string &error)
  {
    const std::string what = "'" + name + "' of " + Describe();

    if (name == "value")
    {
      float number = 0.0f;
      if (!TakeNumber(value, what, number, error)) { return false; }

      Change(number);
      return true;
    }

    if (name == "min" || name == "max" || name == "step")
    {
      float number = 0.0f;
      if (!TakeNumber(value, what, number, error)) { return false; }

      if (name == "step" && number <= 0.0f)
      {
        error = what + " has to be above 0";
        return false;
      }

      (name == "min" ? _min : name == "max" ? _max : _step) = number;

      // what was chosen may be outside of the ends now
      const double snapped = Snapped(_value);
      if (snapped != _value) { Change(snapped); }
      Invalidate(UiDirty::Paint);
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

  std::vector<UiElement::Field> UiSlider::GetFields() const
  {
    return {
      {"value", FieldKind::Number, "The number chosen, which may follow a value of the game as {name}", {}},
      {"min", FieldKind::Number, "The least it can be", {}},
      {"max", FieldKind::Number, "The most it can be", {}},
      {"step", FieldKind::Number, "What it moves by", {}},
      {"enabled", FieldKind::Bool, "Whether it can be moved", {}},
      {"autofocus", FieldKind::Bool, "Whether it has the focus when its file is shown", {}}
    };
  }

  double UiSlider::GetValue() const
  {
    return _value;
  }

  float UiSlider::GetFraction() const
  {
    if (_max == _min) { return 0.0f; }
    return std::clamp(static_cast<float>((_value - _min) / (_max - _min)), 0.0f, 1.0f);
  }
} // neon
