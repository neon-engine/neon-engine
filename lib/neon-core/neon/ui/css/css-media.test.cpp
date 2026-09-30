#include "css-media.hpp"

#include <gtest/gtest.h>

// What is expected here is what https://www.w3.org/TR/mediaqueries-4/ says.

namespace
{
  using neon::CssEnvironment;
  using neon::CssMediaQueryList;

  /// A window of 1280 by 720 points on a display of the density 1.
  CssEnvironment Window(
    const float width = 1280.0f,
    const float height = 720.0f,
    const float resolution = 1.0f,
    const bool reduced_motion = false)
  {
    return {width, height, resolution, reduced_motion};
  }

  bool Holds(const std::string &text, const CssEnvironment &environment = Window())
  {
    CssMediaQueryList list;
    std::string error;
    EXPECT_TRUE(neon::ParseCssMedia(text, list, error)) << text << ": " << error;
    return list.Matches(environment);
  }

  std::string ProblemOf(const std::string &text)
  {
    CssMediaQueryList list;
    std::string error;
    EXPECT_FALSE(neon::ParseCssMedia(text, list, error)) << text;
    EXPECT_TRUE(list.queries.empty());
    return error;
  }

  TEST(CssMediaTest, HoldsAlwaysWithoutAQuery)
  {
    EXPECT_TRUE(Holds(""));
    EXPECT_TRUE(Holds("   "));
  }

  TEST(CssMediaTest, HoldsForAScreenAndNotForPrint)
  {
    EXPECT_TRUE(Holds("all"));
    EXPECT_TRUE(Holds("screen"));
    EXPECT_TRUE(Holds("only screen"));
    EXPECT_FALSE(Holds("print"));
    EXPECT_TRUE(Holds("not print"));
    EXPECT_FALSE(Holds("not screen"));
    EXPECT_TRUE(Holds("SCREEN"));
  }

  TEST(CssMediaTest, ComparesTheWidthWithTheLeastAndTheMost)
  {
    EXPECT_TRUE(Holds("(min-width: 1280px)"));
    EXPECT_TRUE(Holds("(min-width: 800px)"));
    EXPECT_FALSE(Holds("(min-width: 1281px)"));

    EXPECT_TRUE(Holds("(max-width: 1280px)"));
    EXPECT_TRUE(Holds("(max-width: 1920px)"));
    EXPECT_FALSE(Holds("(max-width: 1279px)"));

    EXPECT_TRUE(Holds("(width: 1280px)"));
    EXPECT_FALSE(Holds("(width: 1281px)"));
  }

  TEST(CssMediaTest, ComparesTheHeightWithTheLeastAndTheMost)
  {
    EXPECT_TRUE(Holds("(min-height: 720px)"));
    EXPECT_FALSE(Holds("(min-height: 721px)"));
    EXPECT_TRUE(Holds("(max-height: 720px)"));
    EXPECT_FALSE(Holds("(max-height: 719px)"));
    EXPECT_TRUE(Holds("(height: 720px)"));
  }

  TEST(CssMediaTest, CountsTheWindowInPointsAndNotInPixels)
  {
    // a Retina display shows 2880 pixels for the 1440 points of its window
    const CssEnvironment retina = Window(1440.0f, 900.0f, 2.0f);

    EXPECT_TRUE(Holds("(max-width: 1440px)", retina));
    EXPECT_FALSE(Holds("(min-width: 2880px)", retina));
  }

  TEST(CssMediaTest, TakesAnEmForSixteenPixels)
  {
    EXPECT_TRUE(Holds("(min-width: 80em)"));
    EXPECT_FALSE(Holds("(min-width: 80.5em)"));
    EXPECT_TRUE(Holds("(min-width: 80rem)"));
  }

  TEST(CssMediaTest, TakesZeroWithoutAUnit)
  {
    EXPECT_TRUE(Holds("(min-width: 0)"));
  }

  TEST(CssMediaTest, TellsLandscapeFromPortrait)
  {
    EXPECT_TRUE(Holds("(orientation: landscape)"));
    EXPECT_FALSE(Holds("(orientation: portrait)"));

    EXPECT_TRUE(Holds("(orientation: portrait)", Window(720.0f, 1280.0f)));
    EXPECT_FALSE(Holds("(orientation: landscape)", Window(720.0f, 1280.0f)));
  }

  TEST(CssMediaTest, TakesASquareForAPortrait)
  {
    EXPECT_TRUE(Holds("(orientation: portrait)", Window(800.0f, 800.0f)));
    EXPECT_FALSE(Holds("(orientation: landscape)", Window(800.0f, 800.0f)));
  }

