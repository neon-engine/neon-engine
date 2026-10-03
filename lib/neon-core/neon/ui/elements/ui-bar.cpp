#include "ui-bar.hpp"

#include <neon/ui/ui-fields.hpp>

#include <algorithm>
#include <cmath>
#include <format>

namespace neon
{
  bool UiBar::TellsWhenItChanged() const
  {
    return true;
  }

  bool UiBar::GetField(const std::string &name, FieldValue &value) const
  {
    if (name == "value")
    {
      value = _shown_value;
      return true;
    }

    if (name == "max")
    {
      value = _shown_max;
      return true;
    }

    return false;
  }

  bool UiBar::SetField(const std::string &name, const FieldValue &value, std::string &error)
  {
    if (name != "value" && name != "max") { return UiElement::SetField(name, value, error); }

    float number = 0.0f;
    if (!TakeNumber(value, "'" + name + "' of " + Describe(), number, error)) { return false; }

    // what is set holds, and no longer follows a value of the game
    (name == "value" ? _value : _max) = UiNumber(static_cast<double>(number));
    (name == "value" ? _shown_value : _shown_max) = number;

    const float filled = _shown_max > 0.0f ? _shown_value / _shown_max : 0.0f;
    _filled = std::isfinite(filled) ? std::clamp(filled, 0.0f, 1.0f) : 0.0f;

    Invalidate(UiDirty::Paint);
    return true;
  }

  std::vector<UiElement::Field> UiBar::GetFields() const
  {
    return {
      {"value", FieldKind::Float, "How much there is", {}},
      {"max", FieldKind::Float, "How much there is at the most", {}}
    };
  }

  void UiBar::ApplyDefaults(UiStyle &style) const
  {
    // the size a browser gives a progress bar: 10 by 1 of a font of 16
    style.layout.width = LayoutLength::Pixels(160.0f);
    style.layout.height = LayoutLength::Pixels(16.0f);
    style.layout.flex_shrink = 0.0f;

    style.background_color = {0.0f, 0.0f, 0.0f, 0.5f};
    style.pointer_events = UiPointerEvents::None;
  }

  void UiBar::ReadNumber(const DataReader &reader, const std::string &name, UiNumber &number)
  {
    const auto *value = reader.ReadValue(name);
    if (value == nullptr || UiNumber::Read(*value, number)) { return; }

    std::string text;
    reader.Report(*value, std::format(
                    "'{}' of {} is {}, where a number or a value such as \"{{health}}\" was expected",
                    name,
                    reader.GetWhere(),
                    value->GetText(text) ? "'" + text + "'" : DataValue::Describe(value->GetKind())));
  }

  void UiBar::ReadAttributes(const DataReader &reader)
  {
    ReadNumber(reader, "value", _value);
    ReadNumber(reader, "max", _max);
  }

  void UiBar::Update(const UiFrame &frame)
  {
    const double max = _max.Get(*frame.values, 1.0);
    const double value = _value.Get(*frame.values, 0.0);

    _shown_value = static_cast<float>(value);
    _shown_max = static_cast<float>(max);

    // a bar of nothing is empty, whatever its value
    const double filled = max > 0.0 ? value / max : 0.0;
    const float before = _filled;

    _filled = std::isfinite(filled) ? static_cast<float>(std::clamp(filled, 0.0, 1.0)) : 0.0f;

    if (_filled != before) { Invalidate(UiDirty::Paint); }
  }

  void UiBar::PaintContent(
    UiPainter &painter,
    const UiFrame &frame,
    const UiRectangle &content_box,
    const float opacity)
  {
    const Color &color = GetStyle().accent_color;

    painter.FillRectangle(
      {
        content_box.left,
        content_box.top,
        content_box.left + std::round(content_box.Width() * _filled),
        content_box.bottom
      },
      {color.r, color.g, color.b, color.a * opacity});
  }

  float UiBar::GetFilled() const
  {
    return _filled;
  }
} // neon
