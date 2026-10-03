#include "hanging-curve.hpp"

#include <gtest/gtest.h>

namespace
{
  using neon::HangingCurve;
}

TEST(HangingCurve, IsStraightWhenTheRopeIsTaut)
{
  const auto taut = HangingCurve({0, 2, 0}, {3, 2, 0}, 3.0f);
  EXPECT_NEAR(taut.Flatness(), 0.0f, 1e-6f);
  EXPECT_NEAR(taut.Length(), 3.0f, 1e-4f);

  // and when its ends are further apart than it is long
  const auto stretched = HangingCurve({0, 2, 0}, {3, 2, 0}, 1.0f);
  EXPECT_NEAR(stretched.Flatness(), 0.0f, 1e-6f);
}

TEST(HangingCurve, SagsByAsMuchAsMakesItTheLengthOfTheRope)
{
  const auto slack = HangingCurve({0, 2, 0}, {3, 2, 0}, 4.0f);

  EXPECT_NEAR(slack.Length(1e-4f), 4.0f, 0.01f);
  EXPECT_EQ(slack.p0, glm::vec3(0, 2, 0));
  EXPECT_EQ(slack.p3, glm::vec3(3, 2, 0));

  // lowest in the middle, and below both ends
  const glm::vec3 middle = slack.At(0.5f);
  EXPECT_NEAR(middle.x, 1.5f, 1e-4f);
  EXPECT_LT(middle.y, 1.0f);
  EXPECT_LT(middle.y, slack.At(0.25f).y);
}

TEST(HangingCurve, HangsAsALoopWhenItsEndsAreTogether)
{
  const auto loop = HangingCurve({1, 2, 0}, {1, 2, 0}, 2.0f);
  EXPECT_NEAR(loop.Length(1e-4f), 2.0f, 0.01f);
  EXPECT_NEAR(loop.At(0.5f).y, 1.0f, 0.02f);
}

TEST(HangingCurve, SagsTheWayItIsTold)
{
  const auto sideways = HangingCurve({0, 0, 0}, {0, 0, 2}, 3.0f, {1, 0, 0});
  EXPECT_GT(sideways.At(0.5f).x, 0.5f);
  EXPECT_NEAR(sideways.At(0.5f).y, 0.0f, 1e-5f);
}
