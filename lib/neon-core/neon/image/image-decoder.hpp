#ifndef IMAGE_DECODER_HPP
#define IMAGE_DECODER_HPP

#include <string>
#include <vector>

#include "image-pixels.hpp"

namespace neon
{
  /// Turns the bytes of an image file into pixels. An implementation knows
  /// the formats of one library.
  ///
  /// It is given the bytes of a file and reads no files itself. Those go
  /// through the file system.
  class ImageDecoder
  {
  protected:
    ~ImageDecoder() = default;

  public:
    /// Reads an image. The pixels are red, green, blue, and alpha, a byte
    /// each, row after row from the top, and alpha is not multiplied into
    /// the colours. Returns false and says why for what is no image the
    /// decoder knows.
    virtual bool Decode(const std::vector<unsigned char> &file, ImagePixels &image, std::string &error) = 0;
  };
} // neon

#endif //IMAGE_DECODER_HPP
