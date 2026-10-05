#include "frame-limit.hpp"

#include <gtest/gtest.h>

namespace
{
  using neon::FrameLimit;

  TEST(FrameLimitTest, ANumberFromThirtyToThreeHundredIsTheLimit)
  {
    EXPECT_EQ(FrameLimit::Of(30), 30);
    EXPECT_EQ(FrameLimit::Of(60), 60);
    EXPECT_EQ(FrameLimit::Of(144), 144);
    EXPECT_EQ(FrameLimit::Of(300), 300);
  }

  TEST(FrameLimitTest, ANumberBeyondThemIsHeldToTheNearest)
  {
    EXPECT_EQ(FrameLimit::Of(1), 30);
    EXPECT_EQ(FrameLimit::Of(29), 30);
    EXPECT_EQ(FrameLimit::Of(301), 300);
    EXPECT_EQ(FrameLimit::Of(100000), 300);
  }

  TEST(FrameLimitTest, NoneAndBelowIsNoLimit)
  {
    EXPECT_EQ(FrameLimit::Of(0), FrameLimit::kUnlimited);
    EXPECT_EQ(FrameLimit::Of(-5), FrameLimit::kUnlimited);
  }

  TEST(FrameLimitTest, AFrameLastsItsShareOfASecondAndNoTimeWithoutALimit)
  {
    EXPECT_DOUBLE_EQ(FrameLimit::SecondsOf(60), 1.0 / 60.0);
    EXPECT_DOUBLE_EQ(FrameLimit::SecondsOf(300), 1.0 / 300.0);
    EXPECT_DOUBLE_EQ(FrameLimit::SecondsOf(FrameLimit::kUnlimited), 0.0);
  }
}
