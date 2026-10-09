#include "css-functions.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
  using neon::LayoutLength;
  using neon::ParseCssAngle;
  using neon::ParseCssFunction;
  using neon::ParseCssGradient;
  using neon::ParseCssPlace;
  using neon::ParseCssShadows;
  using neon::ParseCssTransform;
  using neon::SplitCssList;
  using neon::UiGradient;
  using neon::UiPlace;
  using neon::UiShadow;
  using neon::UiTransformStep;

  TEST(SplitCssListTest, SplitsAtTheSeparatorOutsideBrackets)
  {
    EXPECT_EQ(
      SplitCssList("0 1px #000, 2px 2px rgba(0, 0, 0, 0.5) , inset 0 0 4px red", ','),
      (std::vector<std::string>{"0 1px #000", "2px 2px rgba(0, 0, 0, 0.5)", "inset 0 0 4px red"}));

    EXPECT_EQ(SplitCssList("one", ','), (std::vector<std::string>{"one"}));
    EXPECT_TRUE(SplitCssList("", ',').empty());
    EXPECT_EQ(SplitCssList("a,,b", ','), (std::vector<std::string>{"a", "", "b"}));
  }

  TEST(ParseCssFunctionTest, HandsOverWhatIsBetweenTheBrackets)
  {
    std::string inside;

    ASSERT_TRUE(ParseCssFunction("rotate( 45deg )", "rotate", inside));
    EXPECT_EQ(inside, "45deg");

    ASSERT_TRUE(ParseCssFunction("  translate(10px, calc(1 + 2))  ", "translate", inside));
    EXPECT_EQ(inside, "10px, calc(1 + 2)");

    EXPECT_FALSE(ParseCssFunction("rotate(45deg", "rotate", inside));
    EXPECT_FALSE(ParseCssFunction("rotate 45deg", "rotate", inside));
    EXPECT_FALSE(ParseCssFunction("rotateX(45deg)", "rotate", inside));
    EXPECT_FALSE(ParseCssFunction("rotate(1) scale(2)", "rotate", inside));
    EXPECT_FALSE(ParseCssFunction("", "rotate", inside));
  }

  TEST(ParseCssAngleTest, ReadsEveryUnitOfCss)
  {
    float degrees = -1.0f;

    ASSERT_TRUE(ParseCssAngle("45deg", degrees));
    EXPECT_FLOAT_EQ(degrees, 45);

    ASSERT_TRUE(ParseCssAngle("-0.25turn", degrees));
    EXPECT_FLOAT_EQ(degrees, -90);

    ASSERT_TRUE(ParseCssAngle("3.14159265rad", degrees));
    EXPECT_NEAR(degrees, 180, 0.001f);

    ASSERT_TRUE(ParseCssAngle("100grad", degrees));
    EXPECT_FLOAT_EQ(degrees, 90);

    ASSERT_TRUE(ParseCssAngle("0", degrees));
    EXPECT_FLOAT_EQ(degrees, 0);

    EXPECT_FALSE(ParseCssAngle("45", degrees)) << "a number needs its unit, but for 0";
    EXPECT_FALSE(ParseCssAngle("deg", degrees));
    EXPECT_FALSE(ParseCssAngle("45px", degrees));
    EXPECT_FALSE(ParseCssAngle("", degrees));
  }

  TEST(ParseCssGradientTest, ReadsColorsFromTopToBottom)
  {
    UiGradient gradient;
    ASSERT_TRUE(ParseCssGradient("linear-gradient(#ff0000, #0000ff)", gradient));

    EXPECT_EQ(gradient.kind, UiGradient::Kind::Linear);
    EXPECT_FLOAT_EQ(gradient.angle, 180) << "to the bottom, as in CSS";

    ASSERT_EQ(gradient.stops.size(), 2u);
    EXPECT_FLOAT_EQ(gradient.stops[0].color.r, 1);
    EXPECT_FLOAT_EQ(gradient.stops[0].position, 0);
    EXPECT_FLOAT_EQ(gradient.stops[1].color.b, 1);
    EXPECT_FLOAT_EQ(gradient.stops[1].position, 1);
  }

  TEST(ParseCssGradientTest, ReadsTheWayItRuns)
  {
    UiGradient gradient;

    ASSERT_TRUE(ParseCssGradient("linear-gradient(90deg, #f00, #00f)", gradient));
    EXPECT_FLOAT_EQ(gradient.angle, 90);

    ASSERT_TRUE(ParseCssGradient("linear-gradient(to right, #f00, #00f)", gradient));
    EXPECT_FLOAT_EQ(gradient.angle, 90);

    ASSERT_TRUE(ParseCssGradient("linear-gradient(to top, #f00, #00f)", gradient));
    EXPECT_FLOAT_EQ(gradient.angle, 0);

    ASSERT_TRUE(ParseCssGradient("linear-gradient(to left, #f00, #00f)", gradient));
    EXPECT_FLOAT_EQ(gradient.angle, 270);

    ASSERT_TRUE(ParseCssGradient("linear-gradient(to bottom right, #f00, #00f)", gradient));
    EXPECT_FLOAT_EQ(gradient.angle, 135);

    ASSERT_TRUE(ParseCssGradient("linear-gradient(to top left, #f00, #00f)", gradient));
    EXPECT_FLOAT_EQ(gradient.angle, 315);
  }

  TEST(ParseCssGradientTest, ReadsWhereTheColorsLie)
  {
    UiGradient gradient;
    ASSERT_TRUE(ParseCssGradient(
      "linear-gradient(to right, rgba(0, 0, 0, 0.5) 10%, #fff, #f00, transparent 70%, #00f)", gradient));

    ASSERT_EQ(gradient.stops.size(), 5u);
    EXPECT_FLOAT_EQ(gradient.stops[0].position, 0.1f);
    EXPECT_FLOAT_EQ(gradient.stops[0].color.a, 0.5f);

    // what has no place lies evenly between its neighbors
    EXPECT_FLOAT_EQ(gradient.stops[1].position, 0.3f);
    EXPECT_FLOAT_EQ(gradient.stops[2].position, 0.5f);
    EXPECT_FLOAT_EQ(gradient.stops[3].position, 0.7f);
    EXPECT_FLOAT_EQ(gradient.stops[4].position, 1.0f);
  }

  TEST(ParseCssGradientTest, AColorDoesNotLieInFrontOfTheOneBeforeIt)
  {
    UiGradient gradient;
    ASSERT_TRUE(ParseCssGradient("linear-gradient(#f00 50%, #0f0 20%, #00f)", gradient));

    EXPECT_FLOAT_EQ(gradient.stops[1].position, 0.5f);
  }

  TEST(ParseCssGradientTest, ReadsAGradientAroundTheMiddle)
  {
    UiGradient gradient;
    ASSERT_TRUE(ParseCssGradient("radial-gradient(#fff, #000 80%)", gradient));

    EXPECT_EQ(gradient.kind, UiGradient::Kind::Radial);
    ASSERT_EQ(gradient.stops.size(), 2u);
    EXPECT_FLOAT_EQ(gradient.stops[1].position, 0.8f);

    EXPECT_TRUE(ParseCssGradient("radial-gradient(ellipse, #fff, #000)", gradient));
  }

  TEST(ParseCssGradientTest, RefusesWhatIsNoGradient)
  {
    UiGradient gradient;
    gradient.angle = 7;

    for (const std::string text : {
           "", "#ff0000", "linear-gradient()", "linear-gradient(#f00)", "linear-gradient(90deg, #f00)",
           "linear-gradient(#f00, nocolor)", "linear-gradient(#f00 10px, #00f)", "linear-gradient(#f00, #00f",
           "conic-gradient(#f00, #00f)", "linear-gradient(to nowhere, #f00, #00f)",
           "linear-gradient(#000, #111, #222, #333, #444, #555, #666, #777, #888)"
         })
    {
      EXPECT_FALSE(ParseCssGradient(text, gradient)) << text;
    }

    EXPECT_FLOAT_EQ(gradient.angle, 7) << "and leaves what it was given alone";
  }

  TEST(ParseCssGradientTest, TakesUpToEightColors)
  {
    UiGradient gradient;
    EXPECT_TRUE(ParseCssGradient("linear-gradient(#000, #111, #222, #333, #444, #555, #666, #777)", gradient));
    EXPECT_EQ(gradient.stops.size(), 8u);
  }

  TEST(ParseCssShadowsTest, ReadsAShadowOfABox)
  {
    std::vector<UiShadow> shadows;
    ASSERT_TRUE(ParseCssShadows("2px 4px 12px 1px rgba(0, 0, 0, 0.5)", false, shadows));

    ASSERT_EQ(shadows.size(), 1u);
    EXPECT_FLOAT_EQ(shadows[0].offset_x, 2);
    EXPECT_FLOAT_EQ(shadows[0].offset_y, 4);
    EXPECT_FLOAT_EQ(shadows[0].blur, 12);
    EXPECT_FLOAT_EQ(shadows[0].spread, 1);
    EXPECT_FLOAT_EQ(shadows[0].color.a, 0.5f);
    EXPECT_TRUE(shadows[0].has_color);
    EXPECT_FALSE(shadows[0].is_inset);
  }

  TEST(ParseCssShadowsTest, ReadsSeveralShadowsInAnyOrderOfTheirParts)
  {
    std::vector<UiShadow> shadows;
    ASSERT_TRUE(ParseCssShadows("#ff0000 0 0 8px, inset -1 -2 #00f, 3 3 inset", false, shadows));

    ASSERT_EQ(shadows.size(), 3u);
    EXPECT_FLOAT_EQ(shadows[0].blur, 8);
    EXPECT_FLOAT_EQ(shadows[0].color.r, 1);

    EXPECT_TRUE(shadows[1].is_inset);
    EXPECT_FLOAT_EQ(shadows[1].offset_x, -1);
    EXPECT_FLOAT_EQ(shadows[1].offset_y, -2);
    EXPECT_FLOAT_EQ(shadows[1].blur, 0);

    EXPECT_TRUE(shadows[2].is_inset);
    EXPECT_FALSE(shadows[2].has_color) << "which leaves it the color of the text";
  }

  TEST(ParseCssShadowsTest, TheShadowOfATextHasNoInsetAndIsNoLarger)
  {
    std::vector<UiShadow> shadows;

    ASSERT_TRUE(ParseCssShadows("1px 1px 2px #000", true, shadows));
    EXPECT_FLOAT_EQ(shadows[0].blur, 2);

    EXPECT_FALSE(ParseCssShadows("inset 1px 1px #000", true, shadows));
    EXPECT_FALSE(ParseCssShadows("1px 1px 2px 3px #000", true, shadows));
  }

  TEST(ParseCssShadowsTest, NoneTakesTheShadowsAway)
  {
    std::vector<UiShadow> shadows(2);
    ASSERT_TRUE(ParseCssShadows("none", false, shadows));
    EXPECT_TRUE(shadows.empty());
  }

  TEST(ParseCssShadowsTest, RefusesWhatIsNoShadow)
  {
    std::vector<UiShadow> shadows(1);

    for (const std::string text : {
           "", "4px", "#000", "1px 2px 3px 4px 5px", "1px 2px -3px", "1px 2px #000 #fff", "1px 2px soft",
           "1px 2px,", "1px 50%", "inset inset 1px 2px"
         })
    {
      EXPECT_FALSE(ParseCssShadows(text, false, shadows)) << text;
    }

    EXPECT_EQ(shadows.size(), 1u);
  }

  TEST(ParseCssTransformTest, ReadsEveryStep)
  {
    std::vector<UiTransformStep> steps;
    ASSERT_TRUE(ParseCssTransform("translate(10px, 50%) rotate(45deg) scale(1.5)", steps));

    ASSERT_EQ(steps.size(), 3u);

    EXPECT_EQ(steps[0].kind, UiTransformStep::Kind::Translate);
    EXPECT_EQ(steps[0].x, LayoutLength::Pixels(10));
    EXPECT_EQ(steps[0].y, LayoutLength::Percent(50));

    EXPECT_EQ(steps[1].kind, UiTransformStep::Kind::Rotate);
    EXPECT_FLOAT_EQ(steps[1].angle, 45);

    EXPECT_EQ(steps[2].kind, UiTransformStep::Kind::Scale);
    EXPECT_FLOAT_EQ(steps[2].scale_x, 1.5f);
    EXPECT_FLOAT_EQ(steps[2].scale_y, 1.5f);
  }

  TEST(ParseCssTransformTest, ReadsTheStepsAlongOneSide)
  {
    std::vector<UiTransformStep> steps;
    ASSERT_TRUE(ParseCssTransform("translateX(-8) translateY(25%) scaleX(2) scaleY(0.5) scale(2, 3)", steps));

    ASSERT_EQ(steps.size(), 5u);
    EXPECT_EQ(steps[0].x, LayoutLength::Pixels(-8));
    EXPECT_EQ(steps[0].y, LayoutLength::Pixels(0));
    EXPECT_EQ(steps[1].y, LayoutLength::Percent(25));
    EXPECT_FLOAT_EQ(steps[2].scale_x, 2);
    EXPECT_FLOAT_EQ(steps[2].scale_y, 1);
    EXPECT_FLOAT_EQ(steps[3].scale_y, 0.5f);
    EXPECT_FLOAT_EQ(steps[4].scale_x, 2);
    EXPECT_FLOAT_EQ(steps[4].scale_y, 3);
  }

  TEST(ParseCssTransformTest, NoneHasNoSteps)
  {
    std::vector<UiTransformStep> steps(2);
    ASSERT_TRUE(ParseCssTransform("none", steps));
    EXPECT_TRUE(steps.empty());
  }

  TEST(ParseCssTransformTest, RefusesWhatIsNoTransform)
  {
    std::vector<UiTransformStep> steps(1);

    for (const std::string text : {
           "", "rotate", "rotate(45)", "rotate(45deg", "skew(10deg)", "translate()", "translate(1, 2, 3)",
           "translate(auto)", "scale(big)", "matrix(1, 0, 0, 1, 0, 0)", "rotate(45deg) nothing"
         })
    {
      EXPECT_FALSE(ParseCssTransform(text, steps)) << text;
    }

    EXPECT_EQ(steps.size(), 1u);
  }

  TEST(ParseCssPlaceTest, ReadsWordsLengthsAndPercentages)
  {
    UiPlace place;

    ASSERT_TRUE(ParseCssPlace("center", place));
    EXPECT_EQ(place.x, LayoutLength::Percent(50));
    EXPECT_EQ(place.y, LayoutLength::Percent(50));

    ASSERT_TRUE(ParseCssPlace("left top", place));
    EXPECT_EQ(place.x, LayoutLength::Percent(0));
    EXPECT_EQ(place.y, LayoutLength::Percent(0));

    ASSERT_TRUE(ParseCssPlace("bottom right", place));
    EXPECT_EQ(place.x, LayoutLength::Percent(100));
    EXPECT_EQ(place.y, LayoutLength::Percent(100));

    ASSERT_TRUE(ParseCssPlace("25% 10px", place));
    EXPECT_EQ(place.x, LayoutLength::Percent(25));
    EXPECT_EQ(place.y, LayoutLength::Pixels(10));

    ASSERT_TRUE(ParseCssPlace("12", place));
    EXPECT_EQ(place.x, LayoutLength::Pixels(12));
    EXPECT_EQ(place.y, LayoutLength::Percent(50)) << "one value leaves the other in the middle";

    ASSERT_TRUE(ParseCssPlace("top", place));
    EXPECT_EQ(place.x, LayoutLength::Percent(50));
    EXPECT_EQ(place.y, LayoutLength::Percent(0));

    ASSERT_TRUE(ParseCssPlace("top 20%", place));
    EXPECT_EQ(place.x, LayoutLength::Percent(20));
    EXPECT_EQ(place.y, LayoutLength::Percent(0));
  }

  TEST(ParseCssPlaceTest, RefusesWhatIsNoPlace)
  {
    UiPlace place;

    for (const std::string text : {"", "left right", "top bottom", "1 2 3", "middle", "auto", "left top 3"})
    {
      EXPECT_FALSE(ParseCssPlace(text, place)) << text;
    }
  }
}
