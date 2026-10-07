#ifndef IMAGE_HALVING_HPP
#define IMAGE_HALVING_HPP

#include "image-pixels.hpp"

namespace neon
{
  /// The image at half its size on each side, every pixel the average of
  /// the four it stands for: one level of the smaller copies a texture
  /// has, made before it is uploaded, see TextureScale. A side of an odd
  /// number of pixels loses its last row or column, as the graphics card's
  /// own halving does; a side of 1 stays 1.
  ///
  /// The pixels are RGBA, 4 bytes each. When the image holds colours, which
  /// a file keeps in sRGB, red, green, and blue are averaged as light, as
  /// the graphics card makes its smaller copies of colours; alpha, and the
  /// bytes of an image that holds numbers, are averaged as they are.
  [[nodiscard]] ImagePixels HalveImage(const ImagePixels &image, bool is_color);
} // neon

#endif //IMAGE_HALVING_HPP
