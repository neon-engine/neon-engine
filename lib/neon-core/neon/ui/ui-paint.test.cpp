#include "ui-paint.hpp"

#include <vector>

#include <gtest/gtest.h>

namespace
{
  using neon::ChooseImageSource;
  using neon::Color;
  using neon::LayoutLength;
  using neon::ToMatrix;
  using neon::UiGradient;
  using neon::UiImageSource;
  using neon::UiMatrix;
  using neon::UiTransformStep;

  void ExpectPoint(const UiMatrix &matrix, float x, float y, const float to_x, const float to_y)
  {
    matrix.Apply(x, y);
    EXPECT_NEAR(x, to_x, 0.001f);
    EXPECT_NEAR(y, to_y, 0.001f);
  }

  UiTransformStep Translate(const LayoutLength x, const LayoutLength y)
  {
    UiTransformStep step;
    step.kind = UiTransformStep::Kind::Translate;
    step.x = x;
    step.y = y;
    return step;
  }

  UiTransformStep Rotate(const float degrees)
  {
    UiTransformStep step;
    step.kind = UiTransformStep::Kind::Rotate;
    step.angle = degrees;
    return step;
  }

  UiTransformStep Scale(const float x, const float y)
  {
    UiTransformStep step;
    step.kind = UiTransformStep::Kind::Scale;
    step.scale_x = x;
    step.scale_y = y;
    return step;
  }

  TEST(UiMatrixTest, StartsAsWhatMovesNothing)
  {
    const UiMatrix matrix;

    EXPECT_TRUE(matrix.IsIdentity());
    ExpectPoint(matrix, 3, 4, 3, 4);
  }

  TEST(UiMatrixTest, IsUndoneByItsInverse)
  {
    const UiMatrix matrix = ToMatrix({Translate(LayoutLength::Pixels(30), LayoutLength::Pixels(-5)), Rotate(30), Scale(2, 0.5f)}, 100, 50, 50, 25);

    UiMatrix inverse;
    ASSERT_TRUE(matrix.Invert(inverse));

    float x = 17;
    float y = 42;
    matrix.Apply(x, y);
    ExpectPoint(inverse, x, y, 17, 42);
  }

  TEST(UiMatrixTest, WhatFlattensEverythingHasNoInverse)
  {
    const UiMatrix flat = ToMatrix({Scale(0, 1)}, 100, 50, 0, 0);

    UiMatrix inverse;
    EXPECT_FALSE(flat.Invert(inverse));
  }

  TEST(ToMatrixTest, NoStepsMoveNothing)
  {
    EXPECT_TRUE(ToMatrix({}, 100, 50, 50, 25).IsIdentity());
  }

  TEST(ToMatrixTest, MovesByPixelsAndByPartsOfTheElement)
  {
    const UiMatrix matrix =
      ToMatrix({Translate(LayoutLength::Pixels(10), LayoutLength::Percent(50))}, 200, 80, 100, 40);

    ExpectPoint(matrix, 0, 0, 10, 40);
    ExpectPoint(matrix, 5, 5, 15, 45);
  }

  TEST(ToMatrixTest, TurnsClockwiseAroundTheOrigin)
  {
    // around the middle of a box of 100 by 100
    const UiMatrix matrix = ToMatrix({Rotate(90)}, 100, 100, 50, 50);

    ExpectPoint(matrix, 50, 50, 50, 50);

    // the middle of the top goes to the middle of the right side
    ExpectPoint(matrix, 50, 0, 100, 50);
    ExpectPoint(matrix, 0, 0, 100, 0);
  }

  TEST(ToMatrixTest, TurnsAroundACornerWhenThatIsTheOrigin)
  {
    const UiMatrix matrix = ToMatrix({Rotate(90)}, 100, 100, 0, 0);

    ExpectPoint(matrix, 0, 0, 0, 0);
    ExpectPoint(matrix, 100, 0, 0, 100);
  }

  TEST(ToMatrixTest, MakesLargerFromTheOrigin)
  {
    const UiMatrix matrix = ToMatrix({Scale(2, 3)}, 100, 100, 50, 50);

    ExpectPoint(matrix, 50, 50, 50, 50);
    ExpectPoint(matrix, 60, 60, 70, 80);
    ExpectPoint(matrix, 0, 0, -50, -100);
  }

  TEST(ToMatrixTest, DoesTheStepThatIsWrittenLastFirst)
  {
    // as CSS does: the point is turned, and then moved
    const UiMatrix moved_then_turned =
      ToMatrix({Translate(LayoutLength::Pixels(100), LayoutLength::Pixels(0)), Rotate(90)}, 10, 10, 0, 0);
    ExpectPoint(moved_then_turned, 10, 0, 100, 10);

    // the other way around the move is turned as well
    const UiMatrix turned_then_moved =
      ToMatrix({Rotate(90), Translate(LayoutLength::Pixels(100), LayoutLength::Pixels(0))}, 10, 10, 0, 0);
    ExpectPoint(turned_then_moved, 10, 0, 0, 110);
  }

