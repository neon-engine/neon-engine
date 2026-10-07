#include "target-quality.hpp"

#include <gtest/gtest.h>

namespace
{
  using neon::TargetQuality;

  TEST(TargetQualityTest, KnowsItsScales)
  {
    for (const double scale : {1.0, 0.5, 0.25}) { EXPECT_TRUE(TargetQuality::IsScale(scale)) << scale; }
    for (const double other : {0.0, 0.125, 2.0, 0.3}) { EXPECT_FALSE(TargetQuality::IsScale(other)) << other; }
  }

  TEST(TargetQualityTest, KnowsWhatLevelsCanBeAskedFor)
  {
    for (const int mipmaps : {0, 1, 4, 16}) { EXPECT_TRUE(TargetQuality::IsMipmaps(mipmaps)) << mipmaps; }
    for (const int other : {-1, 17}) { EXPECT_FALSE(TargetQuality::IsMipmaps(other)) << other; }
  }

  TEST(TargetQualityTest, ScalesASideButNotBelowTheFloor)
  {
    EXPECT_EQ(TargetQuality::SizeAt(512, 1.0), 512);
    EXPECT_EQ(TargetQuality::SizeAt(512, 0.5), 256);
    EXPECT_EQ(TargetQuality::SizeAt(512, 0.25), 128);
    EXPECT_EQ(TargetQuality::SizeAt(40, 0.25), 16);
    EXPECT_EQ(TargetQuality::SizeAt(8, 0.25), 8) << "what was smaller already stays";
  }

  TEST(TargetQualityTest, TakesTheFewestLevelsAskedFor)
  {
    EXPECT_EQ(TargetQuality::LevelsOf(10, 0, 0), 10) << "as many as the size allows";
    EXPECT_EQ(TargetQuality::LevelsOf(10, 4, 0), 4);
    EXPECT_EQ(TargetQuality::LevelsOf(10, 0, 3), 3);
    EXPECT_EQ(TargetQuality::LevelsOf(10, 4, 2), 2);
    EXPECT_EQ(TargetQuality::LevelsOf(10, 2, 4), 2) << "a setting lowers what a camera asks, never raises it";
    EXPECT_EQ(TargetQuality::LevelsOf(3, 8, 0), 3) << "never more than the size allows";
    EXPECT_EQ(TargetQuality::LevelsOf(10, 1, 0), 1) << "1 is none";
  }
} // namespace
