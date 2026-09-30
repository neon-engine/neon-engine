#include "ui-box-paint.hpp"

#include <vector>

#include <gtest/gtest.h>

#include "ui-image-paint.hpp"

// The shape of a box and where an image goes in one, without a file and
// without a renderer. What is handed to the renderer for a whole user
// interface is covered by the functional tests.

namespace
{
  using neon::CutToBox;
  using neon::DistanceToRoundedBox;
  using neon::FitImage;
  using neon::LayoutLength;
  using neon::MatrixOf;
  using neon::ResolvePlace;
  using neon::ResolveRadii;
  using neon::UiBoxPaint;
  using neon::UiCornerRadii;
  using neon::UiMatrix;
  using neon::UiObjectFit;
  using neon::UiRectangle;
  using neon::UiStyle;
  using neon::UiTransformStep;

  UiCornerRadii All(const LayoutLength radius)
  {
    return {radius, radius, radius, radius};
  }

  TEST(ResolveRadiiTest, APixelOfTheFileIsAsManyOfTheFrameAsTheScaleSays)
  {
    float radii[4];
    ResolveRadii(All(LayoutLength::Pixels(10)), {0, 0, 200, 100}, 1.5f, radii);

    for (const float radius : radii) { EXPECT_FLOAT_EQ(radius, 15); }
  }

  TEST(ResolveRadiiTest, APercentageIsOfTheShorterSide)
  {
    float radii[4];
    ResolveRadii(All(LayoutLength::Percent(25)), {0, 0, 200, 100}, 2.0f, radii);

    for (const float radius : radii) { EXPECT_FLOAT_EQ(radius, 25); }
  }

  TEST(ResolveRadiiTest, MakesCornersSmallerThatDoNotFit)
  {
    const UiCornerRadii radii{
      LayoutLength::Pixels(150), LayoutLength::Pixels(150), LayoutLength::Pixels(0), LayoutLength::Pixels(0)
    };

    float pixels[4];
    ResolveRadii(radii, {0, 0, 200, 100}, 1.0f, pixels);

    // 300 along a side of 200, and 150 along one of 100: by two thirds
    EXPECT_FLOAT_EQ(pixels[0], 100);
    EXPECT_FLOAT_EQ(pixels[1], 100);
    EXPECT_FLOAT_EQ(pixels[2], 0);
    EXPECT_FLOAT_EQ(pixels[3], 0);
  }

  TEST(DistanceToRoundedBoxTest, IsBelowZeroInsideAndAboveOutside)
  {
    const float radii[4] = {20, 20, 20, 20};

    EXPECT_FLOAT_EQ(DistanceToRoundedBox(0, 0, 100, 50, radii), -50);
    EXPECT_FLOAT_EQ(DistanceToRoundedBox(100, 0, 100, 50, radii), 0) << "on the right side";
    EXPECT_FLOAT_EQ(DistanceToRoundedBox(110, 0, 100, 50, radii), 10);
    EXPECT_FLOAT_EQ(DistanceToRoundedBox(0, -60, 100, 50, radii), 10);
  }

  TEST(DistanceToRoundedBoxTest, FollowsTheCorner)
  {
    const float radii[4] = {20, 20, 20, 20};

    // the corner of the box is outside the shape, by what the round corner
    // leaves free: the diagonal of a square of 20, less the radius
    EXPECT_NEAR(DistanceToRoundedBox(100, 50, 100, 50, radii), 28.284f - 20.0f, 0.001f);

    // on the arc, 20 from its middle at 80, 30
    EXPECT_NEAR(DistanceToRoundedBox(80 + 14.142f, 30 + 14.142f, 100, 50, radii), 0, 0.001f);
  }

  TEST(DistanceToRoundedBoxTest, EveryCornerHasARadiusOfItsOwn)
  {
    // left top, right top, right bottom, left bottom
    const float radii[4] = {40, 0, 10, 0};

    EXPECT_GT(DistanceToRoundedBox(-98, -48, 100, 50, radii), 0) << "round";
    EXPECT_LT(DistanceToRoundedBox(98, -48, 100, 50, radii), 0) << "square";
    EXPECT_LT(DistanceToRoundedBox(-98, 48, 100, 50, radii), 0) << "square";
    EXPECT_GT(DistanceToRoundedBox(99.5f, 49.5f, 100, 50, radii), 0) << "a little round";
    EXPECT_LT(DistanceToRoundedBox(96, 46, 100, 50, radii), 0);
  }

