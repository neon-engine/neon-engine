#include "ui-text.hpp"

#include <algorithm>
#include <cmath>
#include <format>

#include <neon/text/utf8.hpp>

namespace neon
{
  namespace
  {
    /// The size of a font in pixels of the frame. It is a whole number, so
    /// that a user interface that is scaled asks for few sizes.
    int PixelSizeOf(const UiStyle &style, const UiFrame &frame)
    {
      return std::max(1, static_cast<int>(std::round(style.font_size * frame.scale)));
    }
  }

  void UiText::Read(const DataReader &reader)
  {
    const auto *value = reader.ReadValue("text");
    if (value == nullptr) { return; }

    // a number is text as well, so that `text: 75` needs no quotes
    std::string text;
    if (double number = 0.0; value->GetNumber(number))
    {
      text = UiValue::Number(number).AsText();
    } else if (!value->GetText(text))
    {
      reader.Report(*value, std::format(
                      "'text' of {} is {}, where text was expected",
                      reader.GetWhere(), DataValue::Describe(value->GetKind())));
      return;
    }

    if (std::string error; !UiTemplate::Parse(text, _template, error))
    {
      reader.Report(*value, std::format("'text' of {} cannot be read: {}", reader.GetWhere(), error));
      return;
    }

    _has_text = true;
    _is_made = false;
  }

  bool UiText::IsWritten() const
  {
    return _has_text;
  }

  const std::string &UiText::Get() const
  {
    return _text;
  }

  void UiText::Update(const UiFrame &frame)
  {
    if (_is_made && (!_template.HasValues() || _revision == frame.values->GetRevision())) { return; }

    _text = _template.Format(*frame.values);
    _characters = DecodeUtf8(_text);
    _revision = frame.values->GetRevision();
    _is_made = true;
  }

  const UiFont *UiText::FontOf(const UiStyle &style, const UiFrame &frame)
  {
    return frame.resources->GetFont(style.font_family, style.font_weight, PixelSizeOf(style, frame));
  }

  TextOptions UiText::OptionsOf(const UiStyle &style, const UiFrame &frame)
  {
    TextOptions options;
    options.align = style.text_align;
    options.line_height = style.LineHeight() * frame.scale;
    return options;
  }

  LayoutSize UiText::Measure(const UiStyle &style, const UiFrame &frame, const float available_width) const
  {
    const UiFont *font = FontOf(style, frame);
    if (font == nullptr || _characters.empty()) { return {}; }

    TextOptions options = OptionsOf(style, frame);
    if (!std::isnan(available_width)) { options.max_width = available_width * frame.scale; }

    const PlacedText placed = PlaceText(font->atlas, _characters, options);

    // Rounded up to the next pixel, so that the box is never a part of a
    // pixel too small for the text that was measured.
    return {std::ceil(placed.width) / frame.scale, placed.height / frame.scale};
  }

  void UiText::Paint(
    UiPainter &painter,
    const UiStyle &style,
    const UiFrame &frame,
    const UiRectangle &content_box,
    const float opacity,
    const bool centered) const
  {
    const UiFont *font = FontOf(style, frame);
    if (font == nullptr || _characters.empty()) { return; }

    TextOptions options = OptionsOf(style, frame);

    // half a pixel of room, since the box went through the units of the
    // file and back
    options.max_width = content_box.Width() + 0.5f;
    options.box_width = content_box.Width();

    const PlacedText placed = PlaceText(font->atlas, _characters, options);

    float top = content_box.top;
    if (centered) { top += std::round((content_box.Height() - placed.height) / 2.0f); }

    painter.DrawText(
      *font,
      placed,
      content_box.left,
      top,
      {style.color.r, style.color.g, style.color.b, style.color.a * opacity});
  }
} // neon
