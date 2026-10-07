#include "image-halving.hpp"

#include <gtest/gtest.h>

#include <neon/common/color-space.hpp>

namespace
{
  using neon::HalveImage;
  using neon::ImagePixels;

  ImagePixels Filled(const int width, const int height, const unsigned char r, const unsigned char g,
                     const unsigned char b, const unsigned char a)
  {
    ImagePixels image{.width = width, .height = height};
    for (int i = 0; i < width * height; i++) { image.pixels.insert(image.pixels.end(), {r, g, b, a}); }
    return image;
  }

  TEST(ImageHalvingTest, HalvesBothSides)
  {
    const ImagePixels half = HalveImage(Filled(8, 4, 10, 20, 30, 255), true);

    EXPECT_EQ(half.width, 4);
    EXPECT_EQ(half.height, 2);
    EXPECT_EQ(half.pixels.size(), 4u * 2u * 4u);
  }

  TEST(ImageHalvingTest, KeepsAnImageOfOneColourAsItIs)
  {
    const ImagePixels half = HalveImage(Filled(4, 4, 200, 100, 50, 128), true);

    for (std::size_t i = 0; i < half.pixels.size(); i += 4)
    {
      EXPECT_EQ(half.pixels[i], 200);
      EXPECT_EQ(half.pixels[i + 1], 100);
      EXPECT_EQ(half.pixels[i + 2], 50);
      EXPECT_EQ(half.pixels[i + 3], 128);
    }
  }

  TEST(ImageHalvingTest, AveragesColoursAsLight)
  {
    // black and white side by side, twice over: half the light, which in
    // sRGB is far brighter than the byte halfway
    ImagePixels image = Filled(2, 2, 0, 0, 0, 255);
    for (const std::size_t pixel : {1u, 2u}) { std::fill_n(image.pixels.begin() + pixel * 4, 3, 255); }

    const ImagePixels half = HalveImage(image, true);

    ASSERT_EQ(half.width, 1);
    EXPECT_EQ(half.pixels[0], neon::LinearToSrgbByte(0.5f));
    EXPECT_GT(half.pixels[0], 180);
    EXPECT_EQ(half.pixels[3], 255);
  }

  TEST(ImageHalvingTest, AveragesNumbersAsTheyAre)
  {
    ImagePixels image = Filled(2, 2, 0, 0, 0, 0);
    for (const std::size_t pixel : {1u, 2u}) { std::fill_n(image.pixels.begin() + pixel * 4, 4, 255); }

    const ImagePixels half = HalveImage(image, false);

    EXPECT_EQ(half.pixels[0], 128);
    EXPECT_EQ(half.pixels[3], 128) << "alpha as well";
  }

  TEST(ImageHalvingTest, ASideOfOneStaysOne)
  {
    const ImagePixels half = HalveImage(Filled(8, 1, 1, 2, 3, 4), false);

    EXPECT_EQ(half.width, 4);
    EXPECT_EQ(half.height, 1);
  }

  TEST(ImageHalvingTest, ASideOfAnOddNumberLosesItsLastRow)
  {
    const ImagePixels half = HalveImage(Filled(5, 3, 1, 2, 3, 4), false);

    EXPECT_EQ(half.width, 2);
    EXPECT_EQ(half.height, 1);
  }

  TEST(ImageHalvingTest, HandsBackAnImageWithNothingAsItIs)
  {
    EXPECT_TRUE(HalveImage(ImagePixels{}, true).IsEmpty());
  }
} // namespace
