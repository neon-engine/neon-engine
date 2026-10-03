#include "ui-label.hpp"

#include <neon/ui/ui-fields.hpp>

namespace neon
{
  bool UiLabel::TellsWhenItChanged() const
  {
    return true;
  }

  bool UiLabel::GetField(const std::string &name, FieldValue &value) const
  {
    if (name != "text") { return false; }

    value = _text.Get();
    return true;
  }

  bool UiLabel::SetField(const std::string &name, const FieldValue &value, std::string &error)
  {
    if (name != "text") { return UiElement::SetField(name, value, error); }

    std::string text;
    if (!TakeText(value, "'text' of " + Describe(), text, error)) { return false; }

    if (std::string problem; !_text.Set(text, problem))
    {
      error = "'text' of " + Describe() + " cannot be read: " + problem;
      return false;
    }

    Invalidate(UiDirty::Layout | UiDirty::Paint);
    return true;
  }

  std::vector<UiElement::Field> UiLabel::GetFields() const
  {
    return {{"text", FieldKind::String, "What is shown, which may refer to values of the game as {name}", {}}};
  }

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
    const std::string before = _text.Get();
    _text.Update(frame);

    // a text that changed may be as long as it was, and is drawn again
    // either way
    if (_text.Get() != before) { Invalidate(UiDirty::Layout | UiDirty::Paint); }
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