  class UiBoxPaintTest : public ::testing::Test
  {
  protected:
    UiStyle _style;
    UiRectangle _border_box{100, 100, 300, 200};
    UiRectangle _padding_box{104, 104, 296, 196};

    [[nodiscard]] UiBoxPaint Box() const
    {
      return UiBoxPaint(_style, _border_box, _padding_box, 1.0f);
    }
  };

  TEST_F(UiBoxPaintTest, IsPlainUntilItHasSomethingAShaderWorksOut)
  {
    EXPECT_TRUE(Box().IsPlain());
    EXPECT_FALSE(Box().IsRound());

    _style.border_radius = All(LayoutLength::Pixels(8));
    EXPECT_FALSE(Box().IsPlain());
    EXPECT_TRUE(Box().IsRound());

    _style = UiStyle{};
    _style.box_shadow.emplace_back();
    EXPECT_FALSE(Box().IsPlain());

    _style = UiStyle{};
    _style.border_left_color = neon::Color{1, 0, 0, 1};
    EXPECT_FALSE(Box().IsPlain());

    _style = UiStyle{};
    _style.background_gradient = neon::UiGradient{};
    EXPECT_FALSE(Box().IsPlain());

    // a transform moves rectangles as it moves shapes
    _style = UiStyle{};
    _style.transform.emplace_back();
    EXPECT_TRUE(Box().IsPlain());
  }

  TEST_F(UiBoxPaintTest, TheCornersOfTheInsideAreLessRoundByTheBorder)
  {
    _style.border_radius = All(LayoutLength::Pixels(12));

    const UiBoxPaint box = Box();

    for (int i = 0; i < 4; i++)
    {
      EXPECT_FLOAT_EQ(box.GetRadii()[i], 12);
      EXPECT_FLOAT_EQ(box.GetInnerRadii()[i], 8);
    }

    // and square once the border is as wide as the corner is round
    _padding_box = {120, 120, 280, 180};
    for (int i = 0; i < 4; i++) { EXPECT_FLOAT_EQ(Box().GetInnerRadii()[i], 0); }
  }

  TEST_F(UiBoxPaintTest, APointInARoundCornerIsNotOnTheBox)
  {
    _style.border_radius = All(LayoutLength::Pixels(30));

    const UiBoxPaint box = Box();

    EXPECT_TRUE(box.Contains(200, 150));
    EXPECT_TRUE(box.Contains(100, 150)) << "its left side";
    EXPECT_FALSE(box.Contains(300, 150)) << "its right side is the first pixel that is not on it";

    EXPECT_FALSE(box.Contains(102, 102));
    EXPECT_FALSE(box.Contains(298, 198));

    // 30 from the middle of the arc at 130, 130
    EXPECT_TRUE(box.Contains(130 - 21, 130 - 21));
    EXPECT_FALSE(box.Contains(130 - 22, 130 - 22));
  }

  TEST_F(UiBoxPaintTest, ABoxWithoutRoundCornersHoldsItsCorners)
  {
    EXPECT_TRUE(Box().Contains(100, 100));
    EXPECT_TRUE(Box().Contains(299, 199));
    EXPECT_FALSE(Box().Contains(99, 100));
  }

  TEST_F(UiBoxPaintTest, WhatIsInsideTheBorderHasCornersOfItsOwn)
  {
    _style.border_radius = All(LayoutLength::Pixels(30));

    const UiBoxPaint box = Box();

    EXPECT_FALSE(box.ContainsInPadding(102, 150)) << "on the border";
    EXPECT_TRUE(box.ContainsInPadding(105, 150));
    EXPECT_FALSE(box.ContainsInPadding(106, 106));
    EXPECT_TRUE(box.ContainsInPadding(200, 150));
  }

  TEST(MatrixOfTest, AnElementWithoutATransformIsNotMoved)
  {
    EXPECT_TRUE(MatrixOf(UiStyle{}, {100, 100, 300, 200}, 2.0f).IsIdentity());
  }