  TEST(CssMediaTest, ComparesTheDensityInEveryUnit)
  {
    const CssEnvironment retina = Window(1440.0f, 900.0f, 2.0f);

    EXPECT_TRUE(Holds("(min-resolution: 2dppx)", retina));
    EXPECT_TRUE(Holds("(min-resolution: 2x)", retina));
    EXPECT_TRUE(Holds("(min-resolution: 192dpi)", retina));
    EXPECT_FALSE(Holds("(min-resolution: 193dpi)", retina));
    EXPECT_TRUE(Holds("(resolution: 2dppx)", retina));
    EXPECT_FALSE(Holds("(max-resolution: 1.5dppx)", retina));

    EXPECT_FALSE(Holds("(min-resolution: 2dppx)"));
    EXPECT_TRUE(Holds("(max-resolution: 1dppx)"));
    EXPECT_TRUE(Holds("(min-resolution: 96dpi)"));

    // 96 to the inch are 37.8 to the centimetre
    EXPECT_TRUE(Holds("(min-resolution: 37dpcm)"));
    EXPECT_FALSE(Holds("(min-resolution: 38dpcm)"));
  }

  TEST(CssMediaTest, ComparesADensityOfOneAndAQuarter)
  {
    const CssEnvironment laptop = Window(1280.0f, 720.0f, 1.25f);

    EXPECT_TRUE(Holds("(min-resolution: 1.25dppx)", laptop));
    EXPECT_TRUE(Holds("(min-resolution: 120dpi)", laptop));
    EXPECT_FALSE(Holds("(min-resolution: 1.5dppx)", laptop));
  }

  TEST(CssMediaTest, AsksWhetherLessMotionIsWanted)
  {
    const CssEnvironment calm = Window(1280.0f, 720.0f, 1.0f, true);

    EXPECT_TRUE(Holds("(prefers-reduced-motion: reduce)", calm));
    EXPECT_FALSE(Holds("(prefers-reduced-motion: no-preference)", calm));
    EXPECT_TRUE(Holds("(prefers-reduced-motion)", calm));

    EXPECT_FALSE(Holds("(prefers-reduced-motion: reduce)"));
    EXPECT_TRUE(Holds("(prefers-reduced-motion: no-preference)"));
    EXPECT_FALSE(Holds("(prefers-reduced-motion)"));
  }

  TEST(CssMediaTest, ComparesTheRatioOfTheSides)
  {
    EXPECT_TRUE(Holds("(aspect-ratio: 16/9)"));
    EXPECT_TRUE(Holds("(aspect-ratio: 16 / 9)"));
    EXPECT_TRUE(Holds("(min-aspect-ratio: 4/3)"));
    EXPECT_FALSE(Holds("(max-aspect-ratio: 4/3)"));
    EXPECT_TRUE(Holds("(min-aspect-ratio: 21/9)", Window(3440.0f, 1440.0f)));
  }

  TEST(CssMediaTest, NeedsEveryPartOfAQueryToHold)
  {
    EXPECT_TRUE(Holds("screen and (min-width: 800px) and (orientation: landscape)"));
    EXPECT_FALSE(Holds("screen and (min-width: 800px) and (orientation: portrait)"));
    EXPECT_TRUE(Holds("(min-width: 800px) and (max-width: 1280px)"));
    EXPECT_FALSE(Holds("(min-width: 800px) and (max-width: 1000px)"));
    EXPECT_FALSE(Holds("print and (min-width: 800px)"));
  }

  TEST(CssMediaTest, NeedsOneQueryOfAListToHold)
  {
    EXPECT_TRUE(Holds("(max-width: 800px), (orientation: landscape)"));
    EXPECT_TRUE(Holds("(orientation: landscape), (max-width: 800px)"));
    EXPECT_FALSE(Holds("(max-width: 800px), (orientation: portrait)"));
    EXPECT_TRUE(Holds("print, screen"));
  }

  TEST(CssMediaTest, TurnsAWholeQueryAroundWithNot)
  {
    EXPECT_FALSE(Holds("not (min-width: 800px)"));
    EXPECT_TRUE(Holds("not (min-width: 1281px)"));
    EXPECT_TRUE(Holds("not screen and (min-width: 1281px)"));
    EXPECT_FALSE(Holds("not screen and (min-width: 800px)"));

    // each query of a list by itself
    EXPECT_TRUE(Holds("not screen, (min-width: 800px)"));
  }

  TEST(CssMediaTest, ComparesWithTheSignsOfARange)
  {
    EXPECT_TRUE(Holds("(width >= 1280px)"));
    EXPECT_FALSE(Holds("(width > 1280px)"));
    EXPECT_TRUE(Holds("(width <= 1280px)"));
    EXPECT_FALSE(Holds("(width < 1280px)"));
    EXPECT_TRUE(Holds("(width = 1280px)"));
    EXPECT_TRUE(Holds("(height<721px)"));
  }

  TEST(CssMediaTest, ReadsARangeWithTheValueInFront)
  {
    EXPECT_TRUE(Holds("(800px <= width)"));
    EXPECT_FALSE(Holds("(1281px <= width)"));
    EXPECT_TRUE(Holds("(1281px > width)"));
  }

