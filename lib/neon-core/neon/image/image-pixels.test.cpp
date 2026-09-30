#include "image-pixels.hpp"

#include <cstddef>
#include <vector>

#include <gtest/gtest.h>

namespace
{
  using neon::ImagePixels;
  using neon::MakeSmallerCopies;
  using neon::PremultiplyAlpha;

  ImagePixels Image(const int width, const int height, const std::vector<unsigned char> &pixels)
  {
    ImagePixels image;
    image.width = width;
    image.height = height;
    image.pixels = pixels;
    return image;
  }

  /// An image of one colour, with alpha multiplied in.
  ImagePixels Filled(const int width, const int height, const unsigned char value, const unsigned char alpha)
  {
    ImagePixels image;
    image.width = width;
    image.height = height;

    for (int i = 0; i < width * height; i++)
    {
      for (int channel = 0; channel < 3; channel++) { image.pixels.push_back(value); }
      image.pixels.push_back(alpha);
    }

    PremultiplyAlpha(image.pixels);
    return image;
  }

  /// The colour of a pixel as it is seen: without its alpha multiplied in.
  float SeenAs(const ImagePixels &image, const std::size_t pixel, const std::size_t channel)
  {
    const float alpha = image.pixels[pixel * 4 + 3];
    return alpha > 0.0f ? static_cast<float>(image.pixels[pixel * 4 + channel]) / alpha * 255.0f : 0.0f;
  }

  TEST(PremultiplyAlphaTest, MultipliesAlphaIntoTheColours)
  {
    std::vector<unsigned char> pixels = {
      255, 255, 255, 255,
      255, 128, 0, 128,
      200, 100, 50, 0,
      10, 20, 30, 255
    };

    PremultiplyAlpha(pixels);

    EXPECT_EQ(pixels, (std::vector<unsigned char>{
                255, 255, 255, 255,
                128, 64, 0, 128,
                0, 0, 0, 0,
                10, 20, 30, 255
              }));
  }

  TEST(MakeSmallerCopiesTest, HalvesTheImageDownToOnePixel)
  {
    const auto copies = MakeSmallerCopies(Filled(16, 4, 255, 255));

    ASSERT_EQ(copies.size(), 5u);

    const int widths[5] = {16, 8, 4, 2, 1};
    const int heights[5] = {4, 2, 1, 1, 1};

    for (std::size_t i = 0; i < copies.size(); i++)
    {
      EXPECT_EQ(copies[i].width, widths[i]) << "copy " << i;
      EXPECT_EQ(copies[i].height, heights[i]) << "copy " << i;
      EXPECT_EQ(copies[i].pixels.size(), static_cast<std::size_t>(widths[i]) * heights[i] * 4);
    }
  }

  TEST(MakeSmallerCopiesTest, TheFirstIsTheImageItself)
  {
    const ImagePixels image = Image(2, 1, {1, 2, 3, 4, 5, 6, 7, 8});

    const auto copies = MakeSmallerCopies(image);

    ASSERT_EQ(copies.size(), 2u);
    EXPECT_EQ(copies[0].pixels, image.pixels);
  }

  TEST(MakeSmallerCopiesTest, AnImageOfOneColourStaysThatColour)
  {
    for (const auto &copy : MakeSmallerCopies(Filled(8, 8, 200, 255)))
    {
      for (std::size_t i = 0; i < copy.pixels.size(); i += 4)
      {
        EXPECT_EQ(copy.pixels[i], 200);
        EXPECT_EQ(copy.pixels[i + 3], 255);
      }
    }
  }

