#include "ui-painter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace neon
{
  namespace
  {
    /// A part of an image as a part of its texture. They are the same but
    /// for an image that is a part of an atlas.
    UiRectangle PartOf(const UiImage &image, const UiRectangle &part)
    {
      const float width = image.part_right - image.part_left;
      const float height = image.part_bottom - image.part_top;

      return {
        image.part_left + part.left * width,
        image.part_top + part.top * height,
        image.part_left + part.right * width,
        image.part_top + part.bottom * height
      };
    }
  }

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
    _materials = 0;
    _transforms.clear();
    _rounded_clips.clear();
    _state = State{};
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
    _batch.shapes.clear();
  }

  void UiPainter::SetClip()
  {
    const bool clipped = !_clips.empty();
    const ClipRectangle clip = clipped ? _clips.back() : ClipRectangle{};

    const bool is_rounded = !_rounded_clips.empty();
    const RoundedClip2D rounded = is_rounded ? _rounded_clips.back().second : RoundedClip2D{};

    if (_batch.clipped == clipped && _batch.clip == clip &&
        _batch.has_rounded_clip == is_rounded && _batch.rounded_clip == rounded)
    {
      return;
    }

    End();
    _batch.clipped = clipped;
    _batch.clip = clip;
    _batch.has_rounded_clip = is_rounded;
    _batch.rounded_clip = rounded;
  }

  void UiPainter::SetState()
  {
    const auto same_values = [this]
    {
      if (_batch.material_values.size() != _state.material_values.size()) { return false; }

      for (std::size_t i = 0; i < _state.material_values.size(); i++)
      {
        const MaterialValue2D &a = _batch.material_values[i];
        const MaterialValue2D &b = _state.material_values[i];

        if (a.name != b.name || a.count != b.count ||
            std::memcmp(a.numbers, b.numbers, sizeof(a.numbers)) != 0)
        {
          return false;
        }
      }
      return true;
    };

    const float box[4] = {
      _state.material_box.left, _state.material_box.top, _state.material_box.Width(), _state.material_box.Height()
    };

    const bool same_material =
      _batch.material == _state.material &&
      (_state.material == No_Material ||
       (std::memcmp(_batch.material_box, box, sizeof(box)) == 0 && same_values()));

    if (_batch.filter == _state.filter && same_material && _batch.time == _time) { return; }

    End();
    _batch.filter = _state.filter;
    _batch.material = _state.material;
    _batch.material_values = _state.material == No_Material ? std::vector<MaterialValue2D>{} : _state.material_values;
    std::memcpy(_batch.material_box, box, sizeof(box));
    if (_state.material == No_Material) { std::memset(_batch.material_box, 0, sizeof(_batch.material_box)); }
    _batch.time = _time;
  }

  float UiPainter::PlaceOf(const Shape2D &shape)
  {
    if (!_batch.shapes.empty() && std::memcmp(&_batch.shapes.back(), &shape, sizeof(Shape2D)) == 0)
    {
      return static_cast<float>(_batch.shapes.size() - 1);
    }

    _batch.shapes.push_back(shape);
    return static_cast<float>(_batch.shapes.size() - 1);
  }

  void UiPainter::PushClip(const UiRectangle &rectangle)
  {
    UiRectangle moved = rectangle;

    if (!_transforms.empty() && !_transforms.back().IsIdentity())
    {
      // what holds the rectangle where it is drawn, since a clip has its
      // sides along those of the frame
      float xs[4] = {rectangle.left, rectangle.right, rectangle.right, rectangle.left};
      float ys[4] = {rectangle.top, rectangle.top, rectangle.bottom, rectangle.bottom};

      for (int i = 0; i < 4; i++) { _transforms.back().Apply(xs[i], ys[i]); }

      moved.left = std::min({xs[0], xs[1], xs[2], xs[3]});
      moved.right = std::max({xs[0], xs[1], xs[2], xs[3]});
      moved.top = std::min({ys[0], ys[1], ys[2], ys[3]});
      moved.bottom = std::max({ys[0], ys[1], ys[2], ys[3]});
    }

    int left = static_cast<int>(std::round(moved.left));
    int top = static_cast<int>(std::round(moved.top));
    int right = static_cast<int>(std::round(moved.right));
    int bottom = static_cast<int>(std::round(moved.bottom));

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
    if (place.IsEmpty()) { return; }

    if (quad.has_corner_colors)
    {
      if (quad.corners[0].a <= 0.0f && quad.corners[1].a <= 0.0f &&
          quad.corners[2].a <= 0.0f && quad.corners[3].a <= 0.0f)
      {
        return;
      }
    } else if (quad.color.a <= 0.0f && quad.shape == nullptr)
    {
      return;
    }

    // the corners from the left top one around to the left bottom one
    float xs[4] = {place.left, place.right, place.right, place.left};
    float ys[4] = {place.top, place.top, place.bottom, place.bottom};

    const bool is_moved = !_transforms.empty() && !_transforms.back().IsIdentity();
    if (is_moved)
    {
      for (int i = 0; i < 4; i++) { _transforms.back().Apply(xs[i], ys[i]); }
    }

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

    const float least_x = std::min({xs[0], xs[1], xs[2], xs[3]});
    const float most_x = std::max({xs[0], xs[1], xs[2], xs[3]});
    const float least_y = std::min({ys[0], ys[1], ys[2], ys[3]});
    const float most_y = std::max({ys[0], ys[1], ys[2], ys[3]});

    if (most_x <= left || least_x >= right || most_y <= top || least_y >= bottom) { return; }

    SetClip();
    SetState();

    if (quad.textured)
    {
      if (_batch.texture != No_Texture && _batch.texture != texture) { End(); }
      _batch.texture = texture;
    }

    const auto first = static_cast<std::uint32_t>(_batch.vertices.size());
    const float textured = quad.mode >= 0.0f ? quad.mode : (quad.textured ? 1.0f : 0.0f);
    const UiRectangle &part = quad.part;

    const float us[4] = {part.left, part.right, part.right, part.left};
    const float vs[4] = {part.top, part.top, part.bottom, part.bottom};

    float shape = -1.0f;
    float center_x = 0.0f;
    float center_y = 0.0f;

    if (quad.shape != nullptr)
    {
      shape = PlaceOf(*quad.shape);
      center_x = (quad.shape_box.left + quad.shape_box.right) / 2.0f;
      center_y = (quad.shape_box.top + quad.shape_box.bottom) / 2.0f;
    }

    // where a corner is in its shape is counted before it is moved, so
    // that a shape turns with its element
    const float local_xs[4] = {place.left, place.right, place.right, place.left};
    const float local_ys[4] = {place.top, place.top, place.bottom, place.bottom};

    for (int i = 0; i < 4; i++)
    {
      Vertex2D vertex;
      vertex.x = xs[i];
      vertex.y = ys[i];
      vertex.u = us[i];
      vertex.v = vs[i];
      vertex.color = quad.has_corner_colors ? quad.corners[i] : quad.color;
      vertex.textured = textured;
      vertex.shape = shape;
      vertex.local_x = quad.shape != nullptr ? local_xs[i] - center_x : 0.0f;
      vertex.local_y = quad.shape != nullptr ? local_ys[i] - center_y : 0.0f;

      _batch.vertices.push_back(vertex);
    }

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
    quad.part = PartOf(image, part);
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

  void UiPainter::SetTime(const float seconds)
  {
    _time = seconds;
  }

  void UiPainter::PushTransform(const UiMatrix &matrix)
  {
    // what is inside is moved first by its own matrix, and then by those
    // of what it is in
    _transforms.push_back(_transforms.empty() ? matrix : _transforms.back().After(matrix));
  }

  void UiPainter::PopTransform()
  {
    if (!_transforms.empty()) { _transforms.pop_back(); }
  }

  const UiMatrix &UiPainter::GetTransform() const
  {
    static const UiMatrix identity;
    return _transforms.empty() ? identity : _transforms.back();
  }

  void UiPainter::PushRoundedClip(const UiRectangle &rectangle, const float radii[4])
  {
    PushClip(rectangle);

    const UiMatrix &matrix = GetTransform();

    // A box that is moved or made larger keeps its corners. One that is
    // turned cannot be told to the renderer, which knows boxes whose
    // sides lie along those of the frame.
    const bool is_turned = matrix.b != 0.0f || matrix.c != 0.0f;

    RoundedClip2D clip;

    if (is_turned)
    {
      // as large as the frame, which cuts nothing off
      clip.center_x = 0.0f;
      clip.center_y = 0.0f;
      clip.half_width = 1e6f;
      clip.half_height = 1e6f;
    } else
    {
      float left = rectangle.left;
      float top = rectangle.top;
      float right = rectangle.right;
      float bottom = rectangle.bottom;
      matrix.Apply(left, top);
      matrix.Apply(right, bottom);

      clip.center_x = (left + right) / 2.0f;
      clip.center_y = (top + bottom) / 2.0f;
      clip.half_width = std::abs(right - left) / 2.0f;
      clip.half_height = std::abs(bottom - top) / 2.0f;

      const float scale = std::min(std::abs(matrix.a), std::abs(matrix.d));
      for (int i = 0; i < 4; i++) { clip.radii[i] = radii[i] * scale; }
    }

    _rounded_clips.emplace_back(_clips.size(), clip);
  }

  void UiPainter::PopRoundedClip()
  {
    if (!_rounded_clips.empty()) { _rounded_clips.pop_back(); }
    PopClip();
  }

  void UiPainter::SetFilter(const TextureFilter2D filter)
  {
    _state.filter = filter;
  }

  void UiPainter::SetMaterial(
    const int material,
    const std::vector<MaterialValue2D> &values,
    const UiRectangle &box)
  {
    _state.material = material;
    _state.material_values = values;
    _state.material_box = box;

    if (material != No_Material) { _materials++; }
  }

  bool UiPainter::UsedMaterials() const
  {
    return _materials > 0;
  }

  void UiPainter::FillShape(
    const UiRectangle &rectangle,
    const UiRectangle &box,
    const Shape2D &shape,
    const Color &color)
  {
    Quad quad;
    quad.place = rectangle;
    quad.color = color;
    quad.textured = false;
    quad.shape = &shape;
    quad.shape_box = box;

    Add(quad, No_Texture);
  }

  void UiPainter::FillShape(
    const UiRectangle &rectangle,
    const UiRectangle &box,
    const Shape2D &shape,
    const UiImage &image,
    const UiRectangle &part,
    const Color &tint)
  {
    if (image.texture == No_Texture) { return; }

    Quad quad;
    quad.place = rectangle;
    quad.part = PartOf(image, part);
    quad.color = tint;
    quad.textured = true;
    quad.shape = &shape;
    quad.shape_box = box;

    Add(quad, image.texture);
  }

  void UiPainter::FillRectangle(const UiRectangle &rectangle, const Color corners[4])
  {
    Quad quad;
    quad.place = rectangle;
    quad.textured = false;
    quad.has_corner_colors = true;
    for (int i = 0; i < 4; i++) { quad.corners[i] = corners[i]; }

    Add(quad, No_Texture);
  }

  void UiPainter::DrawGlyph(
    const int texture,
    const UiRectangle &place,
    const UiRectangle &part,
    const Color corners[4],
    const float mode,
    const Shape2D *shape,
    const UiRectangle &shape_box)
  {
    if (texture == No_Texture) { return; }

    Quad quad;
    quad.place = place;
    quad.part = part;
    quad.textured = true;
    quad.mode = mode;
    quad.has_corner_colors = true;
    for (int i = 0; i < 4; i++) { quad.corners[i] = corners[i]; }
    quad.shape = shape;
    quad.shape_box = shape_box;

    Add(quad, texture);
  }

  Shape2D BoxShape(const UiRectangle &box, const float radii[4], const ShapeKind2D kind)
  {
    Shape2D shape;
    shape.box[0] = box.Width() / 2.0f;
    shape.box[1] = box.Height() / 2.0f;
    shape.box[2] = static_cast<float>(kind);

    // Corners that do not fit next to each other are made smaller by the
    // same factor, all four, as CSS does.
    float factor = 1.0f;
    const auto fit = [&factor](const float a, const float b, const float side)
    {
      if (a + b > side && a + b > 0.0f) { factor = std::min(factor, side / (a + b)); }
    };

    fit(radii[0], radii[1], box.Width());
    fit(radii[3], radii[2], box.Width());
    fit(radii[0], radii[3], box.Height());
    fit(radii[1], radii[2], box.Height());

    for (int i = 0; i < 4; i++) { shape.radii[i] = std::max(0.0f, radii[i] * factor); }
    return shape;
  }

  void SetGradient(Shape2D &shape, const UiGradient &gradient, const float opacity)
  {
    const std::size_t count = std::min(gradient.stops.size(), UiGradient::kMax_Stops);

    shape.gradient[0] = gradient.kind == UiGradient::Kind::Linear ? 1.0f : 2.0f;
    shape.gradient[1] = gradient.angle * 3.14159265358979f / 180.0f;
    shape.gradient[2] = static_cast<float>(count);

    for (std::size_t i = 0; i < count; i++)
    {
      const UiGradientStop &stop = gradient.stops[i];
      shape.stop_positions[i] = stop.position;
      shape.stop_colors[i][0] = stop.color.r;
      shape.stop_colors[i][1] = stop.color.g;
      shape.stop_colors[i][2] = stop.color.b;
      shape.stop_colors[i][3] = stop.color.a * opacity;
    }
  }

  float DistanceToRoundedBox(
    const float x,
    const float y,
    const float half_width,
    const float half_height,
    const float radii[4])
  {
    // the corner of the quarter the point is in
    const float radius = x < 0.0f
      ? (y < 0.0f ? radii[0] : radii[3])
      : (y < 0.0f ? radii[1] : radii[2]);

    const float from_x = std::abs(x) - half_width + radius;
    const float from_y = std::abs(y) - half_height + radius;

    const float outside = std::hypot(std::max(from_x, 0.0f), std::max(from_y, 0.0f));
    const float inside = std::min(std::max(from_x, from_y), 0.0f);

    return outside + inside - radius;
  }
} // neon
