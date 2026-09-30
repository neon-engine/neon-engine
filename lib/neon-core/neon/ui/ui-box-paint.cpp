#include "ui-box-paint.hpp"

#include <algorithm>
#include <cmath>

namespace neon
{
  namespace
  {
    Color Faded(const Color &color, const float opacity)
    {
      return {color.r, color.g, color.b, color.a * opacity};
    }

    void Put(float to[4], const Color &color)
    {
      to[0] = color.r;
      to[1] = color.g;
      to[2] = color.b;
      to[3] = color.a;
    }

    UiRectangle Grown(const UiRectangle &box, const float by)
    {
      return {box.left - by, box.top - by, box.right + by, box.bottom + by};
    }
  }

  float ResolvePlace(const LayoutLength &place, const float room, const float scale)
  {
    if (place.unit == LayoutLength::Unit::Percent) { return room * place.value / 100.0f; }

    return place.value * scale;
  }

  void ResolveRadii(const UiCornerRadii &radii, const UiRectangle &box, const float scale, float pixels[4])
  {
    const float shorter = std::min(box.Width(), box.Height());

    const auto resolve = [shorter, scale](const LayoutLength &radius)
    {
      const float value = radius.unit == LayoutLength::Unit::Percent
        ? shorter * radius.value / 100.0f
        : radius.value * scale;
      return std::max(0.0f, value);
    };

    const float asked[4] = {
      resolve(radii.top_left), resolve(radii.top_right), resolve(radii.bottom_right), resolve(radii.bottom_left)
    };

    // fitted next to each other
    const Shape2D shape = BoxShape(box, asked, ShapeKind2D::Fill);
    for (int i = 0; i < 4; i++) { pixels[i] = shape.radii[i]; }
  }

  UiMatrix MatrixOf(const UiStyle &style, const UiRectangle &border_box, const float scale)
  {
    if (style.transform.empty()) { return {}; }

    // lengths are written in units of the file
    std::vector<UiTransformStep> steps = style.transform;
    for (auto &step : steps)
    {
      if (step.x.unit == LayoutLength::Unit::Pixels) { step.x.value *= scale; }
      if (step.y.unit == LayoutLength::Unit::Pixels) { step.y.value *= scale; }
    }

    const float origin_x = border_box.left + ResolvePlace(style.transform_origin.x, border_box.Width(), scale);
    const float origin_y = border_box.top + ResolvePlace(style.transform_origin.y, border_box.Height(), scale);

    return ToMatrix(steps, border_box.Width(), border_box.Height(), origin_x, origin_y);
  }

  UiBoxPaint::UiBoxPaint(
    const UiStyle &style,
    const UiRectangle &border_box,
    const UiRectangle &padding_box,
    const float scale)
  {
    _style = &style;
    _border_box = border_box;
    _padding_box = padding_box;
    _scale = scale;

    // the widths of the border follow from where the padding box landed,
    // so that the border meets it without a gap
    _widths[0] = std::max(0.0f, padding_box.top - border_box.top);
    _widths[1] = std::max(0.0f, border_box.right - padding_box.right);
    _widths[2] = std::max(0.0f, border_box.bottom - padding_box.bottom);
    _widths[3] = std::max(0.0f, padding_box.left - border_box.left);

    ResolveRadii(style.border_radius, border_box, scale, _radii);

    // The corners of the inside are those of the outside, less the border.
    // Where two sides differ in width the corner is that of the wider
    // side here, and the shader draws it as wide as each side asks for.
    _inner_radii[0] = std::max(0.0f, _radii[0] - std::max(_widths[3], _widths[0]));
    _inner_radii[1] = std::max(0.0f, _radii[1] - std::max(_widths[1], _widths[0]));
    _inner_radii[2] = std::max(0.0f, _radii[2] - std::max(_widths[1], _widths[2]));
    _inner_radii[3] = std::max(0.0f, _radii[3] - std::max(_widths[3], _widths[2]));
  }

  bool UiBoxPaint::IsRound() const
  {
    return _radii[0] > 0.0f || _radii[1] > 0.0f || _radii[2] > 0.0f || _radii[3] > 0.0f;
  }

  bool UiBoxPaint::IsPlain() const
  {
    return !IsRound() && _style->box_shadow.empty() && !_style->background_gradient.has_value() &&
           !_style->HasSideColors();
  }

  const float *UiBoxPaint::GetRadii() const
  {
    return _radii;
  }

  const float *UiBoxPaint::GetInnerRadii() const
  {
    return _inner_radii;
  }