  TEST(MakeSmallerCopiesTest, WhiteNextToNothingStaysWhite)
  {
    // A white pixel that covers everything next to one that is see-through,
    // written as black the way image editors write it. The mean of the
    // colours alone would be grey.
    ImagePixels image = Image(2, 1, {255, 255, 255, 255, 0, 0, 0, 0});
    PremultiplyAlpha(image.pixels);

    const auto copies = MakeSmallerCopies(image);

    ASSERT_EQ(copies.size(), 2u);
    ASSERT_EQ(copies[1].pixels.size(), 4u);

    EXPECT_EQ(copies[1].pixels[3], 128) << "half as much is covered";

    for (std::size_t channel = 0; channel < 3; channel++)
    {
      // with alpha multiplied in, white at half the alpha
      EXPECT_EQ(copies[1].pixels[channel], 128);
      EXPECT_NEAR(SeenAs(copies[1], 0, channel), 255, 1) << "and what is seen is white, not grey";
    }
  }

  TEST(MakeSmallerCopiesTest, TheColourOfWhatIsSeeThroughDoesNotCount)
  {
    // red that covers everything, next to green that is not there at all
    ImagePixels image = Image(2, 2, {
                                255, 0, 0, 255, 0, 255, 0, 0,
                                0, 255, 0, 0, 255, 0, 0, 255
                              });
    PremultiplyAlpha(image.pixels);

    const auto copies = MakeSmallerCopies(image);
    const ImagePixels &smallest = copies.back();

    EXPECT_EQ(smallest.pixels[3], 128);
    EXPECT_NEAR(SeenAs(smallest, 0, 0), 255, 1);
    EXPECT_NEAR(SeenAs(smallest, 0, 1), 0, 1) << "no green shines through";
  }

  TEST(MakeSmallerCopiesTest, TakesInTheLastRowOfAnImageWithAnOddSide)
  {
    // three columns: black, black, white
    const ImagePixels image = Image(3, 1, {0, 0, 0, 255, 0, 0, 0, 255, 255, 255, 255, 255});

    const auto copies = MakeSmallerCopies(image);

    ASSERT_EQ(copies.size(), 2u);
    EXPECT_EQ(copies[1].width, 1);
    EXPECT_EQ(copies[1].pixels[0], 85) << "a third of it is white";
  }

  TEST(MakeSmallerCopiesTest, MakesNothingOfWhatIsNoImage)
  {
    EXPECT_TRUE(MakeSmallerCopies(ImagePixels{}).empty());
    EXPECT_TRUE(MakeSmallerCopies(Image(2, 2, {1, 2, 3})).empty());
  }

  TEST(CopyPixelsTest, PutsAPartOfAnImageOverAnother)
  {
    const ImagePixels from = Filled(4, 4, 255, 255);
    ImagePixels target = Filled(4, 4, 0, 255);

    neon::CopyPixels(from, 0, 0, 2, 2, target, 1, 1);

    const auto red_at = [&target](const int x, const int y)
    {
      return target.pixels[(static_cast<std::size_t>(y) * 4 + x) * 4];
    };

    EXPECT_EQ(red_at(0, 0), 0);
    EXPECT_EQ(red_at(1, 1), 255);
    EXPECT_EQ(red_at(2, 2), 255);
    EXPECT_EQ(red_at(3, 3), 0);
  }

  TEST(CopyPixelsTest, LetsWhatIsBelowShowThroughWhatIsSeeThrough)
  {
    const ImagePixels from = Filled(1, 1, 255, 128);
    ImagePixels target = Filled(1, 1, 0, 255);

    neon::CopyPixels(from, 0, 0, 1, 1, target, 0, 0);

    EXPECT_NEAR(target.pixels[0], 128, 1);
    EXPECT_EQ(target.pixels[3], 255);
  }

  TEST(CopyPixelsTest, LeavesOutWhatLiesOutsideEitherImage)
  {
    const ImagePixels from = Filled(2, 2, 255, 255);
    ImagePixels target = Filled(2, 2, 0, 255);

    neon::CopyPixels(from, 1, 1, 4, 4, target, 1, 1);
    neon::CopyPixels(from, -3, -3, 2, 2, target, -1, -1);

    EXPECT_EQ(target.pixels[0], 0);
    EXPECT_EQ(target.pixels[3 * 4], 255);
  }
}
