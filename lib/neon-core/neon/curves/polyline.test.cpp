#include "polyline.hpp"

#include <gtest/gtest.h>

#include "cubic-bezier.hpp"

TEST(Polyline, IsAsLongAsItsPieces)
{
  const std::vector<glm::vec3> corner{{0, 0, 0}, {3, 0, 0}, {3, 4, 0}};
  EXPECT_FLOAT_EQ(neon::PolylineLength(corner), 7.0f);
  EXPECT_FLOAT_EQ(neon::PolylineLength(std::vector<glm::vec3>{}), 0.0f);
}

TEST(Polyline, IsResampledAtEvenDistances)
{
  // pieces of uneven length
  const std::vector<glm::vec3> line{{0, 0, 0}, {1, 0, 0}, {8, 0, 0}};
  std::vector<glm::vec3> evenly;
  neon::ResamplePolyline(line, 5, evenly);

  ASSERT_EQ(evenly.size(), 5u);
  for (std::size_t i = 0; i < evenly.size(); i++) { EXPECT_NEAR(evenly[i].x, 2.0f * static_cast<float>(i), 1e-5f); }
}

TEST(Polyline, KeepsItsEndsRoundACorner)
{
  const std::vector<glm::vec3> corner{{0, 0, 0}, {3, 0, 0}, {3, 4, 0}};
  std::vector<glm::vec3> evenly;
  neon::ResamplePolyline(corner, 8, evenly);

  ASSERT_EQ(evenly.size(), 8u);
  EXPECT_EQ(evenly.front(), corner.front());
  EXPECT_EQ(evenly.back(), corner.back());
  EXPECT_NEAR(evenly[3].x, 3.0f, 1e-5f);
  EXPECT_NEAR(evenly[3].y, 0.0f, 1e-5f);
}

TEST(Polyline, MakesTheStepsOfACurveEven)
{
  // the parameter rushes through the middle of this one
  const neon::CubicBezier<glm::vec3> curve{{0, 0, 0}, {0, 0, 0}, {10, 0, 0}, {10, 0, 0}};
  std::vector<glm::vec3> flat{curve.p0};
  curve.Flatten(0.0001f, flat);

  std::vector<glm::vec3> evenly;
  neon::ResamplePolyline(flat, 11, evenly);

  ASSERT_EQ(evenly.size(), 11u);
  for (std::size_t i = 0; i < evenly.size(); i++) { EXPECT_NEAR(evenly[i].x, static_cast<float>(i), 1e-3f); }
}

TEST(Polyline, OfOnePointIsThatPointTwice)
{
  std::vector<glm::vec3> evenly;
  neon::ResamplePolyline(std::vector<glm::vec3>{{1, 2, 3}}, 4, evenly);
  ASSERT_EQ(evenly.size(), 4u);
  for (const auto &point : evenly) { EXPECT_EQ(point, glm::vec3(1, 2, 3)); }
}
