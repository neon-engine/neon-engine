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

    const std::string text = _template.Format(*frame.values);

    // glyphs are made again for another text alone, and not whenever a
    // value changes that the text does not show
    if (!_is_made || text != _text)
    {
      _text = text;
      _characters = DecodeUtf8(_text);
      _text_count++;
    }

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

  const UiText::Shaped *UiText::Shape(const UiStyle &style, const UiFrame &frame) const
  {
    const int pixel_size = PixelSizeOf(style, frame);

    ShapingStyle shaping;
    shaping.direction = style.direction;
    shaping.letter_spacing = style.letter_spacing * frame.scale;
    shaping.word_spacing = style.word_spacing * frame.scale;
    shaping.white_space = style.white_space;
    shaping.transform = style.text_transform;

    const std::uint64_t fonts_revision = frame.resources->GetFontsRevision();

    const bool is_current =
      _shaped.is_made && _shaped.made_from == _text_count && _shaped.fonts_revision == fonts_revision &&
      _shaped.pixel_size == pixel_size && _shaped.weight == style.font_weight &&
      _shaped.is_italic == style.font_italic && _shaped.style == shaping &&
      _shaped.families == style.font_family;

    if (!is_current)
    {
      _shaped.is_made = true;
      _shaped.made_from = _text_count;
      _shaped.fonts_revision = fonts_revision;
      _shaped.families = style.font_family;
      _shaped.weight = style.font_weight;
      _shaped.is_italic = style.font_italic;
      _shaped.pixel_size = pixel_size;
      _shaped.style = shaping;

      _shaped.fonts = frame.resources->GetTextFonts(
        style.font_family, style.font_weight, style.font_italic, pixel_size);

      _shaped.text = _shaped.fonts != nullptr
        ? ShapeText(_characters, _shaped.fonts->fonts, shaping)
        : ShapedText{};

      _shaped_count++;
      _measured.is_made = false;
      _painted.is_made = false;
    }

    return _shaped.fonts != nullptr ? &_shaped : nullptr;
  }

  const PlacedShapedText &UiText::Place(
    Placed &placed,
    const Shaped &shaped,
    const PlacingOptions &options) const
  {
    const auto same = [](const float a, const float b)
    {
      return a == b || (std::isnan(a) && std::isnan(b));
    };

    if (placed.is_made && placed.shaped == _shaped_count &&
        same(placed.max_width, options.max_width) && same(placed.box_width, options.box_width) &&
        placed.align == options.align && placed.line_height == options.line_height &&
        placed.overflow == options.overflow)
    {
      return placed.text;
    }

    placed.is_made = true;
    placed.shaped = _shaped_count;
    placed.max_width = options.max_width;
    placed.box_width = options.box_width;
    placed.align = options.align;
    placed.line_height = options.line_height;
    placed.overflow = options.overflow;
    placed.text = PlaceShapedText(shaped.text, shaped.fonts->fonts, options);
    return placed.text;
  }

  LayoutSize UiText::Measure(const UiStyle &style, const UiFrame &frame, const float available_width) const
  {
    if (_characters.empty()) { return {}; }

    const Shaped *shaped = Shape(style, frame);
    if (shaped == nullptr) { return {}; }

    PlacingOptions options;
    options.align = style.text_align;
    options.line_height = style.LineHeight() * frame.scale;

    const bool breaks = style.white_space == WhiteSpace::Normal || style.white_space == WhiteSpace::PreWrap ||
                        style.white_space == WhiteSpace::PreLine;
    if (breaks && !std::isnan(available_width)) { options.max_width = available_width * frame.scale; }

    const PlacedShapedText &placed = Place(_measured, *shaped, options);

    // Rounded up to the next pixel, so that the box is never a part of a
    // pixel too small for the text that was measured.
    return {std::ceil(placed.width) / frame.scale, placed.height / frame.scale};
  }

  namespace
  {
    Color Faded(const Color &color, const float opacity)
    {
      return {color.r, color.g, color.b, color.a * opacity};
    }

    /// Where a point lies along a gradient that runs over a box, from 0 to
    /// 1.
    float AlongGradient(const UiGradient &gradient, const UiRectangle &box, const float x, const float y)
    {
      const float center_x = (box.left + box.right) / 2.0f;
      const float center_y = (box.top + box.bottom) / 2.0f;

      if (gradient.kind == UiGradient::Kind::Radial)
      {
        // an ellipse that reaches the corners of the box
        const float reach_x = std::max(box.Width() / 2.0f, 0.001f) * 1.41421356f;
        const float reach_y = std::max(box.Height() / 2.0f, 0.001f) * 1.41421356f;
        return std::hypot((x - center_x) / reach_x, (y - center_y) / reach_y);
      }

      const float radians = gradient.angle * 3.14159265358979f / 180.0f;
      const float along_x = std::sin(radians);
      const float along_y = -std::cos(radians);

      // as long as it takes for the corners to have the colours of the
      // ends
      const float length = std::abs(box.Width() * along_x) + std::abs(box.Height() * along_y);
      if (length <= 0.0f) { return 0.0f; }

      return ((x - center_x) * along_x + (y - center_y) * along_y) / length + 0.5f;
    }
  }

  void UiText::Paint(
    UiPainter &painter,
    const UiStyle &style,
    const UiFrame &frame,
    const UiRectangle &content_box,
    const float opacity,
    const bool centered) const
  {
    if (_characters.empty()) { return; }

    const Shaped *shaped = Shape(style, frame);
    if (shaped == nullptr) { return; }

    const UiTextFonts &fonts = *shaped->fonts;

    PlacingOptions options;
    options.align = style.text_align;
    options.line_height = style.LineHeight() * frame.scale;
    options.box_width = content_box.Width();

    const bool breaks = style.white_space == WhiteSpace::Normal || style.white_space == WhiteSpace::PreWrap ||
                        style.white_space == WhiteSpace::PreLine;

    // half a pixel of room, since the box went through the units of the
    // file and back
    if (breaks) { options.max_width = content_box.Width() + 0.5f; }
    if (!breaks) { options.overflow = style.text_overflow; }

    const PlacedShapedText &placed = Place(_painted, *shaped, options);

    const float left = content_box.left;
    float top = content_box.top;
    if (centered) { top += std::round((content_box.Height() - placed.height) / 2.0f); }

    const UiRectangle text_box{left, top, left + std::max(placed.width, content_box.Width()), top + placed.height};

    // Draws every glyph of the text once: moved, in a colour, and with
    // what is done to its picture.
    const auto draw = [&](
      const float move_x,
      const float move_y,
      const Color &color,
      const UiGradient *gradient,
      const GlyphEffect &effect,
      const Shape2D *shape)
    {
      for (const PlacedShapedGlyph &glyph : placed.glyphs)
      {
        if (glyph.font >= fonts.fonts.size()) { continue; }

        const TextFont &font = fonts.fonts[glyph.font];
        UiGlyphFont &textures = *fonts.glyphs[glyph.font];

        const CachedGlyph *picture = effect.kind == GlyphEffect::Kind::None
          ? glyph.picture
          : font.atlas->Find(glyph.glyph, glyph.variant, effect);

        if (picture == nullptr || picture->width <= 0 || picture->height <= 0) { continue; }

        const int texture = frame.resources->GetGlyphTexture(textures, picture->page);
        if (texture == No_Texture) { continue; }

        const GlyphPage &page = font.atlas->GetPage(picture->page);
        const auto page_width = static_cast<float>(page.width);
        const auto page_height = static_cast<float>(page.height);

        UiRectangle place;
        place.left = left + move_x + glyph.origin_x + static_cast<float>(picture->left) * font.scale;
        place.top = top + move_y + glyph.origin_y - static_cast<float>(picture->top) * font.scale;
        place.right = place.left + static_cast<float>(picture->width) * font.scale;
        place.bottom = place.top + static_cast<float>(picture->height) * font.scale;

        const UiRectangle part{
          static_cast<float>(picture->x) / page_width,
          static_cast<float>(picture->y) / page_height,
          static_cast<float>(picture->x + picture->width) / page_width,
          static_cast<float>(picture->y + picture->height) / page_height
        };

        Color corners[4];

        if (picture->has_colors)
        {
          // a glyph with colours of its own keeps them
          for (auto &corner : corners) { corner = {1.0f, 1.0f, 1.0f, opacity}; }
        } else if (gradient != nullptr)
        {
          const float xs[4] = {place.left, place.right, place.right, place.left};
          const float ys[4] = {place.top, place.top, place.bottom, place.bottom};

          for (int i = 0; i < 4; i++)
          {
            corners[i] = Faded(gradient->At(AlongGradient(*gradient, text_box, xs[i], ys[i])), opacity);
          }
        } else
        {
          for (auto &corner : corners) { corner = color; }
        }

        painter.DrawGlyph(
          texture,
          place,
          part,
          corners,
          font.atlas->IsDistanceField() ? 2.0f : 1.0f,
          font.atlas->IsDistanceField() ? shape : nullptr,
          text_box);
      }
    };

    const bool is_distance_field = fonts.fonts.front().atlas->IsDistanceField();
    const float scale_of_field = fonts.fonts.front().scale;
    const float range = fonts.fonts.front().atlas->GetDistanceRange();

    // what a shader does to glyphs that are kept as distances
    const auto field_shape = [&](const float stroke, const float blur, const Color &stroke_color)
    {
      Shape2D shape;
      shape.box[0] = text_box.Width() / 2.0f;
      shape.box[1] = text_box.Height() / 2.0f;
      shape.box[2] = static_cast<float>(ShapeKind2D::Text);
      shape.widths[0] = stroke;
      shape.widths[1] = blur;
      shape.widths[2] = range * scale_of_field;
      shape.colors[0][0] = stroke_color.r;
      shape.colors[0][1] = stroke_color.g;
      shape.colors[0][2] = stroke_color.b;
      shape.colors[0][3] = stroke_color.a;
      return shape;
    };

    const Color fill = Faded(style.color, opacity);

    // the shadows, the one that is written first on top
    for (std::size_t i = style.text_shadow.size(); i > 0; i--)
    {
      const UiShadow &shadow = style.text_shadow[i - 1];
      const Color color = Faded(shadow.has_color ? shadow.color : style.color, opacity);
      const float blur = shadow.blur * frame.scale;

      const float move_x = std::round(shadow.offset_x * frame.scale);
      const float move_y = std::round(shadow.offset_y * frame.scale);

      if (is_distance_field)
      {
        const Shape2D shape = field_shape(0.0f, blur, {0.0f, 0.0f, 0.0f, 0.0f});
        draw(move_x, move_y, color, nullptr, {}, &shape);
      } else
      {
        draw(move_x, move_y, color, nullptr, {GlyphEffect::Kind::Blur, blur}, nullptr);
      }
    }

    // the lines of the text. The one under it lies below the glyphs, the
    // one through it above them
    const FontMetrics &metrics = fonts.fonts.front().atlas->GetMetrics();
    const auto pixel_size = static_cast<float>(shaped->pixel_size);

    float underline_position = pixel_size * 0.1f;
    float thickness = std::max(1.0f, pixel_size / 14.0f);

    if (const TextFont &first = fonts.fonts.front(); first.rasterizer != nullptr)
    {
      (void) first.rasterizer->GetUnderline(first.font, pixel_size, underline_position, thickness);
    }

    if (style.text_decoration_thickness > 0.0f) { thickness = style.text_decoration_thickness * frame.scale; }
    thickness = std::max(1.0f, std::round(thickness));

    const Color line_color = Faded(style.text_decoration_color.value_or(style.color), opacity);

    const auto draw_line = [&](const PlacedLine &line, const float line_top)
    {
      if (line.width <= 0.0f) { return; }

      const float y = std::round(top + line.baseline + line_top);
      painter.FillRectangle(
        {left + line.left, y, left + line.left + std::ceil(line.width), y + thickness}, line_color);
    };

    if (style.text_underline)
    {
      for (const PlacedLine &line : placed.lines) { draw_line(line, underline_position); }
    }

    const UiGradient *gradient = style.color_gradient.has_value() ? &*style.color_gradient : nullptr;
    const float stroke = style.text_stroke_width * frame.scale;
    const Color stroke_color = Faded(style.text_stroke_color.value_or(style.color), opacity);

    if (is_distance_field)
    {
      // the glyph and the line around it in one go
      const Shape2D shape = field_shape(stroke, 0.0f, stroke_color);
      draw(0.0f, 0.0f, fill, gradient, {}, &shape);
    } else
    {
      draw(0.0f, 0.0f, fill, gradient, {}, nullptr);

      if (stroke > 0.0f)
      {
        draw(0.0f, 0.0f, stroke_color, nullptr, {GlyphEffect::Kind::Stroke, stroke}, nullptr);
      }
    }

    if (style.text_line_through)
    {
      // through the middle of the small letters
      const float through = -(metrics.ascent * fonts.fonts.front().scale) * 0.3f;
      for (const PlacedLine &line : placed.lines) { draw_line(line, through - thickness / 2.0f); }
    }
  }
} // neon
