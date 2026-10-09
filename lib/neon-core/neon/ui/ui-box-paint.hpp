#ifndef UI_BOX_PAINT_HPP
#define UI_BOX_PAINT_HPP

#include "ui-painter.hpp"
#include "ui-style.hpp"

namespace neon
{
  /// The box of an element as it is painted: where it is in pixels, how
  /// round its corners are, and what it is filled and framed with.
  ///
  /// Everything that has round corners, a gradient, a shadow, or sides of
  /// several colors is a shape that the shader works out for every pixel
  /// from the distance to its outline. It is sharp at every size, its edge
  /// is smoothed over one pixel, and it needs no texture. A box that has
  /// none of these is drawn as plain rectangles, as before.
  class UiBoxPaint
  {
    const UiStyle *_style;
    UiRectangle _border_box;
    UiRectangle _padding_box;
    float _scale = 1.0f;

    // in pixels, from the left top corner around to the left bottom one
    float _radii[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float _inner_radii[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    // top, right, bottom, left
    float _widths[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    void PaintShadows(UiPainter &painter, float opacity, bool inset) const;

  public:
    /// `border_box` and `padding_box` are in pixels. `scale` is pixels for
    /// each unit of the file.
    UiBoxPaint(const UiStyle &style, const UiRectangle &border_box, const UiRectangle &padding_box, float scale);

    /// Whether the box is drawn with rectangles alone.
    [[nodiscard]] bool IsPlain() const;

    [[nodiscard]] bool IsRound() const;

    /// The radius of each corner of the border box in pixels, and of the
    /// padding box.
    [[nodiscard]] const float *GetRadii() const;

    [[nodiscard]] const float *GetInnerRadii() const;

    /// The shadows around the box, its background, its gradient, the
    /// shadows that fall into it, and its border, in that order.
    void PaintBackground(UiPainter &painter, float opacity) const;

    void PaintBorder(UiPainter &painter, float opacity) const;

    /// An image over the border box, cut off at the round corners. `place`
    /// is where the image is drawn and `part` what of it, in parts of its
    /// size.
    void PaintImage(
      UiPainter &painter,
      const UiImage &image,
      const UiRectangle &place,
      const UiRectangle &part,
      float opacity) const;

    /// A line around the border box that follows its corners.
    void PaintOutline(UiPainter &painter, float offset, float width, const Color &color) const;

    /// Whether a point in pixels is inside the border box, its round
    /// corners taken into account. What the pointer is tested against.
    [[nodiscard]] bool Contains(float x, float y) const;

    [[nodiscard]] bool ContainsInPadding(float x, float y) const;
  };

  /// The radii of the corners of a box in pixels. A percentage is of the
  /// shorter side of the box. Corners that do not fit next to each other
  /// are made smaller by the same factor, as CSS does.
  void ResolveRadii(const UiCornerRadii &radii, const UiRectangle &box, float scale, float pixels[4]);

  /// What moves the points of an element to where they are drawn, in
  /// pixels. `border_box` is the box of the element before it is moved.
  [[nodiscard]] UiMatrix MatrixOf(const UiStyle &style, const UiRectangle &border_box, float scale);

  /// Where a place lies between two ends: a length from the start, or a
  /// percentage of the room that is left over.
  [[nodiscard]] float ResolvePlace(const LayoutLength &place, float room, float scale);
} // neon

#endif //UI_BOX_PAINT_HPP
