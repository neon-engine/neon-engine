#ifndef STB_IMAGE_DECODER_HPP
#define STB_IMAGE_DECODER_HPP

#include <neon/image/image-decoder.hpp>

namespace neon
{
  /// Reads images with stb_image.
  ///
  /// | Format | What is read |
  /// |---|---|
  /// | PNG | 8 and 16 bits for each channel, with and without alpha, with a palette |
  /// | JPEG | Baseline and progressive. Not arithmetic coding, not 12 bits |
  /// | TGA | With and without compression |
  /// | BMP | Without compression, and with alpha |
  /// | PSD | The picture of all layers put together, which Photoshop writes into the file when it is saved with `Maximize Compatibility`. 8 and 16 bits for each channel, RGB alone. No layers, no effects, no text |
  /// | GIF | The first picture |
  ///
  /// 16 bits are brought down to 8. Anything else is refused.
  ///
  /// stb_image checks a file less than a library that is made for files
  /// from anywhere. An image has to come from where the assets of the game
  /// come from, and not from a player.
  // ReSharper disable once CppInconsistentNaming
  class STB_ImageDecoder final : public ImageDecoder
  {
  public:
    /// The largest image that is read, along each side.
    static constexpr int kMax_Size = 16384;

    bool Decode(const std::vector<unsigned char> &file, ImagePixels &image, std::string &error) override;
  };
} // neon

#endif //STB_IMAGE_DECODER_HPP