  TEST(MatrixOfTest, TurnsAroundTheMiddleOfTheBoxAndMovesInUnitsOfTheFile)
  {
    UiStyle style;

    UiTransformStep move;
    move.x = LayoutLength::Pixels(10);
    move.y = LayoutLength::Percent(50);
    style.transform.push_back(move);

    // at twice the scale: 20 to the right, and half of 100 down
    const UiMatrix moved = MatrixOf(style, {100, 100, 300, 200}, 2.0f);

    float x = 100;
    float y = 100;
    moved.Apply(x, y);
    EXPECT_FLOAT_EQ(x, 120);
    EXPECT_FLOAT_EQ(y, 150);

    UiTransformStep turn;
    turn.kind = UiTransformStep::Kind::Rotate;
    turn.angle = 180;
    style.transform = {turn};

    const UiMatrix turned = MatrixOf(style, {100, 100, 300, 200}, 2.0f);

    x = 100;
    y = 100;
    turned.Apply(x, y);
    EXPECT_NEAR(x, 300, 0.001f);
    EXPECT_NEAR(y, 200, 0.001f);
  }

  TEST(ResolvePlaceTest, APercentageIsOfTheRoomAndALengthOfTheFile)
  {
    EXPECT_FLOAT_EQ(ResolvePlace(LayoutLength::Percent(25), 80, 2.0f), 20);
    EXPECT_FLOAT_EQ(ResolvePlace(LayoutLength::Pixels(25), 80, 2.0f), 50);
    EXPECT_FLOAT_EQ(ResolvePlace(LayoutLength::Percent(50), -40, 1.0f), -20) << "of an image that is too large";
  }

  TEST(CutToBoxTest, LeavesWhatIsInsideAsItIs)
  {
    UiRectangle place{10, 10, 50, 30};
    UiRectangle part{0, 0, 1, 1};

    ASSERT_TRUE(CutToBox({0, 0, 100, 100}, place, part));

    EXPECT_FLOAT_EQ(place.left, 10);
    EXPECT_FLOAT_EQ(place.right, 50);
    EXPECT_FLOAT_EQ(part.right, 1);
  }

  TEST(CutToBoxTest, CutsOffWhatSticksOutAndShowsAsMuchLess)
  {
    UiRectangle place{-20, 80, 60, 120};
    UiRectangle part{0.5f, 0, 1, 1};

    ASSERT_TRUE(CutToBox({0, 0, 100, 100}, place, part));

    EXPECT_FLOAT_EQ(place.left, 0);
    EXPECT_FLOAT_EQ(place.right, 60);
    EXPECT_FLOAT_EQ(place.top, 80);
    EXPECT_FLOAT_EQ(place.bottom, 100);

    // a quarter of its width is left of the box, and half of its height
    // below
    EXPECT_FLOAT_EQ(part.left, 0.625f);
    EXPECT_FLOAT_EQ(part.right, 1);
    EXPECT_FLOAT_EQ(part.top, 0);
    EXPECT_FLOAT_EQ(part.bottom, 0.5f);
  }

  TEST(CutToBoxTest, SaysThatNothingIsLeft)
  {
    UiRectangle place{200, 0, 300, 50};
    UiRectangle part{0, 0, 1, 1};

    EXPECT_FALSE(CutToBox({0, 0, 100, 100}, place, part));

    UiRectangle empty{10, 10, 10, 50};
    EXPECT_FALSE(CutToBox({0, 0, 100, 100}, empty, part));
  }

  TEST(FitImageTest, FillsTheBoxUnlessItIsToldOtherwise)
  {
    UiRectangle place;
    UiRectangle part;
    FitImage(UiStyle{}, 64, 32, {0, 0, 100, 100}, 1.0f, place, part);

    EXPECT_FLOAT_EQ(place.Width(), 100);
    EXPECT_FLOAT_EQ(place.Height(), 100);
    EXPECT_FLOAT_EQ(part.Width(), 1);
  }

  TEST(FitImageTest, AnImageWithoutASizeFillsTheBox)
  {
    UiStyle style;
    style.object_fit = UiObjectFit::Contain;

    UiRectangle place;
    UiRectangle part;
    FitImage(style, 0, 32, {0, 0, 100, 100}, 1.0f, place, part);

    EXPECT_FLOAT_EQ(place.Width(), 100);
    EXPECT_FLOAT_EQ(place.Height(), 100);
  }

  TEST(FitImageTest, PlacesWhatKeepsItsShapeAtWholePixels)
  {
    UiStyle style;
    style.object_fit = UiObjectFit::Contain;
    style.object_position = {LayoutLength::Percent(50), LayoutLength::Percent(50)};

    UiRectangle place;
    UiRectangle part;
    FitImage(style, 30, 10, {0, 0, 100, 50}, 1.0f, place, part);

    // 100 by 33.3, which leaves 16.7 over
    EXPECT_FLOAT_EQ(place.Height(), 33);
    EXPECT_FLOAT_EQ(place.top, 9);
  }
}
