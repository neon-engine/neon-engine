#include "stb-image-decoder.hpp"

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <vector>

#include <gtest/gtest.h>

// An image of PNG and one of JPEG, written into this header by the build
#include <test-images.hpp>

namespace
{
  using neon::ImagePixels;
  using neon::STB_ImageDecoder;

  using Bytes = std::vector<unsigned char>;

  void Put16(Bytes &bytes, const unsigned int value, const bool big_end_first)
  {
    const auto low = static_cast<unsigned char>(value & 0xFF);
    const auto high = static_cast<unsigned char>((value >> 8) & 0xFF);

    bytes.push_back(big_end_first ? high : low);
    bytes.push_back(big_end_first ? low : high);
  }

  void Put32(Bytes &bytes, const std::uint32_t value, const bool big_end_first)
  {
    for (int i = 0; i < 4; i++)
    {
      const int shift = big_end_first ? 24 - 8 * i : 8 * i;
      bytes.push_back(static_cast<unsigned char>((value >> shift) & 0xFF));
    }
  }

  /// Two pixels side by side: red, and blue at half its alpha.
  constexpr unsigned char two_pixels[8] = {255, 0, 0, 255, 0, 0, 255, 128};

  /// A BMP of 32 bits without compression, which counts its rows from the
  /// bottom.
  Bytes Bmp()
  {
    Bytes bytes = {'B', 'M'};
    Put32(bytes, 14 + 108 + 8, false);
    Put32(bytes, 0, false);
    Put32(bytes, 14 + 108, false);

    // the header of version 4, which names where alpha is kept
    Put32(bytes, 108, false);
    Put32(bytes, 2, false);
    Put32(bytes, 1, false);
    Put16(bytes, 1, false);
    Put16(bytes, 32, false);
    Put32(bytes, 3, false); // what each channel is kept in is named
    Put32(bytes, 8, false);
    Put32(bytes, 2835, false);
    Put32(bytes, 2835, false);
    Put32(bytes, 0, false);
    Put32(bytes, 0, false);
    Put32(bytes, 0x00FF0000, false); // red
    Put32(bytes, 0x0000FF00, false); // green
    Put32(bytes, 0x000000FF, false); // blue
    Put32(bytes, 0xFF000000, false); // alpha
    while (bytes.size() < 14 + 108) { bytes.push_back(0); }

    // blue, green, red, alpha
    for (const unsigned char byte : {0, 0, 255, 255, 255, 0, 0, 128}) { bytes.push_back(byte); }
    return bytes;
  }

  /// A TGA of 32 bits without compression, with its rows from the top.
  Bytes Tga()
  {
    Bytes bytes = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    Put16(bytes, 2, false);
    Put16(bytes, 1, false);
    bytes.push_back(32);
    bytes.push_back(0x28); // 8 bits of alpha, rows from the top

    for (const unsigned char byte : {0, 0, 255, 255, 255, 0, 0, 128}) { bytes.push_back(byte); }
    return bytes;
  }

  /// A PSD as Photoshop writes one that has nothing but the picture of all
  /// its layers put together. Every channel is kept on its own.
  Bytes Psd(const int depth, const int channels = 4)
  {
    Bytes bytes = {'8', 'B', 'P', 'S'};
    Put16(bytes, 1, true);
    for (int i = 0; i < 6; i++) { bytes.push_back(0); }
    Put16(bytes, static_cast<unsigned int>(channels), true);
    Put32(bytes, 1, true); // height
    Put32(bytes, 2, true); // width
    Put16(bytes, static_cast<unsigned int>(depth), true);
    Put16(bytes, 3, true); // RGB

    Put32(bytes, 0, true); // no palette
    Put32(bytes, 0, true); // no resources
    Put32(bytes, 0, true); // no layers
    Put16(bytes, 0, true); // no compression

    for (int channel = 0; channel < channels; channel++)
    {
      for (int pixel = 0; pixel < 2; pixel++)
      {
        unsigned char value = two_pixels[pixel * 4 + channel];

        // Photoshop keeps the colours of a picture with alpha blended over
        // white, and a reader takes the white off again. A colour kept as
        // it is would be darker than any blend over white can be, which
        // stb_image turns into a float below zero and then into a byte,
        // and what that gives is not defined: 0 without the optimiser, 3
        // with it.
        if (channels == 4 && channel < 3)
        {
          const int alpha = two_pixels[pixel * 4 + 3];
          value = static_cast<unsigned char>((value * alpha + 255 * (255 - alpha) + 127) / 255);
        }

        bytes.push_back(value);
        // 16 bits repeat the byte, so that 255 is 65535
        if (depth == 16) { bytes.push_back(value); }
      }
    }

    return bytes;
  }

