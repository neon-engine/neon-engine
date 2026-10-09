#include "flex-layout-engine.hpp"

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

// What is expected here is what CSS says, worked out by hand from
// https://www.w3.org/TR/css-flexbox-1/ and https://www.w3.org/TR/css-position-3/
// and not what the engine happened to give.

namespace
{
  using neon::AlignContent;
  using neon::AlignItems;
  using neon::AlignSelf;
  using neon::Flex_LayoutEngine;
  using neon::FlexDirection;
  using neon::FlexWrap;
  using neon::JustifyContent;
  using neon::LayoutBox;
  using neon::LayoutBoxSizing;
  using neon::LayoutDisplay;
  using neon::LayoutLength;
  using neon::LayoutNode;
  using neon::LayoutPosition;
  using neon::LayoutSize;
  using neon::LayoutStyle;

  LayoutLength Px(const float pixels)
  {
    return LayoutLength::Pixels(pixels);
  }

  LayoutLength Percent(const float percent)
  {
    return LayoutLength::Percent(percent);
  }

  LayoutStyle Sized(const float width, const float height)
  {
    LayoutStyle style;
    style.width = Px(width);
    style.height = Px(height);
    return style;
  }

  LayoutStyle Wide(const float width)
  {
    LayoutStyle style;
    style.width = Px(width);
    return style;
  }

  LayoutStyle Absolute()
  {
    LayoutStyle style;
    style.position = LayoutPosition::Absolute;
    return style;
  }

  class FlexLayoutEngineTest : public ::testing::Test
  {
  protected:
    Flex_LayoutEngine _engine;

    LayoutNode Box(const LayoutStyle &style, const std::vector<LayoutNode> &children = {})
    {
      const LayoutNode node = _engine.CreateNode();
      _engine.SetStyle(node, style);
      _engine.SetChildren(node, children);
      return node;
    }

    /// A box whose content is a text of one line of 300 by 20, which breaks
    /// into lines of 20 when it has less room than that.
    LayoutNode Text(const LayoutStyle &style = {})
    {
      const LayoutNode node = Box(style);
      _engine.SetMeasure(node, [](const float available_width, float)
      {
        if (std::isnan(available_width) || available_width >= 300.0f) { return LayoutSize{300.0f, 20.0f}; }

        const float lines = std::ceil(300.0f / std::max(available_width, 1.0f));
        return LayoutSize{available_width, lines * 20.0f};
      });
      return node;
    }

    void ExpectBox(
      const LayoutNode node,
      const float left,
      const float top,
      const float width,
      const float height) const
    {
      const LayoutBox box = _engine.GetBox(node);
      EXPECT_NEAR(box.left, left, 0.01f) << "left";
      EXPECT_NEAR(box.top, top, 0.01f) << "top";
      EXPECT_NEAR(box.width, width, 0.01f) << "width";
      EXPECT_NEAR(box.height, height, 0.01f) << "height";
    }

    /// Three boxes of 100 by 50 in a container of 500 by 200.
    struct Three
    {
      LayoutNode root;
      LayoutNode a;
      LayoutNode b;
      LayoutNode c;
    };

    Three ThreeIn(LayoutStyle container, const float width = 500.0f, const float height = 200.0f)
    {
      container.width = Px(width);
      container.height = Px(height);

      Three three{};
      three.a = Box(Sized(100, 50));
      three.b = Box(Sized(100, 50));
      three.c = Box(Sized(100, 50));
      three.root = Box(container, {three.a, three.b, three.c});

      _engine.Calculate(three.root, 1920, 1080);
      return three;
    }
  };

  // the root

  TEST_F(FlexLayoutEngineTest, ARootWithoutASizeFillsTheRoom)
  {
    const auto root = Box({});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(root, 0, 0, 1920, 1080);
  }

  TEST_F(FlexLayoutEngineTest, ARootKeepsTheSizeItWasGiven)
  {
    const auto root = Box(Sized(300, 200));
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(root, 0, 0, 300, 200);
  }

  TEST_F(FlexLayoutEngineTest, ARootInPercentRefersToTheRoom)
  {
    LayoutStyle style;
    style.width = Percent(50);
    style.height = Percent(25);

    const auto root = Box(style);
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(root, 0, 0, 960, 270);
  }

  TEST_F(FlexLayoutEngineTest, AnAbsoluteRootIsPlacedAgainstTheRoom)
  {
    LayoutStyle style = Sized(100, 50);
    style.position = LayoutPosition::Absolute;
    style.inset.right = Px(20);
    style.inset.bottom = Px(10);

    const auto inside = Box(Sized(30, 30));
    const auto root = Box(style, {inside});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(root, 1800, 1020, 100, 50);
    ExpectBox(inside, 0, 0, 30, 30);
  }

  TEST_F(FlexLayoutEngineTest, AnAbsoluteRootWithoutASizeIsAsLargeAsItsContent)
  {
    LayoutStyle style = Absolute();
    style.inset.left = Px(20);
    style.inset.top = Px(10);

    const auto inside = Box(Sized(30, 40));
    const auto root = Box(style, {inside});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(root, 20, 10, 30, 40);
  }

  TEST_F(FlexLayoutEngineTest, AnAbsoluteRootBetweenAllSidesFillsTheRoom)
  {
    LayoutStyle style = Absolute();
    style.inset = {Px(10), Px(20), Px(30), Px(40)};

    const auto root = Box(style);
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(root, 40, 10, 1860, 1040);
  }

  TEST_F(FlexLayoutEngineTest, ARootKeepsItsMargins)
  {
    LayoutStyle style;
    style.margin = {Px(10), Px(20), Px(30), Px(40)};

    const auto root = Box(style);
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(root, 40, 10, 1860, 1040);
  }

  // the direction

