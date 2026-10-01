#include "color-space.hpp"

#include <gtest/gtest.h>

namespace
{
  using neon::Color;
  using neon::LinearToSrgb;
  using neon::LinearToSrgbByte;
  using neon::SrgbByteToLinear;
  using neon::SrgbToLinear;

  constexpr float tolerance = 1e-4f;

  TEST(ColorSpaceTest, KeepsBlackAndWhiteAsTheyAre)
  {
    EXPECT_FLOAT_EQ(SrgbToLinear(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(SrgbToLinear(1.0f), 1.0f);
    EXPECT_FLOAT_EQ(LinearToSrgb(0.0f), 0.0f);
    EXPECT_NEAR(LinearToSrgb(1.0f), 1.0f, tolerance);
  }

  TEST(ColorSpaceTest, AGreyHalfwayOnTheScreenIsAboutAFifthOfTheLight)
  {
    // the value every table of the sRGB standard gives for 0.5
    EXPECT_NEAR(SrgbToLinear(0.5f), 0.214041f, tolerance);
    EXPECT_NEAR(LinearToSrgb(0.214041f), 0.5f, tolerance);
  }

  TEST(ColorSpaceTest, IsAStraightLineNearBlack)
  {
    EXPECT_NEAR(SrgbToLinear(0.04f), 0.04f / 12.92f, 1e-6f);
    EXPECT_NEAR(LinearToSrgb(0.003f), 0.003f * 12.92f, 1e-6f);
  }

  TEST(ColorSpaceTest, KeepsWhatLiesOutsideZeroToOneInsideIt)
  {
    EXPECT_FLOAT_EQ(SrgbToLinear(-0.5f), 0.0f);
    EXPECT_FLOAT_EQ(SrgbToLinear(2.0f), 1.0f);
    EXPECT_FLOAT_EQ(LinearToSrgb(-0.5f), 0.0f);
    EXPECT_NEAR(LinearToSrgb(2.0f), 1.0f, tolerance);
  }

  TEST(ColorSpaceTest, TurnsEveryByteIntoLightAndBackIntoTheSameByte)
  {
    for (int value = 0; value < 256; value++)
    {
      const auto byte = static_cast<unsigned char>(value);
      EXPECT_EQ(LinearToSrgbByte(SrgbByteToLinear(byte)), byte) << "byte " << value;
      EXPECT_NEAR(SrgbByteToLinear(byte), SrgbToLinear(static_cast<float>(value) / 255.0f), 1e-6f);
    }
  }

  TEST(ColorSpaceTest, TurnsAColourIntoLightAndLeavesItsAlpha)
  {
    const Color linear = SrgbToLinear(Color{1.0f, 0.5f, 0.0f, 0.25f});

    EXPECT_FLOAT_EQ(linear.r, 1.0f);
    EXPECT_NEAR(linear.g, 0.214041f, tolerance);
    EXPECT_FLOAT_EQ(linear.b, 0.0f);
    EXPECT_FLOAT_EQ(linear.a, 0.25f);
  }
} // namespace
