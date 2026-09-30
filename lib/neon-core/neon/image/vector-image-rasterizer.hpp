#ifndef VECTOR_IMAGE_RASTERIZER_HPP
#define VECTOR_IMAGE_RASTERIZER_HPP

#include <string>
#include <vector>

#include "image-pixels.hpp"

namespace neon
{
  /// Turns an image that is made of shapes, such as an SVG, into pixels at
  /// the size it is asked for. Such an image is sharp at every size,
  /// because it is drawn again for every size and not scaled.
  ///
  /// It is given the bytes of a file and reads no files itself. Those go
  /// through the file system.
  class VectorImageRasterizer
  {
  protected:
    ~VectorImageRasterizer() = default;

  public:
    /// Returns what the image is known as from now on, or -1 and why when
    /// the bytes are no image it can draw.
    virtual int Load(const std::vector<unsigned char> &file, std::string &error) = 0;

    virtual void Unload(int image) = 0;

    /// The size the image says it has, in its own units, which count as
    /// units of a user interface. Returns false for an image that does
    /// not exist.
    virtual bool GetSize(int image, float &width, float &height) = 0;

    /// Draws the image so that it fills `width` by `height` pixels. The
    /// pixels are red, green, blue, and alpha, and alpha is not multiplied
    /// into the colours.
    virtual bool Rasterize(int image, int width, int height, ImagePixels &pixels) = 0;
  };
} // neon

#endif //VECTOR_IMAGE_RASTERIZER_HPP
