#include "luna-vector-image-rasterizer.hpp"

#include <cstddef>
#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
  using neon::ImagePixels;
  using neon::LUNA_VectorImageRasterizer;

  std::vector<unsigned char> Bytes(const std::string &text)
  {
    return {text.begin(), text.end()};
  }

  /// A red disc on nothing, in a square of 16.
  const std::string disc =
    "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"16\" viewBox=\"0 0 16 16\">"
    "<circle cx=\"8\" cy=\"8\" r=\"6\" fill=\"#ff0000\"/>"
    "</svg>";

  class LunaVectorImageRasterizerTest : public ::testing::Test
  {
  protected:
    LUNA_VectorImageRasterizer _rasterizer;
    std::string _error;

    int Load(const std::string &svg)
    {
      return _rasterizer.Load(Bytes(svg), _error);
    }

    ImagePixels Draw(const int image, const int width, const int height)
    {
      ImagePixels pixels;
      EXPECT_TRUE(_rasterizer.Rasterize(image, width, height, pixels));
      return pixels;
    }

    static const unsigned char *At(const ImagePixels &pixels, const int x, const int y)
    {
      return pixels.pixels.data() + (static_cast<std::size_t>(y) * pixels.width + x) * 4;
    }

    /// How many pixels are neither covered nor free, which are those on
    /// the edge of a shape.
    static std::size_t PixelsOnAnEdge(const ImagePixels &pixels)
    {
      std::size_t count = 0;
      for (std::size_t i = 3; i < pixels.pixels.size(); i += 4)
      {
        if (pixels.pixels[i] > 8 && pixels.pixels[i] < 247) { count++; }
      }
      return count;
    }
  };

  TEST_F(LunaVectorImageRasterizerTest, KnowsTheSizeAnImageSaysItHas)
  {
    const int image = Load(disc);
    ASSERT_GE(image, 0) << _error;

    float width = 0.0f;
    float height = 0.0f;
    ASSERT_TRUE(_rasterizer.GetSize(image, width, height));

    EXPECT_FLOAT_EQ(width, 16);
    EXPECT_FLOAT_EQ(height, 16);
  }

  TEST_F(LunaVectorImageRasterizerTest, TakesTheSizeOfTheViewBoxWhenNoOtherIsWritten)
  {
    const int image = Load(
      "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 40 20\"><rect width=\"40\" height=\"20\"/></svg>");
    ASSERT_GE(image, 0) << _error;

    float width = 0.0f;
    float height = 0.0f;
    ASSERT_TRUE(_rasterizer.GetSize(image, width, height));

    EXPECT_FLOAT_EQ(width, 40);
    EXPECT_FLOAT_EQ(height, 20);
  }

  TEST_F(LunaVectorImageRasterizerTest, DrawsAtTheSizeItIsAskedFor)
  {
    const int image = Load(disc);
    ASSERT_GE(image, 0) << _error;

    const ImagePixels small = Draw(image, 16, 16);
    const ImagePixels large = Draw(image, 64, 64);

    EXPECT_EQ(small.width, 16);
    EXPECT_EQ(small.pixels.size(), 16u * 16u * 4u);
    EXPECT_EQ(large.width, 64);
    EXPECT_EQ(large.pixels.size(), 64u * 64u * 4u);
  }

  TEST_F(LunaVectorImageRasterizerTest, HandsOverColorsWithoutAlphaMultipliedIn)
  {
    const int image = Load(
      "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"4\" height=\"4\">"
      "<rect width=\"4\" height=\"4\" fill=\"#ff8000\" fill-opacity=\"0.5\"/></svg>");
    ASSERT_GE(image, 0) << _error;

    const ImagePixels pixels = Draw(image, 4, 4);
    const unsigned char *pixel = At(pixels, 2, 2);

    // red, green, blue, alpha, and the color as it is written
    EXPECT_NEAR(pixel[0], 255, 2);
    EXPECT_NEAR(pixel[1], 128, 2);
    EXPECT_NEAR(pixel[2], 0, 2);
    EXPECT_NEAR(pixel[3], 128, 2);
  }

  TEST_F(LunaVectorImageRasterizerTest, WhatAnImageLeavesFreeIsSeeThrough)
  {
    const int image = Load(disc);
    ASSERT_GE(image, 0) << _error;

    const ImagePixels pixels = Draw(image, 32, 32);

    EXPECT_EQ(At(pixels, 0, 0)[3], 0);
    EXPECT_EQ(At(pixels, 31, 31)[3], 0);

    const unsigned char *middle = At(pixels, 16, 16);
    EXPECT_EQ(middle[0], 255);
    EXPECT_EQ(middle[1], 0);
    EXPECT_EQ(middle[2], 0);
    EXPECT_EQ(middle[3], 255);
  }

  TEST_F(LunaVectorImageRasterizerTest, IsAsSharpAtEverySize)
  {
    const int image = Load(disc);
    ASSERT_GE(image, 0) << _error;

    const ImagePixels small = Draw(image, 16, 16);
    const ImagePixels large = Draw(image, 128, 128);

    // The edge of the disc is a pixel wide at both sizes. An image that
    // was scaled from 16 to 128 would have an edge of 8 pixels, and eight
    // times as many pixels on it as its outline is long.
    const std::size_t on_the_small = PixelsOnAnEdge(small);
    const std::size_t on_the_large = PixelsOnAnEdge(large);

    EXPECT_GT(on_the_small, 20u);
    EXPECT_GT(on_the_large, on_the_small * 5);
    EXPECT_LT(on_the_large, on_the_small * 12);

    // from free to covered within two pixels, along the row through the
    // middle
    int first_touched = -1;
    int first_covered = -1;
    for (int x = 0; x < 128; x++)
    {
      const unsigned char alpha = At(large, x, 64)[3];
      if (first_touched < 0 && alpha > 0) { first_touched = x; }
      if (first_covered < 0 && alpha == 255) { first_covered = x; }
    }

    EXPECT_GT(first_touched, 8);
    EXPECT_LE(first_covered - first_touched, 2);
  }

  TEST_F(LunaVectorImageRasterizerTest, FillsARoomOfAnotherShapeAsItsOwnRulesSay)
  {
    const int image = Load(disc);
    ASSERT_GE(image, 0) << _error;

    // twice as wide as high: the disc stays round
    const ImagePixels pixels = Draw(image, 64, 32);

    EXPECT_EQ(pixels.width, 64);
    EXPECT_EQ(pixels.height, 32);
    EXPECT_EQ(At(pixels, 32, 16)[3], 255);
    EXPECT_EQ(At(pixels, 2, 16)[3], 0);
  }

  TEST_F(LunaVectorImageRasterizerTest, DrawsGradientsAndStrokes)
  {
    const int image = Load(
      "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"32\" height=\"32\">"
      "<defs><linearGradient id=\"g\" x1=\"0\" y1=\"0\" x2=\"1\" y2=\"0\">"
      "<stop offset=\"0\" stop-color=\"#000000\"/><stop offset=\"1\" stop-color=\"#ffffff\"/>"
      "</linearGradient></defs>"
      "<rect x=\"4\" y=\"4\" width=\"24\" height=\"24\" fill=\"url(#g)\" stroke=\"#00ff00\" stroke-width=\"4\"/>"
      "</svg>");
    ASSERT_GE(image, 0) << _error;

    const ImagePixels pixels = Draw(image, 32, 32);

    EXPECT_LT(At(pixels, 8, 16)[0], 80);
    EXPECT_GT(At(pixels, 24, 16)[0], 180);

    // the line around it, which lies half outside
    const unsigned char *stroke = At(pixels, 3, 16);
    EXPECT_EQ(stroke[0], 0);
    EXPECT_EQ(stroke[1], 255);
    EXPECT_EQ(stroke[3], 255);
  }

  TEST_F(LunaVectorImageRasterizerTest, RefusesWhatIsNoSvg)
  {
    EXPECT_EQ(Load(""), -1);
    EXPECT_EQ(_error, "the file is empty");

    EXPECT_EQ(Load("not an image"), -1);
    EXPECT_EQ(_error, "it is no SVG that can be read");

    EXPECT_EQ(Load("<html><body/></html>"), -1);
  }

  TEST_F(LunaVectorImageRasterizerTest, RefusesAnSvgWithoutASize)
  {
    EXPECT_EQ(Load("<svg xmlns=\"http://www.w3.org/2000/svg\"></svg>"), -1);
    EXPECT_EQ(_error, "it has no size: neither a width and a height nor a viewBox say how large it is");
  }

  TEST_F(LunaVectorImageRasterizerTest, RefusesAnSvgThatRefersToAFile)
  {
    // the library would open the file by itself, past the file system
    EXPECT_EQ(
      Load(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"8\" height=\"8\">"
        "<image href=\"/etc/passwd\" width=\"8\" height=\"8\"/></svg>"),
      -1);

    EXPECT_EQ(
      _error,
      "it refers to the image '/etc/passwd', which is a file of its own. An image inside an SVG has to be "
      "part of the SVG, as data");
  }

  TEST_F(LunaVectorImageRasterizerTest, FindsTheFileOfAnImageHoweverItIsWritten)
  {
    const auto find = LUNA_VectorImageRasterizer::FindFileOfAnImage;

    EXPECT_EQ(find("<svg><image xlink:href='a.png'/></svg>"), "a.png");
    EXPECT_EQ(find("<svg><image\n width=\"4\"\n href = \"../b.jpg\" /></svg>"), "../b.jpg");
    EXPECT_EQ(find("<svg><image href=\"data:image/png;base64,AAAA\"/><image href=\"c.png\"/></svg>"), "c.png");

    EXPECT_EQ(find("<svg><image href=\"data:image/png;base64,AAAA\"/></svg>"), "");
    EXPECT_EQ(find("<svg><use href=\"#shape\"/><a href=\"https://example.org\"/></svg>"), "");
    EXPECT_EQ(find("<svg></svg>"), "");
  }

  TEST_F(LunaVectorImageRasterizerTest, AnImageThatWasUnloadedCannotBeDrawn)
  {
    const int image = Load(disc);
    ASSERT_GE(image, 0);

    _rasterizer.Unload(image);

    float width = 0.0f;
    float height = 0.0f;
    ImagePixels pixels;
    EXPECT_FALSE(_rasterizer.GetSize(image, width, height));
    EXPECT_FALSE(_rasterizer.Rasterize(image, 16, 16, pixels));

    EXPECT_EQ(Load(disc), image) << "and its number is given out again";
  }

  TEST_F(LunaVectorImageRasterizerTest, RefusesASizeThatIsNone)
  {
    const int image = Load(disc);
    ASSERT_GE(image, 0);

    ImagePixels pixels;
    EXPECT_FALSE(_rasterizer.Rasterize(image, 0, 16, pixels));
    EXPECT_FALSE(_rasterizer.Rasterize(image, 16, -1, pixels));
    EXPECT_FALSE(_rasterizer.Rasterize(image, 100000, 16, pixels));
    EXPECT_FALSE(_rasterizer.Rasterize(image + 5, 16, 16, pixels));
  }
}