  TEST(UiMatrixTest, OneAfterAnotherIsBothInThatOrder)
  {
    const UiMatrix moves = ToMatrix({Translate(LayoutLength::Pixels(5), LayoutLength::Pixels(0))}, 0, 0, 0, 0);
    const UiMatrix doubles = ToMatrix({Scale(2, 2)}, 0, 0, 0, 0);

    ExpectPoint(doubles.After(moves), 1, 1, 12, 2);
    ExpectPoint(moves.After(doubles), 1, 1, 7, 2);
  }

  TEST(UiGradientTest, BlendsBetweenTheColorsAroundAPlace)
  {
    UiGradient gradient;
    gradient.stops = {
      {{1, 0, 0, 1}, 0.0f, true},
      {{0, 0, 1, 1}, 0.5f, true},
      {{0, 1, 0, 1}, 1.0f, true}
    };

    EXPECT_FLOAT_EQ(gradient.At(0).r, 1);
    EXPECT_FLOAT_EQ(gradient.At(0.25f).r, 0.5f);
    EXPECT_FLOAT_EQ(gradient.At(0.25f).b, 0.5f);
    EXPECT_FLOAT_EQ(gradient.At(0.5f).b, 1);
    EXPECT_FLOAT_EQ(gradient.At(0.75f).g, 0.5f);
    EXPECT_FLOAT_EQ(gradient.At(1).g, 1);
  }

  TEST(UiGradientTest, HasTheColorsOfItsEndsBeyondThem)
  {
    UiGradient gradient;
    gradient.stops = {{{1, 0, 0, 1}, 0.2f, true}, {{0, 0, 1, 1}, 0.8f, true}};

    EXPECT_FLOAT_EQ(gradient.At(-1).r, 1);
    EXPECT_FLOAT_EQ(gradient.At(0.1f).r, 1);
    EXPECT_FLOAT_EQ(gradient.At(0.9f).b, 1);
    EXPECT_FLOAT_EQ(gradient.At(5).b, 1);
  }

  TEST(UiGradientTest, AColorThatIsSeeThroughDoesNotShineThroughItsNeighbor)
  {
    // from red to nothing, where the nothing is written as black
    UiGradient gradient;
    gradient.stops = {{{1, 0, 0, 1}, 0.0f, true}, {{0, 0, 0, 0}, 1.0f, true}};

    const Color middle = gradient.At(0.5f);

    EXPECT_FLOAT_EQ(middle.a, 0.5f);
    EXPECT_FLOAT_EQ(middle.r, 1) << "red at half its alpha, and not a dark red";
  }

  TEST(UiGradientTest, TwoColorsAtOnePlaceAreASharpEdge)
  {
    UiGradient gradient;
    gradient.stops = {
      {{1, 0, 0, 1}, 0.0f, true},
      {{1, 0, 0, 1}, 0.5f, true},
      {{0, 0, 1, 1}, 0.5f, true},
      {{0, 0, 1, 1}, 1.0f, true}
    };

    EXPECT_FLOAT_EQ(gradient.At(0.49f).r, 1);
    EXPECT_FLOAT_EQ(gradient.At(0.51f).b, 1);
  }

  TEST(UiGradientTest, WithoutColorsItIsNothing)
  {
    const UiGradient gradient;
    EXPECT_FLOAT_EQ(gradient.At(0.5f).a, 0);
  }

  TEST(ChooseImageSourceTest, TakesTheImageThatWasMadeForTheScale)
  {
    const std::vector<UiImageSource> sources = {{"a.png", 1}, {"a@2x.png", 2}, {"a@3x.png", 3}};

    EXPECT_EQ(ChooseImageSource(sources, 1.0f), 0u);
    EXPECT_EQ(ChooseImageSource(sources, 2.0f), 1u);
    EXPECT_EQ(ChooseImageSource(sources, 3.0f), 2u);
  }

  TEST(ChooseImageSourceTest, TakesTheNextDenserOneBetweenTwo)
  {
    const std::vector<UiImageSource> sources = {{"a.png", 1}, {"a@2x.png", 2}, {"a@3x.png", 3}};

    // an image that is made smaller stays sharp, one that is made larger
    // does not
    EXPECT_EQ(ChooseImageSource(sources, 1.333f), 1u);
    EXPECT_EQ(ChooseImageSource(sources, 0.667f), 0u);
    EXPECT_EQ(ChooseImageSource(sources, 2.5f), 2u);

    // what is all but enough counts as enough
    EXPECT_EQ(ChooseImageSource(sources, 1.02f), 0u);
    EXPECT_EQ(ChooseImageSource(sources, 2.04f), 1u);
  }

  TEST(ChooseImageSourceTest, TakesTheDensestWhenNoneIsEnough)
  {
    const std::vector<UiImageSource> sources = {{"a@2x.png", 2}, {"a.png", 1}};

    EXPECT_EQ(ChooseImageSource(sources, 4.0f), 0u);
    EXPECT_EQ(ChooseImageSource(sources, 1.0f), 1u) << "whatever order they are written in";
  }

  TEST(ChooseImageSourceTest, OneImageIsTakenAtEveryScale)
  {
    const std::vector<UiImageSource> sources = {{"a.png", 1}};

    EXPECT_EQ(ChooseImageSource(sources, 0.5f), 0u);
    EXPECT_EQ(ChooseImageSource(sources, 3.0f), 0u);
  }
}
