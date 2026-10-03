#include "cubic-bezier.hpp"

#include <array>

#include <glm/gtc/constants.hpp>
#include <gtest/gtest.h>

#include "bezier.hpp"

namespace
{
  using neon::CubicBezier;

  void ExpectNear(const glm::vec3 &actual, const glm::vec3 &expected, const float tolerance = 1e-5f)
  {
    EXPECT_NEAR(actual.x, expected.x, tolerance);
    EXPECT_NEAR(actual.y, expected.y, tolerance);
    EXPECT_NEAR(actual.z, expected.z, tolerance);
  }

  const CubicBezier<glm::vec3> arc{{0, 0, 0}, {0, 2, 0}, {3, 2, 1}, {3, 0, 1}};
}

TEST(CubicBezier, StartsAtItsFirstPointAndEndsAtItsLast)
{
  ExpectNear(arc.At(0.0f), arc.p0);
  ExpectNear(arc.At(1.0f), arc.p3);
}

TEST(CubicBezier, IsTheWeightedAverageOfItsPoints)
{
  // (1-t)^3 p0 + 3(1-t)^2 t p1 + 3(1-t) t^2 p2 + t^3 p3 at a quarter
  const glm::vec3 expected = arc.p0 * 0.421875f + arc.p1 * 0.421875f + arc.p2 * 0.140625f + arc.p3 * 0.015625f;
  ExpectNear(arc.At(0.25f), expected);
}

TEST(CubicBezier, AgreesWithTheCurveOfAnyOrder)
{
  const std::array<glm::vec3, 4> points{arc.p0, arc.p1, arc.p2, arc.p3};
  for (const float t : {0.0f, 0.1f, 0.5f, 0.9f, 1.0f})
  {
    ExpectNear(arc.At(t), neon::BezierPoint<glm::vec3>(points, t));
  }
}

TEST(CubicBezier, LeavesTowardsItsSecondPointAndArrivesFromItsThird)
{
  ExpectNear(arc.Tangent(0.0f), (arc.p1 - arc.p0) * 3.0f);
  ExpectNear(arc.Tangent(1.0f), (arc.p3 - arc.p2) * 3.0f);
}

TEST(CubicBezier, ItsTangentIsHowThePointMoves)
{
  constexpr float step = 1e-3f;
  const glm::vec3 moved = (arc.At(0.4f + step) - arc.At(0.4f - step)) / (2.0f * step);
  ExpectNear(arc.Tangent(0.4f), moved, 1e-2f);
}

TEST(CubicBezier, CutInTwoIsTheSameCurve)
{
  const auto [first, second] = arc.Split(0.25f);

  ExpectNear(first.p0, arc.p0);
  ExpectNear(first.p3, arc.At(0.25f));
  ExpectNear(second.p0, arc.At(0.25f));
  ExpectNear(second.p3, arc.p3);

  // half way through the first piece is an eighth of the way through the whole
  ExpectNear(first.At(0.5f), arc.At(0.125f));
  ExpectNear(second.At(0.5f), arc.At(0.625f));
}

TEST(CubicBezier, AStraightLineIsFlatAndOnePiece)
{
  const CubicBezier<glm::vec3> line{{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {3, 0, 0}};
  EXPECT_NEAR(line.Flatness(), 0.0f, 1e-6f);

  std::vector<glm::vec3> points;
  line.Flatten(0.01f, points);
  ASSERT_EQ(points.size(), 1u);
  ExpectNear(points[0], line.p3);
  EXPECT_NEAR(line.Length(), 3.0f, 1e-4f);
}

TEST(CubicBezier, IsFlattenedToWithinTheTolerance)
{
  std::vector<glm::vec3> coarse;
  std::vector<glm::vec3> fine;
  arc.Flatten(0.1f, coarse);
  arc.Flatten(0.001f, fine);

  EXPECT_GT(coarse.size(), 1u);
  EXPECT_GT(fine.size(), coarse.size());
  ExpectNear(fine.back(), arc.p3);

  // every point of the curve is near the line of straight pieces
  std::vector<glm::vec3> line{arc.p0};
  line.insert(line.end(), coarse.begin(), coarse.end());
  for (int i = 0; i <= 100; i++)
  {
    const glm::vec3 point = arc.At(static_cast<float>(i) / 100.0f);
    float nearest = 1e9f;
    for (std::size_t piece = 1; piece < line.size(); piece++)
    {
      const glm::vec3 along = line[piece] - line[piece - 1];
      const float t = glm::clamp(dot(point - line[piece - 1], along) / dot(along, along), 0.0f, 1.0f);
      nearest = std::min(nearest, glm::length(point - (line[piece - 1] + along * t)));
    }
    EXPECT_LE(nearest, 0.1f) << "at " << i;
  }
}

TEST(CubicBezier, MeasuresAQuarterCircle)
{
  // the usual four points for a quarter of a circle of radius 1, which is
  // off by less than three parts in ten thousand
  constexpr float k = 0.5522847f;
  const CubicBezier<glm::vec2> quarter{{1, 0}, {1, k}, {k, 1}, {0, 1}};
  EXPECT_NEAR(quarter.Length(1e-5f), glm::half_pi<float>(), 1e-3f);
}

TEST(CubicBezier, CarriesAValueAsWellAsAPoint)
{
  // a float that eases in and out between 0 and 1
  const CubicBezier<float> ease{0.0f, 0.0f, 1.0f, 1.0f};
  EXPECT_NEAR(ease.At(0.5f), 0.5f, 1e-6f);
  EXPECT_LT(ease.At(0.25f), 0.25f);
  EXPECT_GT(ease.At(0.75f), 0.75f);
}

TEST(Bezier, ALineAndAQuadraticCurveAreCurvesToo)
{
  const std::array<glm::vec2, 2> line{glm::vec2{0, 0}, glm::vec2{4, 2}};
  const glm::vec2 middle = neon::BezierPoint<glm::vec2>(line, 0.5f);
  EXPECT_NEAR(middle.x, 2.0f, 1e-6f);
  EXPECT_NEAR(middle.y, 1.0f, 1e-6f);

  const std::array<glm::vec2, 3> quadratic{glm::vec2{0, 0}, glm::vec2{1, 2}, glm::vec2{2, 0}};
  const glm::vec2 top = neon::BezierPoint<glm::vec2>(quadratic, 0.5f);
  EXPECT_NEAR(top.x, 1.0f, 1e-6f);
  EXPECT_NEAR(top.y, 1.0f, 1e-6f);
}

TEST(Bezier, TheWeightsAddUpToOne)
{
  for (const std::size_t order : {1u, 2u, 3u, 6u})
  {
    float sum = 0.0f;
    for (std::size_t i = 0; i <= order; i++) { sum += neon::BernsteinWeight(order, i, 0.3f); }
    EXPECT_NEAR(sum, 1.0f, 1e-5f) << "order " << order;
  }
}
