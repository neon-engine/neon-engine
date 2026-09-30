#include "ui-painter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace neon
{
  UiPainter::UiPainter(Render2DContext *renderer)
  {
    _renderer = renderer;
  }

  void UiPainter::Begin(const int frame_width, const int frame_height)
  {
    _batch = Triangles2D{};
    _clips.clear();
    _frame_width = frame_width;
    _frame_height = frame_height;
    _draw_calls = 0;
    _quads = 0;
  }

  void UiPainter::End()
  {
    if (!_batch.indices.empty())
    {
      _renderer->DrawTriangles(_batch);
      _draw_calls++;
    }

    _batch.vertices.clear();
    _batch.indices.clear();
    _batch.texture = No_Texture;
  }

  void UiPainter::SetClip()
  {
    const bool clipped = !_clips.empty();
    const ClipRectangle clip = clipped ? _clips.back() : ClipRectangle{};

    if (_batch.clipped == clipped && _batch.clip == clip) { return; }

    End();
    _batch.clipped = clipped;
    _batch.clip = clip;
  }

  void UiPainter::PushClip(const UiRectangle &rectangle)
  {
    int left = static_cast<int>(std::round(rectangle.left));
    int top = static_cast<int>(std::round(rectangle.top));
    int right = static_cast<int>(std::round(rectangle.right));
    int bottom = static_cast<int>(std::round(rectangle.bottom));

    if (!_clips.empty())
    {
      const ClipRectangle &outer = _clips.back();
      left = std::max(left, outer.x);
      top = std::max(top, outer.y);
      right = std::min(right, outer.x + outer.width);
      bottom = std::min(bottom, outer.y + outer.height);
    }

    _clips.push_back({left, top, std::max(0, right - left), std::max(0, bottom - top)});
  }

  void UiPainter::PopClip()
  {
    if (!_clips.empty()) { _clips.pop_back(); }
  }

  void UiPainter::Add(const Quad &quad, const int texture)
  {
    const UiRectangle &place = quad.place;
    if (place.IsEmpty() || quad.color.a <= 0.0f) { return; }

    // what cannot be seen is not handed to the renderer
    float left = 0.0f;
    float top = 0.0f;
    auto right = static_cast<float>(_frame_width);
    auto bottom = static_cast<float>(_frame_height);

    if (!_clips.empty())
    {
      const ClipRectangle &clip = _clips.back();
      left = static_cast<float>(clip.x);
      top = static_cast<float>(clip.y);
      right = static_cast<float>(clip.x + clip.width);
      bottom = static_cast<float>(clip.y + clip.height);
    }

    if (place.right <= left || place.left >= right || place.bottom <= top || place.top >= bottom) { return; }

    SetClip();

    if (quad.textured)
    {
      if (_batch.texture != No_Texture && _batch.texture != texture) { End(); }
      _batch.texture = texture;
    }

    // The corners from the left top one around to the left bottom one, and
    // the two triangles they make.
    const auto first = static_cast<std::uint32_t>(_batch.vertices.size());
    const float textured = quad.textured ? 1.0f : 0.0f;
    const UiRectangle &part = quad.part;

    _batch.vertices.push_back({place.left, place.top, part.left, part.top, quad.color, textured});
    _batch.vertices.push_back({place.right, place.top, part.right, part.top, quad.color, textured});
    _batch.vertices.push_back({place.right, place.bottom, part.right, part.bottom, quad.color, textured});
    _batch.vertices.push_back({place.left, place.bottom, part.left, part.bottom, quad.color, textured});

    for (const std::uint32_t corner : {0u, 1u, 2u, 0u, 2u, 3u}) { _batch.indices.push_back(first + corner); }

    _quads++;
  }

  void UiPainter::FillRectangle(const UiRectangle &rectangle, const Color &color)
  {
    Quad quad;
    quad.place = rectangle;
    quad.color = color;
    quad.textured = false;

    Add(quad, No_Texture);
  }

  void UiPainter::FillBorder(
    const UiRectangle &rectangle,
    const LayoutEdges<float> &widths,
    const Color &color)
  {
    const float inner_top = std::min(rectangle.top + widths.top, rectangle.bottom);
    const float inner_bottom = std::max(rectangle.bottom - widths.bottom, inner_top);

    // The top and the bottom run the whole width, and the sides lie
    // between them. No pixel is drawn twice, which would show when the
    // colour lets what is behind it through.
    FillRectangle({rectangle.left, rectangle.top, rectangle.right, inner_top}, color);
    FillRectangle({rectangle.left, inner_bottom, rectangle.right, rectangle.bottom}, color);
    FillRectangle(
      {rectangle.left, inner_top, std::min(rectangle.left + widths.left, rectangle.right), inner_bottom}, color);
    FillRectangle(
      {std::max(rectangle.right - widths.right, rectangle.left), inner_top, rectangle.right, inner_bottom},
      color);
  }

  void UiPainter::DrawImage(
    const UiImage &image,
    const UiRectangle &rectangle,
    const UiRectangle &part,
    const Color &tint)
  {
    if (image.texture == No_Texture) { return; }

    Quad quad;
    quad.place = rectangle;
    quad.part = part;
    quad.color = tint;
    quad.textured = true;

    Add(quad, image.texture);
  }

  void UiPainter::DrawNineSlice(
    const UiImage &image,
    const UiRectangle &rectangle,
    const LayoutEdges<float> &slice,
    const LayoutEdges<float> &widths,
    const Color &tint)
  {
    if (image.texture == No_Texture || image.width <= 0 || image.height <= 0 || rectangle.IsEmpty()) { return; }

    const auto image_width = static_cast<float>(image.width);
    const auto image_height = static_cast<float>(image.height);

    // how far the corners reach into the image, held to the image
    const float slice_left = std::clamp(slice.left, 0.0f, image_width);
    const float slice_right = std::clamp(slice.right, 0.0f, image_width - slice_left);
    const float slice_top = std::clamp(slice.top, 0.0f, image_height);
    const float slice_bottom = std::clamp(slice.bottom, 0.0f, image_height - slice_top);

    float left = std::max(0.0f, widths.left);
    float right = std::max(0.0f, widths.right);
    float top = std::max(0.0f, widths.top);
    float bottom = std::max(0.0f, widths.bottom);

    // corners that overlap are made smaller by the same factor, all four
    float factor = 1.0f;
    if (left + right > rectangle.Width()) { factor = std::min(factor, rectangle.Width() / (left + right)); }
    if (top + bottom > rectangle.Height()) { factor = std::min(factor, rectangle.Height() / (top + bottom)); }

    left = std::round(left * factor);
    right = std::round(right * factor);
    top = std::round(top * factor);
    bottom = std::round(bottom * factor);

    const float xs[4] = {rectangle.left, rectangle.left + left, rectangle.right - right, rectangle.right};
    const float ys[4] = {rectangle.top, rectangle.top + top, rectangle.bottom - bottom, rectangle.bottom};
    const float us[4] = {0.0f, slice_left / image_width, 1.0f - slice_right / image_width, 1.0f};
    const float vs[4] = {0.0f, slice_top / image_height, 1.0f - slice_bottom / image_height, 1.0f};

    for (int row = 0; row < 3; row++)
    {
      for (int column = 0; column < 3; column++)
      {
        DrawImage(
          image,
          {xs[column], ys[row], xs[column + 1], ys[row + 1]},
          {us[column], vs[row], us[column + 1], vs[row + 1]},
          tint);
      }
    }
  }

  void UiPainter::DrawText(
    const UiFont &font,
    const PlacedText &text,
    const float left,
    const float top,
    const Color &color)
  {
    if (font.texture == No_Texture) { return; }

    const auto atlas_width = static_cast<float>(font.atlas.GetWidth());
    const auto atlas_height = static_cast<float>(font.atlas.GetHeight());
    if (atlas_width <= 0.0f || atlas_height <= 0.0f) { return; }

    for (const auto &[glyph, x, y] : text.glyphs)
    {
      Quad quad;
      quad.place.left = left + x;
      quad.place.top = top + y;
      quad.place.right = quad.place.left + static_cast<float>(glyph->width);
      quad.place.bottom = quad.place.top + static_cast<float>(glyph->height);
      quad.part.left = static_cast<float>(glyph->x) / atlas_width;
      quad.part.top = static_cast<float>(glyph->y) / atlas_height;
      quad.part.right = static_cast<float>(glyph->x + glyph->width) / atlas_width;
      quad.part.bottom = static_cast<float>(glyph->y + glyph->height) / atlas_height;
      quad.color = color;
      quad.textured = true;

      Add(quad, font.texture);
    }
  }

  std::size_t UiPainter::GetDrawCalls() const
  {
    return _draw_calls;
  }

  std::size_t UiPainter::GetQuads() const
  {
    return _quads;
  }
} // neon