  void UiBoxPaint::PaintShadows(UiPainter &painter, const float opacity, const bool inset) const
  {
    // the one that is written first is drawn last, and so lies on top
    for (std::size_t i = _style->box_shadow.size(); i > 0; i--)
    {
      const UiShadow &shadow = _style->box_shadow[i - 1];
      if (shadow.is_inset != inset) { continue; }

      const Color color = Faded(shadow.has_color ? shadow.color : _style->color, opacity);
      if (color.a <= 0.0f) { continue; }

      const float offset_x = shadow.offset_x * _scale;
      const float offset_y = shadow.offset_y * _scale;
      const float spread = shadow.spread * _scale;

      // the radius of CSS is twice the deviation of the blur
      const float deviation = shadow.blur * _scale / 2.0f;

      const UiRectangle &box = inset ? _padding_box : _border_box;
      if (box.IsEmpty()) { continue; }

      Shape2D shape = BoxShape(
        box, inset ? _inner_radii : _radii, inset ? ShapeKind2D::InsetShadow : ShapeKind2D::Shadow);
      shape.widths[0] = offset_x;
      shape.widths[1] = offset_y;
      shape.widths[2] = deviation;
      shape.widths[3] = spread;

      if (inset)
      {
        painter.FillShape(box, box, shape, color);
        continue;
      }

      // as far as the shadow can be seen: three deviations from its edge
      const float reach = std::ceil(std::max(0.0f, spread) + 3.0f * deviation + 1.0f);
      UiRectangle drawn = Grown(box, reach);
      drawn.left += std::min(offset_x, 0.0f);
      drawn.right += std::max(offset_x, 0.0f);
      drawn.top += std::min(offset_y, 0.0f);
      drawn.bottom += std::max(offset_y, 0.0f);

      painter.FillShape(drawn, box, shape, color);
    }
  }

  void UiBoxPaint::PaintBackground(UiPainter &painter, const float opacity) const
  {
    PaintShadows(painter, opacity, false);

    if (_border_box.IsEmpty()) { return; }

    if (const Color color = Faded(_style->background_color, opacity); color.a > 0.0f)
    {
      const Shape2D shape = BoxShape(_border_box, _radii, ShapeKind2D::Fill);
      painter.FillShape(_border_box, _border_box, shape, color);
    }

    if (_style->background_gradient.has_value())
    {
      Shape2D shape = BoxShape(_border_box, _radii, ShapeKind2D::Fill);
      SetGradient(shape, *_style->background_gradient, 1.0f);

      // the alpha of the corners fades the gradient
      painter.FillShape(_border_box, _border_box, shape, {1.0f, 1.0f, 1.0f, opacity});
    }
  }

  void UiBoxPaint::PaintBorder(UiPainter &painter, const float opacity) const
  {
    PaintShadows(painter, opacity, true);

    if (_widths[0] <= 0.0f && _widths[1] <= 0.0f && _widths[2] <= 0.0f && _widths[3] <= 0.0f) { return; }
    if (_border_box.IsEmpty()) { return; }

    const Color all = _style->BorderColor();

    Shape2D shape = BoxShape(_border_box, _radii, ShapeKind2D::Border);
    for (int i = 0; i < 4; i++) { shape.widths[i] = _widths[i]; }

    Put(shape.colors[0], _style->border_top_color.value_or(all));
    Put(shape.colors[1], _style->border_right_color.value_or(all));
    Put(shape.colors[2], _style->border_bottom_color.value_or(all));
    Put(shape.colors[3], _style->border_left_color.value_or(all));

    painter.FillShape(_border_box, _border_box, shape, {1.0f, 1.0f, 1.0f, opacity});
  }

  void UiBoxPaint::PaintImage(
    UiPainter &painter,
    const UiImage &image,
    const UiRectangle &place,
    const UiRectangle &part,
    const float opacity) const
  {
    if (!IsRound())
    {
      painter.DrawImage(image, place, part, {1.0f, 1.0f, 1.0f, opacity});
      return;
    }

    const Shape2D shape = BoxShape(_border_box, _radii, ShapeKind2D::Fill);
    painter.FillShape(place, _border_box, shape, image, part, {1.0f, 1.0f, 1.0f, opacity});
  }

  void UiBoxPaint::PaintOutline(
    UiPainter &painter,
    const float offset,
    const float width,
    const Color &color) const
  {
    const UiRectangle outer = Grown(_border_box, offset + width);
    if (outer.IsEmpty()) { return; }

    // a corner of the line is as much rounder as it lies further out, and
    // a corner that is square stays square
    float radii[4];
    for (int i = 0; i < 4; i++) { radii[i] = _radii[i] > 0.0f ? std::max(0.0f, _radii[i] + offset + width) : 0.0f; }

    Shape2D shape = BoxShape(outer, radii, ShapeKind2D::Border);
    for (int i = 0; i < 4; i++)
    {
      shape.widths[i] = width;
      Put(shape.colors[i], {color.r, color.g, color.b, 1.0f});
    }

    painter.FillShape(outer, outer, shape, {1.0f, 1.0f, 1.0f, color.a});
  }

  bool UiBoxPaint::Contains(const float x, const float y) const
  {
    if (!_border_box.Contains(x, y)) { return false; }
    if (!IsRound()) { return true; }

    return DistanceToRoundedBox(
             x - (_border_box.left + _border_box.right) / 2.0f,
             y - (_border_box.top + _border_box.bottom) / 2.0f,
             _border_box.Width() / 2.0f,
             _border_box.Height() / 2.0f,
             _radii) <= 0.0f;
  }

  bool UiBoxPaint::ContainsInPadding(const float x, const float y) const
  {
    if (!_padding_box.Contains(x, y)) { return false; }
    if (!IsRound()) { return true; }

    return DistanceToRoundedBox(
             x - (_padding_box.left + _padding_box.right) / 2.0f,
             y - (_padding_box.top + _padding_box.bottom) / 2.0f,
             _padding_box.Width() / 2.0f,
             _padding_box.Height() / 2.0f,
             _inner_radii) <= 0.0f;
  }
} // neon