  TEST_F(FlexLayoutEngineTest, PlacesBoxesInARowFromTheLeftByDefault)
  {
    const auto [root, a, b, c] = ThreeIn({});

    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(b, 100, 0, 100, 50);
    ExpectBox(c, 200, 0, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, PlacesBoxesInAColumnFromTheTop)
  {
    LayoutStyle style;
    style.flex_direction = FlexDirection::Column;
    const auto [root, a, b, c] = ThreeIn(style);

    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(b, 0, 50, 100, 50);
    ExpectBox(c, 0, 100, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, RowReverseStartsAtTheRight)
  {
    LayoutStyle style;
    style.flex_direction = FlexDirection::RowReverse;
    const auto [root, a, b, c] = ThreeIn(style);

    ExpectBox(a, 400, 0, 100, 50);
    ExpectBox(b, 300, 0, 100, 50);
    ExpectBox(c, 200, 0, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, ColumnReverseStartsAtTheBottom)
  {
    LayoutStyle style;
    style.flex_direction = FlexDirection::ColumnReverse;
    const auto [root, a, b, c] = ThreeIn(style);

    ExpectBox(a, 0, 150, 100, 50);
    ExpectBox(b, 0, 100, 100, 50);
    ExpectBox(c, 0, 50, 100, 50);
  }

  // justify_content, 8.2 of the specification

  struct JustifyCase
  {
    JustifyContent justify;
    float first;
    float second;
    float third;
  };

  class JustifyTest : public FlexLayoutEngineTest, public ::testing::WithParamInterface<JustifyCase> {};

  TEST_P(JustifyTest, PlacesThreeBoxesOf100InARowOf500)
  {
    LayoutStyle style;
    style.justify_content = GetParam().justify;
    const auto [root, a, b, c] = ThreeIn(style);

    ExpectBox(a, GetParam().first, 0, 100, 50);
    ExpectBox(b, GetParam().second, 0, 100, 50);
    ExpectBox(c, GetParam().third, 0, 100, 50);
  }

  TEST_P(JustifyTest, PlacesThreeBoxesOf50InAColumnOf350)
  {
    // 200 are left over, as in the row
    LayoutStyle style;
    style.flex_direction = FlexDirection::Column;
    style.justify_content = GetParam().justify;
    const auto [root, a, b, c] = ThreeIn(style, 500, 350);

    // the boxes are half as long along the direction as in the row
    ExpectBox(a, 0, GetParam().first, 100, 50);
    ExpectBox(b, 0, GetParam().second - 50, 100, 50);
    ExpectBox(c, 0, GetParam().third - 100, 100, 50);
  }

  INSTANTIATE_TEST_SUITE_P(EveryValue, JustifyTest, ::testing::Values(
    JustifyCase{JustifyContent::FlexStart, 0, 100, 200},
    JustifyCase{JustifyContent::FlexEnd, 200, 300, 400},
    JustifyCase{JustifyContent::Center, 100, 200, 300},
    JustifyCase{JustifyContent::SpaceBetween, 0, 200, 400},
    JustifyCase{JustifyContent::SpaceAround, 33.333f, 200, 366.667f},
    JustifyCase{JustifyContent::SpaceEvenly, 50, 200, 350}));

  TEST_F(FlexLayoutEngineTest, JustifyFollowsADirectionThatIsReversed)
  {
    LayoutStyle style;
    style.flex_direction = FlexDirection::RowReverse;
    style.justify_content = JustifyContent::FlexEnd;
    const auto [root, a, b, c] = ThreeIn(style);

    // the end of a reversed row is its left
    ExpectBox(a, 200, 0, 100, 50);
    ExpectBox(b, 100, 0, 100, 50);
    ExpectBox(c, 0, 0, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, SpaceBetweenWithOneBoxIsFlexStart)
  {
    LayoutStyle style = Sized(500, 200);
    style.justify_content = JustifyContent::SpaceBetween;

    const auto a = Box(Sized(100, 50));
    const auto root = Box(style, {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, SpaceAroundIsCenterWhenTheBoxesDoNotFit)
  {
    LayoutStyle style = Sized(200, 200);
    style.justify_content = JustifyContent::SpaceAround;

    LayoutStyle rigid = Sized(150, 50);
    rigid.flex_shrink = 0;

    const auto a = Box(rigid);
    const auto b = Box(rigid);
    const auto root = Box(style, {a, b});
    _engine.Calculate(root, 1920, 1080);

    // 100 are missing, half of them on each side
    ExpectBox(a, -50, 0, 150, 50);
    ExpectBox(b, 100, 0, 150, 50);
  }

  // align_items and align_self, 8.3

  struct AlignCase
  {
    AlignItems align;
    float position;
    float size;
  };

  class AlignTest : public FlexLayoutEngineTest, public ::testing::WithParamInterface<AlignCase> {};

  TEST_P(AlignTest, PlacesABoxAcrossARow)
  {
    LayoutStyle style = Sized(500, 200);
    style.align_items = GetParam().align;

    // without a height, so that it can stretch
    LayoutStyle child = Wide(100);
    child.min_height = Px(50);

    const auto a = Box(child);
    const auto root = Box(style, {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, GetParam().position, 100, GetParam().size);
  }

  TEST_P(AlignTest, PlacesABoxAcrossAColumn)
  {
    LayoutStyle style = Sized(200, 500);
    style.flex_direction = FlexDirection::Column;
    style.align_items = GetParam().align;

    LayoutStyle child;
    child.height = Px(100);
    child.min_width = Px(50);

    const auto a = Box(child);
    const auto root = Box(style, {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, GetParam().position, 0, GetParam().size, 100);
  }

  INSTANTIATE_TEST_SUITE_P(EveryValue, AlignTest, ::testing::Values(
    AlignCase{AlignItems::Stretch, 0, 200},
    AlignCase{AlignItems::FlexStart, 0, 50},
    AlignCase{AlignItems::FlexEnd, 150, 50},
    AlignCase{AlignItems::Center, 75, 50}));

  TEST_F(FlexLayoutEngineTest, ABoxWithAHeightDoesNotStretch)
  {
    const auto [root, a, b, c] = ThreeIn({});

    ExpectBox(a, 0, 0, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, AlignSelfWinsOverAlignItems)
  {
    LayoutStyle style = Sized(500, 200);
    style.align_items = AlignItems::FlexStart;

    LayoutStyle at_the_end = Sized(100, 50);
    at_the_end.align_self = AlignSelf::FlexEnd;

    LayoutStyle in_the_middle = Sized(100, 50);
    in_the_middle.align_self = AlignSelf::Center;

    LayoutStyle stretched = Wide(100);
    stretched.align_self = AlignSelf::Stretch;

    const auto a = Box(Sized(100, 50));
    const auto b = Box(at_the_end);
    const auto c = Box(in_the_middle);
    const auto d = Box(stretched);
    const auto root = Box(style, {a, b, c, d});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(b, 100, 150, 100, 50);
    ExpectBox(c, 200, 75, 100, 50);
    ExpectBox(d, 300, 0, 100, 200);
  }

  TEST_F(FlexLayoutEngineTest, AStretchedBoxStaysWithinItsLimits)
  {
    LayoutStyle child = Wide(100);
    child.max_height = Px(120);

    const auto a = Box(child);
    const auto root = Box(Sized(500, 200), {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 100, 120);
  }

  // flex_grow, flex_shrink, flex_basis, 9.7

  LayoutStyle Flexible(const float basis, const float grow, const float shrink = 1.0f)
  {
    LayoutStyle style;
    style.flex_basis = Px(basis);
    style.flex_grow = grow;
    style.flex_shrink = shrink;
    style.height = Px(50);
    return style;
  }

  TEST_F(FlexLayoutEngineTest, HandsOutWhatIsLeftOverByTheGrowFactors)
  {
    const auto a = Box(Flexible(100, 1));
    const auto b = Box(Flexible(100, 2));
    const auto c = Box(Flexible(100, 1));
    const auto root = Box(Sized(600, 200), {a, b, c});
    _engine.Calculate(root, 1920, 1080);

    // 300 are left over, in the parts 1, 2, and 1
    ExpectBox(a, 0, 0, 175, 50);
    ExpectBox(b, 175, 0, 250, 50);
    ExpectBox(c, 425, 0, 175, 50);
  }

  TEST_F(FlexLayoutEngineTest, ABoxThatDoesNotGrowKeepsItsSize)
  {
    const auto a = Box(Flexible(100, 0));
    const auto b = Box(Flexible(100, 1));
    const auto root = Box(Sized(600, 200), {a, b});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(b, 100, 0, 500, 50);
  }

  TEST_F(FlexLayoutEngineTest, GrowFactorsBelowOneHandOutThatPartOnly)
  {
    const auto a = Box(Flexible(100, 0.5f));
    const auto root = Box(Sized(500, 200), {a});
    _engine.Calculate(root, 1920, 1080);

    // half of the 400 that are left over
    ExpectBox(a, 0, 0, 300, 50);
  }

  TEST_F(FlexLayoutEngineTest, ABoxThatGrowsStopsAtItsMaxWidth)
  {
    LayoutStyle limited = Flexible(0, 1);
    limited.max_width = Px(100);

    const auto a = Box(limited);
    const auto b = Box(Flexible(0, 1));
    const auto c = Box(Flexible(0, 1));
    const auto root = Box(Sized(600, 200), {a, b, c});
    _engine.Calculate(root, 1920, 1080);

    // what the first could not take goes to the other two
    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(b, 100, 0, 250, 50);
    ExpectBox(c, 350, 0, 250, 50);
  }

  TEST_F(FlexLayoutEngineTest, ShrinksByTheFactorTimesTheSize)
  {
    const auto a = Box(Flexible(200, 0, 1));
    const auto b = Box(Flexible(400, 0, 1));
    const auto root = Box(Sized(300, 200), {a, b});
    _engine.Calculate(root, 1920, 1080);

    // 300 are missing. The box that is twice as large gives up twice as much
    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(b, 100, 0, 200, 50);
  }

  TEST_F(FlexLayoutEngineTest, ShrinkFactorsWeighWhatABoxGivesUp)
  {
    const auto a = Box(Flexible(300, 0, 1));
    const auto b = Box(Flexible(300, 0, 3));
    const auto root = Box(Sized(400, 200), {a, b});
    _engine.Calculate(root, 1920, 1080);

    // 200 are missing, in the parts 300 * 1 and 300 * 3
    ExpectBox(a, 0, 0, 250, 50);
    ExpectBox(b, 250, 0, 150, 50);
  }

  TEST_F(FlexLayoutEngineTest, ABoxThatDoesNotShrinkSticksOut)
  {
    const auto a = Box(Flexible(300, 0, 0));
    const auto b = Box(Flexible(300, 0, 0));
    const auto root = Box(Sized(400, 200), {a, b});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 300, 50);
    ExpectBox(b, 300, 0, 300, 50);
  }

  TEST_F(FlexLayoutEngineTest, ABoxThatShrinksStopsAtItsMinWidth)
  {
    LayoutStyle limited = Flexible(200, 0, 1);
    limited.min_width = Px(180);

    const auto a = Box(limited);
    const auto b = Box(Flexible(200, 0, 1));
    const auto root = Box(Sized(300, 200), {a, b});
    _engine.Calculate(root, 1920, 1080);

    // what the first could not give up is taken from the second
    ExpectBox(a, 0, 0, 180, 50);
    ExpectBox(b, 180, 0, 120, 50);
  }

  TEST_F(FlexLayoutEngineTest, FlexBasisWinsOverWidth)
  {
    LayoutStyle style = Sized(100, 50);
    style.flex_basis = Px(250);

    const auto a = Box(style);
    const auto root = Box(Sized(600, 200), {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 250, 50);
  }

  TEST_F(FlexLayoutEngineTest, FlexBasisInPercentRefersToTheContainer)
  {
    LayoutStyle style;
    style.flex_basis = Percent(25);
    style.height = Px(50);

    const auto a = Box(style);
    const auto root = Box(Sized(600, 200), {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 150, 50);
  }

  TEST_F(FlexLayoutEngineTest, FlexBasisInAColumnIsAHeight)
  {
    LayoutStyle container = Sized(200, 600);
    container.flex_direction = FlexDirection::Column;

    const auto a = Box(Flexible(100, 1));
    const auto b = Box(Flexible(100, 3));
    const auto root = Box(container, {a, b});
    _engine.Calculate(root, 1920, 1080);

    // `height` is what runs along a column, and the basis wins over it
    ExpectBox(a, 0, 0, 200, 200);
    ExpectBox(b, 0, 200, 200, 400);
  }

  // flex_wrap and align_content, 9.3 and 8.4

  TEST_F(FlexLayoutEngineTest, DoesNotWrapByDefault)
  {
    const auto [root, a, b, c] = ThreeIn({}, 250, 200);

    // they shrink to fit
    ExpectBox(a, 0, 0, 83.333f, 50);
    ExpectBox(b, 83.333f, 0, 83.333f, 50);
    ExpectBox(c, 166.667f, 0, 83.333f, 50);
  }

  TEST_F(FlexLayoutEngineTest, WrapStartsANewLineWhenABoxDoesNotFit)
  {
    LayoutStyle style;
    style.flex_wrap = FlexWrap::Wrap;
    style.align_content = AlignContent::FlexStart;
    const auto [root, a, b, c] = ThreeIn(style, 250, 200);

    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(b, 100, 0, 100, 50);
    ExpectBox(c, 0, 50, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, ABoxThatFitsExactlyStaysOnTheLine)
  {
    LayoutStyle style;
    style.flex_wrap = FlexWrap::Wrap;
    style.align_content = AlignContent::FlexStart;
    const auto [root, a, b, c] = ThreeIn(style, 300, 200);

    ExpectBox(c, 200, 0, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, LinesStretchToFillTheContainerByDefault)
  {
    LayoutStyle style;
    style.flex_wrap = FlexWrap::Wrap;
    const auto [root, a, b, c] = ThreeIn(style, 250, 200);

    // two lines of 100 each. The boxes have a height and keep it
    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(c, 0, 100, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, AlignContentPlacesTheLines)
  {
    LayoutStyle style;
    style.flex_wrap = FlexWrap::Wrap;

    style.align_content = AlignContent::FlexEnd;
    auto three = ThreeIn(style, 250, 200);
    ExpectBox(three.a, 0, 100, 100, 50);
    ExpectBox(three.c, 0, 150, 100, 50);

    style.align_content = AlignContent::Center;
    three = ThreeIn(style, 250, 200);
    ExpectBox(three.a, 0, 50, 100, 50);
    ExpectBox(three.c, 0, 100, 100, 50);

    style.align_content = AlignContent::SpaceBetween;
    three = ThreeIn(style, 250, 200);
    ExpectBox(three.a, 0, 0, 100, 50);
    ExpectBox(three.c, 0, 150, 100, 50);

    style.align_content = AlignContent::SpaceAround;
    three = ThreeIn(style, 250, 200);
    ExpectBox(three.a, 0, 25, 100, 50);
    ExpectBox(three.c, 0, 125, 100, 50);

    style.align_content = AlignContent::SpaceEvenly;
    three = ThreeIn(style, 250, 200);
    ExpectBox(three.a, 0, 33.333f, 100, 50);
    ExpectBox(three.c, 0, 116.667f, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, WrapReverseStacksTheLinesFromTheOtherSide)
  {
    LayoutStyle style;
    style.flex_wrap = FlexWrap::WrapReverse;
    style.align_content = AlignContent::FlexStart;
    const auto [root, a, b, c] = ThreeIn(style, 250, 200);

    ExpectBox(a, 0, 150, 100, 50);
    ExpectBox(b, 100, 150, 100, 50);
    ExpectBox(c, 0, 100, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, AContainerThatWrapsIsAsHighAsItsLines)
  {
    LayoutStyle style;
    style.width = Px(250);
    style.flex_wrap = FlexWrap::Wrap;
    // held at the top of its parent, so that it does not stretch
    style.align_self = AlignSelf::FlexStart;

    const auto a = Box(Sized(100, 50));
    const auto b = Box(Sized(100, 50));
    const auto c = Box(Sized(100, 50));
    const auto container = Box(style, {a, b, c});
    const auto root = Box({}, {container});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(container, 0, 0, 250, 100);
  }

  TEST_F(FlexLayoutEngineTest, EachLineHandsOutItsOwnLeftOver)
  {
    LayoutStyle style;
    style.flex_wrap = FlexWrap::Wrap;
    style.align_content = AlignContent::FlexStart;
    style.width = Px(250);
    style.height = Px(200);

    const auto a = Box(Flexible(100, 1));
    const auto b = Box(Flexible(100, 1));
    const auto c = Box(Flexible(100, 1));
    const auto root = Box(style, {a, b, c});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 125, 50);
    ExpectBox(b, 125, 0, 125, 50);
    ExpectBox(c, 0, 50, 250, 50);
  }

  // gap

  TEST_F(FlexLayoutEngineTest, ColumnGapLiesBetweenTheBoxesOfARow)
  {
    LayoutStyle style;
    style.column_gap = 10;
    style.row_gap = 99;
    const auto [root, a, b, c] = ThreeIn(style);

    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(b, 110, 0, 100, 50);
    ExpectBox(c, 220, 0, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, RowGapLiesBetweenTheBoxesOfAColumn)
  {
    LayoutStyle style;
    style.flex_direction = FlexDirection::Column;
    style.row_gap = 10;
    style.column_gap = 99;
    const auto [root, a, b, c] = ThreeIn(style);

    ExpectBox(b, 0, 60, 100, 50);
    ExpectBox(c, 0, 120, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, TheGapIsNotPartOfWhatIsHandedOut)
  {
    LayoutStyle style;
    style.column_gap = 20;
    style.justify_content = JustifyContent::SpaceBetween;
    const auto [root, a, b, c] = ThreeIn(style);

    // 160 are left over after the gaps, 80 go between each two boxes
    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(b, 200, 0, 100, 50);
    ExpectBox(c, 400, 0, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, TheGapCountsWhenLinesAreBroken)
  {
    LayoutStyle style;
    style.flex_wrap = FlexWrap::Wrap;
    style.align_content = AlignContent::FlexStart;
    style.column_gap = 10;
    style.row_gap = 5;
    const auto [root, a, b, c] = ThreeIn(style, 205, 200);

    // two boxes and their gap need 210
    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(b, 0, 55, 100, 50);
    ExpectBox(c, 0, 110, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, TheGapIsTakenBeforeBoxesGrow)
  {
    LayoutStyle style = Sized(620, 200);
    style.column_gap = 20;

    const auto a = Box(Flexible(0, 1));
    const auto b = Box(Flexible(0, 1));
    const auto root = Box(style, {a, b});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 300, 50);
    ExpectBox(b, 320, 0, 300, 50);
  }

  // percent

  TEST_F(FlexLayoutEngineTest, PercentRefersToTheContentBoxOfTheParent)
  {
    LayoutStyle container = Sized(400, 200);
    container.padding = {Px(10), Px(10), Px(10), Px(10)};

    LayoutStyle child;
    child.width = Percent(50);
    child.height = Percent(25);

    const auto a = Box(child);
    const auto root = Box(container, {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 10, 10, 200, 50);
  }

  TEST_F(FlexLayoutEngineTest, PercentOfAHeightThatFollowsFromContentIsAuto)
  {
    LayoutStyle container = Wide(400);
    container.align_self = AlignSelf::FlexStart;

    LayoutStyle child = Wide(100);
    child.height = Percent(50);

    const auto a = Box(child);
    const auto b = Box(Sized(100, 80));
    const auto container_node = Box(container, {a, b});
    const auto root = Box({}, {container_node});
    _engine.Calculate(root, 1920, 1080);

    // the container is as high as the box that has a height, and the other
    // one stretches
    ExpectBox(container_node, 0, 0, 400, 80);
    ExpectBox(a, 0, 0, 100, 80);
  }

  TEST_F(FlexLayoutEngineTest, MinAndMaxInPercent)
  {
    LayoutStyle child = Flexible(0, 1);
    child.max_width = Percent(50);

    LayoutStyle other = Sized(10, 50);
    other.min_width = Percent(25);

    const auto a = Box(child);
    const auto b = Box(other);
    const auto root = Box(Sized(400, 200), {a, b});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 200, 50);
    ExpectBox(b, 200, 0, 100, 50);
  }

  // the box model

  TEST_F(FlexLayoutEngineTest, PaddingAndBorderAreAddedToAContentBox)
  {
    LayoutStyle style = Sized(100, 50);
    style.padding = {Px(10), Px(10), Px(10), Px(10)};
    style.border = {5, 5, 5, 5};
    style.flex_shrink = 0;

    const auto a = Box(style);
    const auto root = Box(Sized(500, 200), {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 130, 80);

    const LayoutBox box = _engine.GetBox(a);
    EXPECT_FLOAT_EQ(box.padding.left, 10);
    EXPECT_FLOAT_EQ(box.border.top, 5);
  }

  TEST_F(FlexLayoutEngineTest, PaddingAndBorderArePartOfABorderBox)
  {
    LayoutStyle style = Sized(100, 50);
    style.box_sizing = LayoutBoxSizing::BorderBox;
    style.padding = {Px(10), Px(10), Px(10), Px(10)};
    style.border = {5, 5, 5, 5};

    const auto inside = Box({});
    const auto a = Box(style, {inside});
    const auto root = Box(Sized(500, 200), {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(inside, 15, 15, 0, 20);
  }

  TEST_F(FlexLayoutEngineTest, ABorderBoxIsNeverSmallerThanWhatIsAroundItsContent)
  {
    LayoutStyle style = Sized(10, 10);
    style.box_sizing = LayoutBoxSizing::BorderBox;
    style.padding = {Px(10), Px(10), Px(10), Px(10)};

    const auto a = Box(style);
    const auto root = Box(Sized(500, 200), {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 20, 20);
  }

  TEST_F(FlexLayoutEngineTest, ChildrenStartInsideBorderAndPadding)
  {
    LayoutStyle container = Sized(500, 200);
    container.padding = {Px(10), Px(20), Px(30), Px(40)};
    container.border = {1, 2, 3, 4};

    const auto a = Box(Wide(100));
    const auto root = Box(container, {a});
    _engine.Calculate(root, 1920, 1080);

    // top right bottom left, as the shorthand of CSS
    ExpectBox(root, 0, 0, 566, 244);
    ExpectBox(a, 44, 11, 100, 200);
  }

  TEST_F(FlexLayoutEngineTest, MarginsKeepBoxesApart)
  {
    LayoutStyle first = Sized(100, 50);
    first.margin = {Px(5), Px(10), Px(15), Px(20)};

    LayoutStyle second = Sized(100, 50);
    second.margin = {Px(0), Px(0), Px(0), Px(30)};

    const auto a = Box(first);
    const auto b = Box(second);
    const auto root = Box(Sized(500, 200), {a, b});
    _engine.Calculate(root, 1920, 1080);

    // margins of neighbors add up, they do not collapse in a flex container
    ExpectBox(a, 20, 5, 100, 50);
    ExpectBox(b, 160, 0, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, MarginsArePartOfWhatIsHandedOut)
  {
    LayoutStyle first = Flexible(0, 1);
    first.margin = {Px(0), Px(50), Px(0), Px(50)};

    const auto a = Box(first);
    const auto b = Box(Flexible(0, 1));
    const auto root = Box(Sized(500, 200), {a, b});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 50, 0, 200, 50);
    ExpectBox(b, 300, 0, 200, 50);
  }

  TEST_F(FlexLayoutEngineTest, AStretchedBoxLeavesRoomForItsMargins)
  {
    LayoutStyle style = Wide(100);
    style.margin = {Px(10), Px(0), Px(30), Px(0)};

    const auto a = Box(style);
    const auto root = Box(Sized(500, 200), {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 10, 100, 160);
  }

  TEST_F(FlexLayoutEngineTest, PaddingInPercentRefersToTheWidthOnAllSides)
  {
    LayoutStyle style;
    style.padding = {Percent(10), Percent(10), Percent(10), Percent(10)};
    style.align_self = AlignSelf::FlexStart;

    const auto a = Box(style);
    const auto root = Box(Sized(400, 200), {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 80, 80);
  }

  TEST_F(FlexLayoutEngineTest, AnAutoMarginTakesWhatIsLeftOver)
  {
    LayoutStyle pushed = Sized(100, 50);
    pushed.margin.left = LayoutLength::Auto();

    const auto a = Box(Sized(100, 50));
    const auto b = Box(pushed);
    const auto root = Box(Sized(500, 200), {a, b});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(b, 400, 0, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, AutoMarginsOnBothSidesPutABoxInTheMiddle)
  {
    LayoutStyle style = Sized(100, 50);
    style.margin = {LayoutLength::Auto(), LayoutLength::Auto(), LayoutLength::Auto(), LayoutLength::Auto()};

    const auto a = Box(style);
    const auto root = Box(Sized(500, 200), {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 200, 75, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, AutoMarginsWinOverJustifyContent)
  {
    LayoutStyle container = Sized(500, 200);
    container.justify_content = JustifyContent::Center;

    LayoutStyle pushed = Sized(100, 50);
    pushed.margin.right = LayoutLength::Auto();

    const auto a = Box(pushed);
    const auto root = Box(container, {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 100, 50);
  }

  // the size of a container that follows from its content

  TEST_F(FlexLayoutEngineTest, AColumnIsAsHighAsItsContent)
  {
    LayoutStyle container;
    container.flex_direction = FlexDirection::Column;
    container.padding = {Px(10), Px(10), Px(10), Px(10)};
    container.row_gap = 5;
    container.align_self = AlignSelf::FlexStart;

    const auto a = Box(Sized(100, 30));
    const auto b = Box(Sized(150, 40));
    const auto container_node = Box(container, {a, b});
    const auto root = Box({}, {container_node});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(container_node, 0, 0, 170, 95);
    ExpectBox(a, 10, 10, 100, 30);
    ExpectBox(b, 10, 45, 150, 40);
  }

  TEST_F(FlexLayoutEngineTest, ARowIsAsWideAsItsContent)
  {
    LayoutStyle container;
    container.column_gap = 5;
    container.align_self = AlignSelf::FlexStart;

    const auto a = Box(Sized(100, 30));
    const auto b = Box(Sized(150, 40));
    const auto container_node = Box(container, {a, b});

    LayoutStyle column;
    column.flex_direction = FlexDirection::Column;
    column.align_items = AlignItems::FlexStart;

    const auto root = Box(column, {container_node});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(container_node, 0, 0, 255, 40);
  }

  TEST_F(FlexLayoutEngineTest, AContainerStaysWithinItsOwnLimits)
  {
    LayoutStyle container;
    container.flex_direction = FlexDirection::Column;
    container.min_height = Px(100);
    container.max_width = Px(80);
    container.align_self = AlignSelf::FlexStart;

    LayoutStyle rigid = Sized(100, 30);
    rigid.flex_shrink = 0;

    const auto a = Box(rigid);
    const auto container_node = Box(container, {a});
    const auto root = Box({}, {container_node});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(container_node, 0, 0, 80, 100);
  }

  TEST_F(FlexLayoutEngineTest, ContainersInsideContainers)
  {
    LayoutStyle outer = Sized(600, 400);
    outer.flex_direction = FlexDirection::Column;
    outer.padding = {Px(20), Px(20), Px(20), Px(20)};

    LayoutStyle bar;
    bar.height = Px(50);
    bar.justify_content = JustifyContent::SpaceBetween;

    LayoutStyle body;
    body.flex_grow = 1;
    body.justify_content = JustifyContent::Center;
    body.align_items = AlignItems::Center;

    const auto left = Box(Sized(40, 40));
    const auto right = Box(Sized(40, 40));
    const auto bar_node = Box(bar, {left, right});
    const auto middle = Box(Sized(100, 100));
    const auto body_node = Box(body, {middle});
    const auto root = Box(outer, {bar_node, body_node});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(root, 0, 0, 640, 440);
    ExpectBox(bar_node, 20, 20, 600, 50);
    ExpectBox(left, 0, 0, 40, 40);
    ExpectBox(right, 560, 0, 40, 40);
    ExpectBox(body_node, 20, 70, 600, 350);
    ExpectBox(middle, 250, 125, 100, 100);
  }

  // content with a size of its own

  TEST_F(FlexLayoutEngineTest, ATextIsAsLargeAsItIsWhenThereIsRoom)
  {
    LayoutStyle container = Sized(500, 200);
    container.align_items = AlignItems::FlexStart;

    const auto text = Text();
    const auto root = Box(container, {text});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(text, 0, 0, 300, 20);
  }

  TEST_F(FlexLayoutEngineTest, ATextInARowShrinksAndBreaksItsLines)
  {
    LayoutStyle container = Sized(150, 200);
    container.align_items = AlignItems::FlexStart;

    const auto text = Text();
    const auto root = Box(container, {text});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(text, 0, 0, 150, 40);
  }

  TEST_F(FlexLayoutEngineTest, ATextInAColumnBreaksItsLinesAtTheWidthOfTheColumn)
  {
    LayoutStyle container = Sized(100, 200);
    container.flex_direction = FlexDirection::Column;

    const auto text = Text();
    const auto below = Box(Sized(50, 10));
    const auto root = Box(container, {text, below});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(text, 0, 0, 100, 60);
    ExpectBox(below, 0, 60, 50, 10);
  }

  TEST_F(FlexLayoutEngineTest, PaddingIsAddedAroundAText)
  {
    LayoutStyle container = Sized(500, 200);
    container.align_items = AlignItems::FlexStart;

    LayoutStyle style;
    style.padding = {Px(8), Px(16), Px(8), Px(16)};

    const auto text = Text(style);
    const auto root = Box(container, {text});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(text, 0, 0, 332, 36);
  }

  TEST_F(FlexLayoutEngineTest, AContainerIsAsLargeAsTheTextInside)
  {
    LayoutStyle container;
    container.padding = {Px(10), Px(10), Px(10), Px(10)};

    LayoutStyle outer = Sized(800, 600);
    outer.align_items = AlignItems::FlexStart;

    const auto text = Text();
    const auto container_node = Box(container, {text});
    const auto root = Box(outer, {container_node});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(container_node, 0, 0, 320, 40);
    ExpectBox(text, 10, 10, 300, 20);
  }

  // position: absolute

  struct AbsoluteCase
  {
    const char *name;
    LayoutLength top;
    LayoutLength right;
    LayoutLength bottom;
    LayoutLength left;
    bool sized;
    float expected_left;
    float expected_top;
    float expected_width;
    float expected_height;
  };

  void PrintTo(const AbsoluteCase &value, std::ostream *out)
  {
    *out << value.name;
  }

  class AbsoluteTest : public FlexLayoutEngineTest, public ::testing::WithParamInterface<AbsoluteCase> {};

  TEST_P(AbsoluteTest, PlacesABoxOf100By50In400By300)
  {
    LayoutStyle style = Absolute();
    style.inset = {GetParam().top, GetParam().right, GetParam().bottom, GetParam().left};
    if (GetParam().sized)
    {
      style.width = Px(100);
      style.height = Px(50);
    }

    const auto a = Box(style);
    const auto root = Box(Sized(400, 300), {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(
      a,
      GetParam().expected_left,
      GetParam().expected_top,
      GetParam().expected_width,
      GetParam().expected_height);
  }

  const LayoutLength none = LayoutLength::Auto();

  INSTANTIATE_TEST_SUITE_P(EveryCombinationOfSides, AbsoluteTest, ::testing::Values(
    AbsoluteCase{"top left", Px(10), none, none, Px(20), true, 20, 10, 100, 50},
    AbsoluteCase{"top right", Px(10), Px(20), none, none, true, 280, 10, 100, 50},
    AbsoluteCase{"bottom left", none, none, Px(10), Px(20), true, 20, 240, 100, 50},
    AbsoluteCase{"bottom right", none, Px(20), Px(10), none, true, 280, 240, 100, 50},
    AbsoluteCase{"top", Px(10), none, none, none, true, 0, 10, 100, 50},
    AbsoluteCase{"right", none, Px(20), none, none, true, 280, 0, 100, 50},
    AbsoluteCase{"bottom", none, none, Px(10), none, true, 0, 240, 100, 50},
    AbsoluteCase{"left", none, none, none, Px(20), true, 20, 0, 100, 50},
    AbsoluteCase{"none", none, none, none, none, true, 0, 0, 100, 50},
    // with a size, left wins over right and top over bottom
    AbsoluteCase{"all four, with a size", Px(10), Px(20), Px(30), Px(40), true, 40, 10, 100, 50},
    // without one, the box fills what is between the sides
    AbsoluteCase{"all four, without a size", Px(10), Px(20), Px(30), Px(40), false, 40, 10, 340, 260},
    AbsoluteCase{"left and right", none, Px(20), none, Px(40), false, 40, 0, 340, 0},
    AbsoluteCase{"top and bottom", Px(10), none, Px(30), none, false, 0, 10, 0, 260},
    AbsoluteCase{"all four at zero", Px(0), Px(0), Px(0), Px(0), false, 0, 0, 400, 300},
    AbsoluteCase{"in percent", Percent(10), none, none, Percent(50), true, 200, 30, 100, 50},
    AbsoluteCase{"below zero", Px(-10), none, none, Px(-20), true, -20, -10, 100, 50}));

  TEST_F(FlexLayoutEngineTest, AnAbsoluteBoxIsPlacedAgainstThePaddingBox)
  {
    LayoutStyle container = Sized(400, 300);
    container.padding = {Px(10), Px(10), Px(10), Px(10)};
    container.border = {5, 5, 5, 5};

    LayoutStyle at_the_start = Absolute();
    at_the_start.inset.top = Px(0);
    at_the_start.inset.left = Px(0);
    at_the_start.width = Px(100);
    at_the_start.height = Px(50);

    LayoutStyle at_the_end = at_the_start;
    at_the_end.inset = {none, Px(0), Px(0), none};

    LayoutStyle half = Absolute();
    half.inset.top = Px(0);
    half.inset.left = Px(0);
    half.width = Percent(50);
    half.height = Percent(50);

    const auto a = Box(at_the_start);
    const auto b = Box(at_the_end);
    const auto c = Box(half);
    const auto root = Box(container, {a, b, c});
    _engine.Calculate(root, 1920, 1080);

    // the border box is 430 by 330, the padding box 420 by 320
    ExpectBox(a, 5, 5, 100, 50);
    ExpectBox(b, 325, 275, 100, 50);
    ExpectBox(c, 5, 5, 210, 160);
  }

  TEST_F(FlexLayoutEngineTest, AnAbsoluteBoxTakesNoRoomInTheFlow)
  {
    LayoutStyle absolute = Absolute();
    absolute.width = Px(100);
    absolute.height = Px(50);
    absolute.inset.top = Px(0);
    absolute.inset.right = Px(0);

    LayoutStyle container = Sized(500, 200);
    container.justify_content = JustifyContent::SpaceBetween;

    const auto a = Box(Sized(100, 50));
    const auto b = Box(absolute);
    const auto c = Box(Sized(100, 50));
    const auto root = Box(container, {a, b, c});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(c, 400, 0, 100, 50);
    ExpectBox(b, 400, 0, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, AnAbsoluteBoxThatNamesNoSideFollowsTheAlignmentOfItsParent)
  {
    LayoutStyle container = Sized(400, 300);
    container.justify_content = JustifyContent::Center;
    container.align_items = AlignItems::FlexEnd;
    container.padding = {Px(10), Px(10), Px(10), Px(10)};

    LayoutStyle absolute = Absolute();
    absolute.width = Px(100);
    absolute.height = Px(50);

    const auto a = Box(absolute);
    const auto root = Box(container, {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 160, 260, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, AnAbsoluteBoxIsAsLargeAsItsContent)
  {
    LayoutStyle absolute = Absolute();
    absolute.inset.bottom = Px(10);
    absolute.inset.right = Px(10);
    absolute.padding = {Px(5), Px(5), Px(5), Px(5)};

    const auto inside = Box(Sized(80, 30));
    const auto a = Box(absolute, {inside});
    const auto root = Box(Sized(400, 300), {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 300, 250, 90, 40);
    ExpectBox(inside, 5, 5, 80, 30);
  }

  TEST_F(FlexLayoutEngineTest, AnAbsoluteBoxKeepsItsMargins)
  {
    LayoutStyle absolute = Absolute();
    absolute.width = Px(100);
    absolute.height = Px(50);
    absolute.inset.top = Px(10);
    absolute.inset.left = Px(10);
    absolute.margin = {Px(5), Px(0), Px(0), Px(7)};

    const auto a = Box(absolute);
    const auto root = Box(Sized(400, 300), {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 17, 15, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, AutoMarginsPutAnAbsoluteBoxInTheMiddle)
  {
    LayoutStyle absolute = Absolute();
    absolute.width = Px(100);
    absolute.height = Px(50);
    absolute.inset = {Px(0), Px(0), Px(0), Px(0)};
    absolute.margin = {LayoutLength::Auto(), LayoutLength::Auto(), LayoutLength::Auto(), LayoutLength::Auto()};

    const auto a = Box(absolute);
    const auto root = Box(Sized(400, 300), {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 150, 125, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, AnAbsoluteBoxStaysWithinItsLimits)
  {
    LayoutStyle absolute = Absolute();
    absolute.inset = {Px(0), Px(0), Px(0), Px(0)};
    absolute.max_width = Px(200);
    absolute.min_height = Px(350);

    const auto a = Box(absolute);
    const auto root = Box(Sized(400, 300), {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 200, 350);
  }

  // position: relative

  TEST_F(FlexLayoutEngineTest, ARelativeBoxMovesFromWhereItWasPlaced)
  {
    LayoutStyle moved = Sized(100, 50);
    moved.inset.left = Px(10);
    moved.inset.top = Px(20);

    const auto a = Box(Sized(100, 50));
    const auto b = Box(moved);
    const auto c = Box(Sized(100, 50));
    const auto root = Box(Sized(500, 200), {a, b, c});
    _engine.Calculate(root, 1920, 1080);

    // the boxes around it stay where they are
    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(b, 110, 20, 100, 50);
    ExpectBox(c, 200, 0, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, RightAndBottomMoveARelativeBoxTheOtherWay)
  {
    LayoutStyle moved = Sized(100, 50);
    moved.inset.right = Px(10);
    moved.inset.bottom = Px(20);

    const auto a = Box(moved);
    const auto root = Box(Sized(500, 200), {a});
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, -10, -20, 100, 50);
  }

  // display: none

  TEST_F(FlexLayoutEngineTest, ABoxThatIsNotDisplayedTakesNoRoom)
  {
    LayoutStyle hidden = Sized(100, 50);
    hidden.display = LayoutDisplay::None;

    LayoutStyle container = Sized(500, 200);
    container.column_gap = 10;

    const auto inside = Box(Sized(10, 10));
    const auto a = Box(Sized(100, 50));
    const auto b = Box(hidden, {inside});
    const auto c = Box(Sized(100, 50));
    const auto root = Box(container, {a, b, c});
    _engine.Calculate(root, 1920, 1080);

    // there is one gap, not two
    ExpectBox(c, 110, 0, 100, 50);
    ExpectBox(b, 0, 0, 0, 0);
    ExpectBox(inside, 0, 0, 0, 0);
  }

  TEST_F(FlexLayoutEngineTest, ABoxIsPlacedAgainOnceItIsDisplayed)
  {
    LayoutStyle style = Sized(100, 50);
    style.display = LayoutDisplay::None;

    const auto a = Box(style);
    const auto b = Box(Sized(100, 50));
    const auto root = Box(Sized(500, 200), {a, b});
    _engine.Calculate(root, 1920, 1080);
    ExpectBox(b, 0, 0, 100, 50);

    style.display = LayoutDisplay::Flex;
    _engine.SetStyle(a, style);
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(b, 100, 0, 100, 50);
  }

  // several sizes of the room

  struct Room
  {
    float width;
    float height;
  };

  class RoomTest : public FlexLayoutEngineTest, public ::testing::WithParamInterface<Room> {};

  TEST_P(RoomTest, KeepsWhatIsPinnedToACornerAtItsCorner)
  {
    const auto [width, height] = GetParam();

    LayoutStyle top_left = Absolute();
    top_left.inset.top = Px(16);
    top_left.inset.left = Px(16);
    top_left.width = Px(200);
    top_left.height = Px(40);

    LayoutStyle bottom_right = top_left;
    bottom_right.inset = {none, Px(16), Px(16), none};

    LayoutStyle across = Absolute();
    across.inset = {none, Px(0), Px(0), Px(0)};
    across.height = Percent(10);

    const auto a = Box(top_left);
    const auto b = Box(bottom_right);
    const auto c = Box(across);
    const auto root = Box({}, {a, b, c});
    _engine.Calculate(root, width, height);

    ExpectBox(root, 0, 0, width, height);
    ExpectBox(a, 16, 16, 200, 40);
    ExpectBox(b, width - 216, height - 56, 200, 40);
    ExpectBox(c, 0, height * 0.9f, width, height * 0.1f);
  }

  INSTANTIATE_TEST_SUITE_P(SeveralSizes, RoomTest, ::testing::Values(
    Room{1920, 1080},
    Room{1280, 720},
    Room{1280, 1024},
    Room{3440, 1440},
    Room{1080, 1920}));

  // the boxes themselves

  TEST_F(FlexLayoutEngineTest, ABoxThatIsDestroyedLeavesItsParent)
  {
    const auto a = Box(Sized(100, 50));
    const auto b = Box(Sized(100, 50));
    const auto root = Box(Sized(500, 200), {a, b});

    _engine.DestroyNode(a);
    _engine.Calculate(root, 1920, 1080);

    ExpectBox(b, 0, 0, 100, 50);
    ExpectBox(a, 0, 0, 0, 0);
  }

  TEST_F(FlexLayoutEngineTest, ABoxIsInsideOneBoxOnly)
  {
    const auto a = Box(Sized(100, 50));
    const auto first = Box(Sized(500, 200), {a});
    const auto second = Box(Sized(500, 200), {a});

    const auto b = Box(Sized(100, 50));
    _engine.SetChildren(first, {b});
    _engine.Calculate(first, 1920, 1080);
    _engine.Calculate(second, 1920, 1080);

    ExpectBox(a, 0, 0, 100, 50);
    ExpectBox(b, 0, 0, 100, 50);
  }

  TEST_F(FlexLayoutEngineTest, TheNumberOfADestroyedBoxIsGivenOutAgain)
  {
    const auto a = _engine.CreateNode();
    _engine.DestroyNode(a);

    EXPECT_EQ(_engine.CreateNode(), a);
  }

  TEST_F(FlexLayoutEngineTest, ABoxThatDoesNotExistIsLeftAlone)
  {
    _engine.SetStyle(7, {});
    _engine.SetChildren(7, {});
    _engine.DestroyNode(7);
    _engine.Calculate(7, 100, 100);

    ExpectBox(7, 0, 0, 0, 0);
    ExpectBox(neon::No_Layout_Node, 0, 0, 0, 0);
  }
}
