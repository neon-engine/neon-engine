#include "css-values.hpp"

#include <string>
#include <vector>

#include <gtest/gtest.h>

// The notations are those of https://www.w3.org/TR/css-color-4/ and
// https://www.w3.org/TR/css-values-4/

namespace
{
  using neon::Color;
  using neon::LayoutLength;
  using neon::ParseCssColor;
  using neon::ParseCssLength;
  using neon::ParseCssNumber;
  using neon::SplitCssValues;

  void ExpectColor(
    const std::string &text,
    const float red,
    const float green,
    const float blue,
    const float alpha)
  {
    Color color{0.5f, 0.5f, 0.5f, 0.5f};
    ASSERT_TRUE(ParseCssColor(text, color)) << text;

    EXPECT_NEAR(color.r, red, 0.0001f) << text;
    EXPECT_NEAR(color.g, green, 0.0001f) << text;
    EXPECT_NEAR(color.b, blue, 0.0001f) << text;
    EXPECT_NEAR(color.a, alpha, 0.0001f) << text;
  }

  void ExpectNoColor(const std::string &text)
  {
    Color color{0.1f, 0.2f, 0.3f, 0.4f};
    EXPECT_FALSE(ParseCssColor(text, color)) << text;

    // what could not be read changes nothing
    EXPECT_FLOAT_EQ(color.r, 0.1f);
    EXPECT_FLOAT_EQ(color.a, 0.4f);
  }

  TEST(CssColor, ReadsSixDigits)
  {
    ExpectColor("#ff8000", 1.0f, 128.0f / 255.0f, 0.0f, 1.0f);
    ExpectColor("#000000", 0, 0, 0, 1);
    ExpectColor("#ffffff", 1, 1, 1, 1);
  }

  TEST(CssColor, ReadsEightDigitsWithAlphaLast)
  {
    ExpectColor("#ff800080", 1.0f, 128.0f / 255.0f, 0.0f, 128.0f / 255.0f);
    ExpectColor("#00000000", 0, 0, 0, 0);
  }

  TEST(CssColor, ReadsThreeDigitsAsEachDigitTwice)
  {
    ExpectColor("#f80", 1.0f, 136.0f / 255.0f, 0.0f, 1.0f);
    ExpectColor("#fff", 1, 1, 1, 1);
  }

  TEST(CssColor, ReadsFourDigitsWithAlphaLast)
  {
    ExpectColor("#f808", 1.0f, 136.0f / 255.0f, 0.0f, 136.0f / 255.0f);
  }

  TEST(CssColor, ReadsDigitsInEitherCase)
  {
    ExpectColor("#FF8000", 1.0f, 128.0f / 255.0f, 0.0f, 1.0f);
    ExpectColor("#Ff8000", 1.0f, 128.0f / 255.0f, 0.0f, 1.0f);
  }

  TEST(CssColor, ReadsRgbWithCommas)
  {
    ExpectColor("rgb(255, 128, 0)", 1.0f, 128.0f / 255.0f, 0.0f, 1.0f);
    ExpectColor("rgb(255,128,0)", 1.0f, 128.0f / 255.0f, 0.0f, 1.0f);
  }

  TEST(CssColor, ReadsRgbWithSpaces)
  {
    ExpectColor("rgb(255 128 0)", 1.0f, 128.0f / 255.0f, 0.0f, 1.0f);
    ExpectColor("rgb(255 128 0 / 0.5)", 1.0f, 128.0f / 255.0f, 0.0f, 0.5f);
    ExpectColor("rgb(255 128 0 / 50%)", 1.0f, 128.0f / 255.0f, 0.0f, 0.5f);
  }

  TEST(CssColor, ReadsRgba)
  {
    ExpectColor("rgba(255, 128, 0, 0.5)", 1.0f, 128.0f / 255.0f, 0.0f, 0.5f);
    ExpectColor("rgba(0, 0, 0, 0)", 0, 0, 0, 0);
    // the two names stand for the same
    ExpectColor("rgb(255, 128, 0, 0.5)", 1.0f, 128.0f / 255.0f, 0.0f, 0.5f);
    ExpectColor("rgba(255, 128, 0)", 1.0f, 128.0f / 255.0f, 0.0f, 1.0f);
  }

  TEST(CssColor, ReadsPercentages)
  {
    ExpectColor("rgb(100%, 50%, 0%)", 1.0f, 0.5f, 0.0f, 1.0f);
  }

  TEST(CssColor, KeepsValuesWithinTheirRange)
  {
    ExpectColor("rgb(300, -20, 0)", 1, 0, 0, 1);
    ExpectColor("rgba(0, 0, 0, 7)", 0, 0, 0, 1);
  }

  TEST(CssColor, ReadsTheNamesItKnows)
  {
    ExpectColor("transparent", 0, 0, 0, 0);
    ExpectColor("black", 0, 0, 0, 1);
    ExpectColor("white", 1, 1, 1, 1);
    ExpectColor("White", 1, 1, 1, 1);
  }