  TEST(CssMediaTest, ReadsARangeWithTwoSides)
  {
    EXPECT_TRUE(Holds("(800px <= width <= 1280px)"));
    EXPECT_FALSE(Holds("(800px <= width < 1280px)"));
    EXPECT_FALSE(Holds("(1300px <= width <= 1920px)"));
    EXPECT_TRUE(Holds("(1920px >= width >= 800px)"));
  }

  TEST(CssMediaTest, AsksWhetherAFeatureIsThere)
  {
    EXPECT_TRUE(Holds("(width)"));
    EXPECT_FALSE(Holds("(width)", Window(0.0f, 0.0f)));
    EXPECT_TRUE(Holds("(orientation)"));
    EXPECT_TRUE(Holds("(resolution)"));
  }

  TEST(CssMediaTest, KeepsWhatWasWritten)
  {
    CssMediaQueryList list;
    std::string error;
    ASSERT_TRUE(neon::ParseCssMedia("  screen and (min-width: 800px) ", list, error));

    EXPECT_EQ(list.text, "screen and (min-width: 800px)");
    ASSERT_EQ(list.queries.size(), 1u);
    EXPECT_EQ(list.queries[0].type, "screen");
    ASSERT_EQ(list.queries[0].features.size(), 1u);
    EXPECT_EQ(list.queries[0].features[0].name, "width");
    EXPECT_EQ(list.queries[0].features[0].value, 800.0f);
  }

  TEST(CssMediaTest, SaysWhatIsWrongWithAQuery)
  {
    EXPECT_EQ(
      ProblemOf("(min-colour: 8)"),
      "'colour' is not a feature that is known. Known are: width, height, aspect-ratio, orientation, "
      "resolution, prefers-reduced-motion");
    EXPECT_EQ(
      ProblemOf("(colour)"),
      "'colour' is not a feature that is known. Known are: width, height, aspect-ratio, orientation, "
      "resolution, prefers-reduced-motion");
    EXPECT_EQ(ProblemOf("(min-width: wide)"), "'width' is 'wide', where a length such as 800px was expected");
    EXPECT_EQ(ProblemOf("(min-width: 800)"), "'width' is '800', where a length such as 800px was expected");
    EXPECT_EQ(ProblemOf("(min-width: 50%)"), "'width' is '50%', where a length such as 800px was expected");
    EXPECT_EQ(ProblemOf("(min-width: -1px)"), "'width' is '-1px', where a length such as 800px was expected");
    EXPECT_EQ(
      ProblemOf("(orientation: sideways)"),
      "'orientation' is 'sideways', where portrait or landscape was expected");
    EXPECT_EQ(
      ProblemOf("(min-orientation: landscape)"),
      "'orientation' has no least and no most, and is written without min- and max-");
    EXPECT_EQ(
      ProblemOf("(resolution: 2)"),
      "'resolution' is '2', where a density such as 2dppx, 2x, or 192dpi was expected");
    EXPECT_EQ(
      ProblemOf("(prefers-reduced-motion: yes)"),
      "'prefers-reduced-motion' is 'yes', where reduce or no-preference was expected");
    EXPECT_EQ(
      ProblemOf("(aspect-ratio: 16/0)"),
      "'aspect-ratio' is '16/0', where a ratio such as 16/9 was expected");
    EXPECT_EQ(
      ProblemOf("()"),
      "the brackets hold nothing, where a feature such as min-width: 800px was expected");
    EXPECT_EQ(ProblemOf("(min-width: 800px"), "a bracket is not closed");
    EXPECT_EQ(
      ProblemOf("(min-width: 800px) (max-width: 900px)"),
      "two parts of a query follow each other, where 'and' was expected between them");
    EXPECT_EQ(ProblemOf("and (min-width: 800px)"), "'and' has nothing in front of it");
    EXPECT_EQ(
      ProblemOf("screen and"),
      "a query ends with 'and', where a feature in brackets was expected behind it");
    EXPECT_EQ(ProblemOf("screen or (width)"), "'or' follows a part of a query, where 'and' was expected");
    EXPECT_EQ(
      ProblemOf("television"),
      "'television' cannot be read, where all, screen, print, or a feature in brackets was expected");
    EXPECT_EQ(ProblemOf("screen,"), "a query is empty");
    EXPECT_EQ(
      ProblemOf("not"),
      "a query names nothing, where all, screen, print, or a feature in brackets was expected");
    EXPECT_EQ(
      ProblemOf("(width >= )"),
      "'width >=' cannot be read, where a range such as width >= 800px was expected");
    EXPECT_EQ(
      ProblemOf("(800px <= 900px)"),
      "'800px <= 900px' names no feature that is known. Known are: width, height, aspect-ratio, "
      "orientation, resolution, prefers-reduced-motion");
    EXPECT_EQ(
      ProblemOf("(orientation >= landscape)"),
      "'orientation' has no least and no most, and is compared with a colon");
  }
} // namespace
