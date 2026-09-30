#ifndef UI_IMAGE_PAINT_HPP
#define UI_IMAGE_PAINT_HPP

#include "ui-box-paint.hpp"
#include "ui-element.hpp"

namespace neon
{
  /// How the textures of an element are read, which `image_rendering`
  /// says.
  [[nodiscard]] TextureFilter2D FilterOf(const UiStyle &style);

  /// Where an image is drawn in a box and what of it, as `object_fit` and
  /// `object_position` say. `width` and `height` are the size of the image
  /// in pixels of the frame. Nothing is drawn outside the box: what would
  /// stick out is left out of `part`, which is in parts of the image.
  void FitImage(
    const UiStyle &style,
    float width,
    float height,
    const UiRectangle &box,
    float scale,
    UiRectangle &place,
    UiRectangle &part);

  /// The image of `background_image` over the border box, as large as
  /// `background_size` says, where `background_position` says, and again
  /// and again as `background_repeat` says.
  void PaintBackgroundImage(
    UiPainter &painter,
    const UiFrame &frame,
    const UiStyle &style,
    const UiBoxPaint &box,
    const UiRectangle &border_box,
    float opacity,
    const std::string &element);

  /// An image in nine parts whose edges and whose middle are drawn again
  /// and again, as `border_image_repeat` says, and not stretched.
  void PaintNineSlice(
    UiPainter &painter,
    const UiImage &image,
    const UiRectangle &rectangle,
    const LayoutEdges<float> &slice,
    const LayoutEdges<float> &widths,
    UiBorderImageRepeat repeat,
    const Color &tint);

  /// The part of `place` that lies inside `box`, and what of the image
  /// that is.
  [[nodiscard]] bool CutToBox(const UiRectangle &box, UiRectangle &place, UiRectangle &part);
} // neon

#endif //UI_IMAGE_PAINT_HPP
