#include "ui-label.hpp"

namespace neon
{
  void UiLabel::ApplyDefaults(UiStyle &style) const
  {
    style.pointer_events = UiPointerEvents::None;
  }

  void UiLabel::ReadAttributes(const DataReader &reader)
  {
    _text.Read(reader);
  }

  bool UiLabel::HasContent() const
  {
    return true;
  }

  LayoutSize UiLabel::Measure(const UiFrame &frame, const float available_width, float available_height)
  {
    return _text.Measure(GetStyle(), frame, available_width);
  }

  void UiLabel::Update(const UiFrame &frame)
  {
    _text.Update(frame);
  }

  void UiLabel::PaintContent(
    UiPainter &painter,
    const UiFrame &frame,
    const UiRectangle &content_box,
    const float opacity)
  {
    _text.Paint(painter, GetStyle(), frame, content_box, opacity, false);
  }

  const std::string &UiLabel::GetText() const
  {
    return _text.Get();
  }
} // neon
