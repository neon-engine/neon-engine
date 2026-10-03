#include "bezier-path.hpp"

#include <gtest/gtest.h>

#include "polyline.hpp"

namespace
{
  using neon::BezierPath;

  /// Two pieces that meet at (3, 0, 0), with mirrored handles there.
  BezierPath<glm::vec3> TwoPieces()
  {
    return BezierPath<glm::vec3>(
      {{0, 0, 0}, {1, 1, 0}, {2, 1, 0}, {3, 0, 0}, {4, -1, 0}, {5, -1, 0}, {6, 0, 0}});
  }
}

TEST(BezierPath, TakesFourPointsForAPieceAndThreeForEveryFurtherOne)
{
  EXPECT_FALSE(BezierPath<glm::vec3>::FitsPieces(3));
  EXPECT_TRUE(BezierPath<glm::vec3>::FitsPieces(4));
  EXPECT_FALSE(BezierPath<glm::vec3>::FitsPieces(6));
  EXPECT_TRUE(BezierPath<glm::vec3>::FitsPieces(7));

  EXPECT_EQ(TwoPieces().PieceCount(), 2u);
  EXPECT_TRUE(BezierPath<glm::vec3>({{0, 0, 0}, {1, 0, 0}}).IsEmpty());

  // the two points beyond the first piece are left out
  const BezierPath<glm::vec3> ragged({{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {3, 0, 0}, {4, 0, 0}, {5, 0, 0}});
  EXPECT_EQ(ragged.PieceCount(), 1u);
  EXPECT_EQ(ragged.Points().size(), 4u);
}

TEST(BezierPath, GoesThroughEveryThirdPoint)
{
  const auto path = TwoPieces();
  EXPECT_EQ(path.At(0.0f), glm::vec3(0, 0, 0));
  EXPECT_EQ(path.At(1.0f), glm::vec3(3, 0, 0));
  EXPECT_EQ(path.At(2.0f), glm::vec3(6, 0, 0));

  // one and a half is half way through the second piece
  EXPECT_EQ(path.At(1.5f), path.Piece(1).At(0.5f));

  // beyond its ends it stays at them
  EXPECT_EQ(path.At(-1.0f), glm::vec3(0, 0, 0));
  EXPECT_EQ(path.At(7.0f), glm::vec3(6, 0, 0));
}

TEST(BezierPath, IsSmoothWhereTheHandlesMirrorEachOther)
{
  EXPECT_TRUE(TwoPieces().IsSmooth());

  const glm::vec3 before = TwoPieces().Piece(0).Tangent(1.0f);
  const glm::vec3 after = TwoPieces().Piece(1).Tangent(0.0f);
  EXPECT_NEAR(glm::length(after - before), 0.0f, 1e-5f);

  const BezierPath<glm::vec3> corner(
    {{0, 0, 0}, {1, 1, 0}, {2, 1, 0}, {3, 0, 0}, {4, 1, 0}, {5, 1, 0}, {6, 0, 0}});
  EXPECT_FALSE(corner.IsSmooth());
}

TEST(BezierPath, IsFlattenedFromItsFirstPointToItsLast)
{
  const auto path = TwoPieces();
  const auto points = path.Flatten(0.001f);

  ASSERT_GT(points.size(), 4u);
  EXPECT_EQ(points.front(), glm::vec3(0, 0, 0));
  EXPECT_EQ(points.back(), glm::vec3(6, 0, 0));
  EXPECT_NEAR(neon::PolylineLength(points), path.Length(1e-4f), 1e-2f);
}
