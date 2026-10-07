#include "texture-scale.hpp"

#include <gtest/gtest.h>

namespace
{
  using neon::TextureScale;

  TEST(TextureScaleTest, KnowsItsScales)
  {
    for (const double scale : {1.0, 0.5, 0.25, 0.125}) { EXPECT_TRUE(TextureScale::IsScale(scale)) << scale; }
    for (const double other : {0.0, 2.0, 0.75, 0.3, -0.5}) { EXPECT_FALSE(TextureScale::IsScale(other)) << other; }
  }

  TEST(TextureScaleTest, HalvesOnceForEveryHalvingTheScaleAsksFor)
  {
    EXPECT_EQ(TextureScale::HalvingsOf(2048, 2048, 1.0), 0);
    EXPECT_EQ(TextureScale::HalvingsOf(2048, 2048, 0.5), 1);
    EXPECT_EQ(TextureScale::HalvingsOf(2048, 2048, 0.25), 2);
    EXPECT_EQ(TextureScale::HalvingsOf(2048, 2048, 0.125), 3);
  }

  TEST(TextureScaleTest, NeverHalvesASideBelowTheFloor)
  {
    // 64 halves once to 32, and no further
    EXPECT_EQ(TextureScale::HalvingsOf(64, 64, 0.125), 1);
    EXPECT_EQ(TextureScale::HalvingsOf(32, 32, 0.5), 0);
    EXPECT_EQ(TextureScale::HalvingsOf(16, 16, 0.125), 0);
  }

  TEST(TextureScaleTest, TheSmallerSideDecides)
  {
    EXPECT_EQ(TextureScale::HalvingsOf(4096, 64, 0.125), 1);
    EXPECT_EQ(TextureScale::HalvingsOf(64, 4096, 0.125), 1);
  }
} // namespace