  TEST(CssColor, LeavesOutSpacesAround)
  {
    ExpectColor("  #ff8000  ", 1.0f, 128.0f / 255.0f, 0.0f, 1.0f);
    ExpectColor(" RGB( 255 , 128 , 0 ) ", 1.0f, 128.0f / 255.0f, 0.0f, 1.0f);
  }

  TEST(CssColor, RefusesWhatIsNoColour)
  {
    ExpectNoColor("");
    ExpectNoColor("#");
    ExpectNoColor("#ff");
    ExpectNoColor("#fffff");
    ExpectNoColor("#fffffff");
    ExpectNoColor("#fffffffff");
    ExpectNoColor("#ggg");
    ExpectNoColor("ff8000");
    ExpectNoColor("rgb(255, 128)");
    ExpectNoColor("rgb(255, 128, 0, 1, 1)");
    ExpectNoColor("rgb(red, 128, 0)");
    ExpectNoColor("rgb(255, 128, 0");
    ExpectNoColor("hsl(120, 50%, 50%)");
    ExpectNoColor("rebeccapurple");
  }

  void ExpectLength(const std::string &text, const LayoutLength &expected)
  {
    LayoutLength length = LayoutLength::Pixels(-1);
    ASSERT_TRUE(ParseCssLength(text, length)) << text;
    EXPECT_EQ(length, expected) << text;
  }

  TEST(CssLength, ReadsPixels)
  {
    ExpectLength("12px", LayoutLength::Pixels(12));
    ExpectLength("12.5px", LayoutLength::Pixels(12.5f));
    ExpectLength("-4px", LayoutLength::Pixels(-4));
    ExpectLength("0", LayoutLength::Pixels(0));
    ExpectLength("12PX", LayoutLength::Pixels(12));
  }

  TEST(CssLength, ANumberWithoutAUnitCountsAsPixels)
  {
    ExpectLength("12", LayoutLength::Pixels(12));
    ExpectLength(".5", LayoutLength::Pixels(0.5f));
  }

  TEST(CssLength, ReadsPercentages)
  {
    ExpectLength("50%", LayoutLength::Percent(50));
    ExpectLength("12.5%", LayoutLength::Percent(12.5f));
    ExpectLength("100%", LayoutLength::Percent(100));
  }

  TEST(CssLength, ReadsAuto)
  {
    ExpectLength("auto", LayoutLength::Auto());
    ExpectLength(" Auto ", LayoutLength::Auto());
  }

  TEST(CssLength, RefusesWhatIsNoLength)
  {
    LayoutLength length = LayoutLength::Pixels(7);

    for (const std::string text : {"", "px", "%", "12em", "12 px", "12pt", "wide", "1e", "12px 4px", "--4"})
    {
      EXPECT_FALSE(ParseCssLength(text, length)) << text;
    }

    EXPECT_EQ(length, LayoutLength::Pixels(7));
  }

  TEST(CssNumber, ReadsNumbers)
  {
    float number = 0;

    EXPECT_TRUE(ParseCssNumber("1.5", number));
    EXPECT_FLOAT_EQ(number, 1.5f);

    EXPECT_TRUE(ParseCssNumber("-3", number));
    EXPECT_FLOAT_EQ(number, -3);

    EXPECT_TRUE(ParseCssNumber("+3", number));
    EXPECT_FLOAT_EQ(number, 3);

    EXPECT_TRUE(ParseCssNumber(" 2 ", number));
    EXPECT_FLOAT_EQ(number, 2);
  }

  TEST(CssNumber, RefusesWhatIsNoNumber)
  {
    float number = 7;

    for (const std::string text : {"", "one", "1px", "1,5", "1 2", "inf", "nan"})
    {
      EXPECT_FALSE(ParseCssNumber(text, number)) << text;
    }

    EXPECT_FLOAT_EQ(number, 7);
  }

  TEST(CssValues, AreSetApartBySpaces)
  {
    EXPECT_EQ(SplitCssValues("8px 16px"), (std::vector<std::string>{"8px", "16px"}));
    EXPECT_EQ(SplitCssValues("  1   2\t3\n4 "), (std::vector<std::string>{"1", "2", "3", "4"}));
    EXPECT_EQ(SplitCssValues("auto"), (std::vector<std::string>{"auto"}));
    EXPECT_TRUE(SplitCssValues("").empty());
    EXPECT_TRUE(SplitCssValues("   ").empty());
  }

  TEST(CssValues, StayTogetherBetweenBrackets)
  {
    EXPECT_EQ(
      SplitCssValues("2px solid rgb(255, 128, 0)"),
      (std::vector<std::string>{"2px", "solid", "rgb(255, 128, 0)"}));
  }
}
