#include "ui-bar.hpp"

#include <algorithm>
#include <cmath>
#include <format>

namespace neon
{
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

    // a bar of nothing is empty, whatever its value
    const double filled = max > 0.0 ? value / max : 0.0;
    _filled = std::isfinite(filled) ? static_cast<float>(std::clamp(filled, 0.0, 1.0)) : 0.0f;
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