  class StbImageDecoderTest : public ::testing::Test
  {
  protected:
    STB_ImageDecoder _decoder;
    ImagePixels _image;
    std::string _error;

    void ExpectTwoPixels(const Bytes &file, const char *format)
    {
      ASSERT_TRUE(_decoder.Decode(file, _image, _error)) << format << ": " << _error;

      EXPECT_EQ(_image.width, 2) << format;
      EXPECT_EQ(_image.height, 1) << format;
      EXPECT_EQ(_image.pixels, Bytes(std::begin(two_pixels), std::end(two_pixels))) << format;
    }
  };

  TEST_F(StbImageDecoderTest, ReadsPng)
  {
    ASSERT_TRUE(_decoder.Decode({std::begin(png_image), std::end(png_image)}, _image, _error)) << _error;

    EXPECT_GT(_image.width, 0);
    EXPECT_GT(_image.height, 0);
    EXPECT_EQ(_image.pixels.size(), static_cast<std::size_t>(_image.width) * _image.height * 4);

    // the corner of the heart is see-through, and its colour is kept
    EXPECT_EQ(_image.pixels[3], 0);
  }

  TEST_F(StbImageDecoderTest, ReadsJpeg)
  {
    ASSERT_TRUE(_decoder.Decode({std::begin(jpeg_image), std::end(jpeg_image)}, _image, _error)) << _error;

    EXPECT_GT(_image.width, 0);
    EXPECT_EQ(_image.pixels.size(), static_cast<std::size_t>(_image.width) * _image.height * 4);
    EXPECT_EQ(_image.pixels[3], 255) << "a JPEG covers everything";
  }

  TEST_F(StbImageDecoderTest, ReadsBmpAndTga)
  {
    ExpectTwoPixels(Bmp(), "BMP");
    ExpectTwoPixels(Tga(), "TGA");
  }

  TEST_F(StbImageDecoderTest, ReadsThePictureOfAPsdWith8And16Bits)
  {
    ExpectTwoPixels(Psd(8), "PSD with 8 bits");
    ExpectTwoPixels(Psd(16), "PSD with 16 bits");
  }

  TEST_F(StbImageDecoderTest, APsdWithoutAlphaCoversEverything)
  {
    ASSERT_TRUE(_decoder.Decode(Psd(8, 3), _image, _error)) << _error;

    EXPECT_EQ(_image.pixels, (Bytes{255, 0, 0, 255, 0, 0, 255, 255}));
  }

  TEST_F(StbImageDecoderTest, RefusesAPsdThatIsNotRgb)
  {
    Bytes cmyk = Psd(8);
    cmyk[25] = 4;

    EXPECT_FALSE(_decoder.Decode(cmyk, _image, _error));
    EXPECT_FALSE(_error.empty());
  }

  TEST_F(StbImageDecoderTest, RefusesWhatIsNoImage)
  {
    EXPECT_FALSE(_decoder.Decode({}, _image, _error));
    EXPECT_EQ(_error, "the file is empty");

    EXPECT_FALSE(_decoder.Decode({'n', 'o', 't', ' ', 'a', 'n', ' ', 'i', 'm', 'a', 'g', 'e'}, _image, _error));
    EXPECT_EQ(_error, "it is none of PNG, JPEG, TGA, BMP, PSD, and GIF, or it is damaged");

    EXPECT_TRUE(_image.IsEmpty()) << "and hands over nothing";
  }

  TEST_F(StbImageDecoderTest, RefusesAnImageThatIsCutOff)
  {
    Bytes png(std::begin(png_image), std::end(png_image));
    png.resize(png.size() / 2);

    EXPECT_FALSE(_decoder.Decode(png, _image, _error));
    EXPECT_FALSE(_error.empty());
  }

  TEST_F(StbImageDecoderTest, RefusesAnImageThatIsTooLarge)
  {
    Bytes huge = Psd(8);

    // a width of 100000
    huge[18] = 0x00;
    huge[19] = 0x01;
    huge[20] = 0x86;
    huge[21] = 0xA0;

    EXPECT_FALSE(_decoder.Decode(huge, _image, _error));
    EXPECT_EQ(_error, "it is 100000 by 1, where up to 16384 by 16384 is read");
  }
}
