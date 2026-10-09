#ifndef IMAGE_PIXELS_HPP
#define IMAGE_PIXELS_HPP

#include <vector>

namespace neon
{
  /// An image in memory: red, green, blue, and alpha for each pixel, a byte
  /// each, row after row from the top.
  struct ImagePixels
  {
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;

    [[nodiscard]] bool IsEmpty() const
    {
      return width <= 0 || height <= 0 || pixels.empty();
    }
  };

  /// Multiplies alpha into the colors of every pixel, which is what
  /// blending and scaling expect. A color is rounded to the nearest, so
  /// that white at full alpha stays white.
  void PremultiplyAlpha(std::vector<unsigned char> &pixels);

  /// Smaller copies of an image, each half as wide and half as high as the
  /// one before, down to a single pixel. The image itself is the first.
  ///
  /// `image` has alpha multiplied into its colors, and so have the copies.
  /// A pixel of a copy is the mean of the pixels it stands for. With alpha
  /// multiplied in, a pixel that is see-through adds nothing to that mean
  /// but its share of the room: white next to nothing stays white, at half
  /// the alpha, and does not turn gray.
  [[nodiscard]] std::vector<ImagePixels> MakeSmallerCopies(const ImagePixels &image);

  /// Draws `part` of an image over `target` at a place, the way paint is
  /// put over paint. Both have alpha multiplied into their colors. For
  /// putting an image together from its parts, such as one that repeats.
  void CopyPixels(
    const ImagePixels &from,
    int from_x,
    int from_y,
    int width,
    int height,
    ImagePixels &target,
    int target_x,
    int target_y);
} // neon

#endif //IMAGE_PIXELS_HPP
