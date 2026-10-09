#include "css-expression.hpp"

#include <map>

#include <gtest/gtest.h>

// What is expected here is what https://www.w3.org/TR/css-values-4/,
// https://www.w3.org/TR/css-variables-1/, and
// https://www.w3.org/TR/css-color-4/ say.

namespace
{
  using neon::CssValueContext;
  using neon::CssValueUses;

  class CssExpressionTest : public ::testing::Test
  {
  protected:
    std::map<std::string, std::string> _variables;
    CssValueContext _context;

    void SetUp() override
    {
      // a font of 20 in a document whose root has one of 10, shown at 1000
      // by 500
      _context.font_size = 20.0f;
      _context.root_font_size = 10.0f;
      _context.viewport_width = 1000.0f;
      _context.viewport_height = 500.0f;

      _context.variable = [this](const std::string &name, std::string &value)
      {
        const auto found = _variables.find(name);
        if (found == _variables.end()) { return false; }

        value = found->second;
        return true;
      };
    }

    std::string Resolve(const std::string &value, CssValueUses *uses = nullptr)
    {
      std::string resolved;
      std::string error;
      EXPECT_TRUE(neon::ResolveCssValue(value, _context, resolved, error, uses)) << value << ": " << error;
      return resolved;
    }

    std::string ProblemOf(const std::string &value)
    {
      std::string resolved = "untouched";
      std::string error;
      EXPECT_FALSE(neon::ResolveCssValue(value, _context, resolved, error)) << value << " gave " << resolved;
      EXPECT_EQ(resolved, "untouched");
      return error;
    }
  };

  // what is left as it is

  TEST_F(CssExpressionTest, LeavesPixelsPercentagesNumbersAndWordsAsTheyAre)
  {
    EXPECT_EQ(Resolve("12px"), "12px");
    EXPECT_EQ(Resolve("50%"), "50%");
    EXPECT_EQ(Resolve("0.5"), "0.5");
    EXPECT_EQ(Resolve("-4px"), "-4px");
    EXPECT_EQ(Resolve("auto"), "auto");
    EXPECT_EQ(Resolve("space-between"), "space-between");
    EXPECT_EQ(Resolve("8px 16px 4px"), "8px 16px 4px");
    EXPECT_EQ(Resolve("2px solid #ffffff"), "2px solid #ffffff");
    EXPECT_EQ(Resolve("rgb(255, 128, 0)"), "rgb(255, 128, 0)");
    EXPECT_EQ(Resolve("rgba(255 128 0 / 50%)"), "rgba(255 128 0 / 50%)");
  }

  TEST_F(CssExpressionTest, TrimsTheSpacesAround)
  {
    EXPECT_EQ(Resolve("  12px \n"), "12px");
  }

  TEST_F(CssExpressionTest, LeavesAColorAsItIsWhateverItsDigitsSpell)
  {
    EXPECT_EQ(Resolve("#1e2em0"), "#1e2em0");
    EXPECT_EQ(Resolve("#12rem"), "#12rem");
    EXPECT_EQ(Resolve("#2vh"), "#2vh");
  }

  TEST_F(CssExpressionTest, LeavesWhatIsInQuotesAndInUrlAsItIs)
  {
    EXPECT_EQ(Resolve("\"2em var(--x) calc(1 + 1)\""), "\"2em var(--x) calc(1 + 1)\"");
    EXPECT_EQ(Resolve("'2em'"), "'2em'");
    EXPECT_EQ(Resolve("url(assets://ui/2em.png)"), "url(assets://ui/2em.png)");
    EXPECT_EQ(Resolve("url(\"a 2em b.png\")"), "url(\"a 2em b.png\")");
  }

  TEST_F(CssExpressionTest, LeavesTheUnitsOfTimeAndOfAnglesAsTheyAre)
  {
    EXPECT_EQ(Resolve("0.2s"), "0.2s");
    EXPECT_EQ(Resolve("150ms"), "150ms");
    EXPECT_EQ(Resolve("opacity 0.2s ease-in 100ms"), "opacity 0.2s ease-in 100ms");
    EXPECT_EQ(Resolve("45deg"), "45deg");
  }

  TEST_F(CssExpressionTest, LeavesAWordWithDigitsAndAUnitInItAsItIs)
  {
    EXPECT_EQ(Resolve("font2em"), "font2em");
    EXPECT_EQ(Resolve("-webkit-2em"), "-webkit-2em");
    EXPECT_EQ(Resolve("cubic-bezier(0.1, 0.7, 1, 0.1)"), "cubic-bezier(0.1, 0.7, 1, 0.1)");
    EXPECT_EQ(Resolve("steps(4, jump-end)"), "steps(4, jump-end)");
  }

  // units

  TEST_F(CssExpressionTest, TakesAnEmForTheSizeOfTheFont)
  {
    EXPECT_EQ(Resolve("1em"), "20px");
    EXPECT_EQ(Resolve("2em"), "40px");
    EXPECT_EQ(Resolve("0.5em"), "10px");
    EXPECT_EQ(Resolve(".5em"), "10px");
    EXPECT_EQ(Resolve("-1.5em"), "-30px");
    EXPECT_EQ(Resolve("+1em"), "20px");
    EXPECT_EQ(Resolve("1EM"), "20px");
  }

  TEST_F(CssExpressionTest, TakesARemForTheSizeOfTheFontOfTheRoot)
  {
    EXPECT_EQ(Resolve("1rem"), "10px");
    EXPECT_EQ(Resolve("2.4rem"), "24px");
  }

  TEST_F(CssExpressionTest, TakesTheUnitsOfTheViewportForAHundredthOfIt)
  {
    EXPECT_EQ(Resolve("1vw"), "10px");
    EXPECT_EQ(Resolve("50vw"), "500px");
    EXPECT_EQ(Resolve("1vh"), "5px");
    EXPECT_EQ(Resolve("100vh"), "500px");

    // the smaller and the larger of the two sides
    EXPECT_EQ(Resolve("10vmin"), "50px");
    EXPECT_EQ(Resolve("10vmax"), "100px");
  }

  TEST_F(CssExpressionTest, TiesTheLengthsOfPaperToThePixel)
  {
    EXPECT_EQ(Resolve("1in"), "96px");
    EXPECT_EQ(Resolve("72pt"), "96px");
    EXPECT_EQ(Resolve("12pt"), "16px");
    EXPECT_EQ(Resolve("6pc"), "96px");
    EXPECT_EQ(Resolve("2.54cm"), "96px");
    EXPECT_EQ(Resolve("25.4mm"), "96px");
  }

  TEST_F(CssExpressionTest, WorksOutEveryValueOfSeveral)
  {
    EXPECT_EQ(Resolve("1em 2rem 10vw 50%"), "20px 20px 100px 50%");
    EXPECT_EQ(Resolve("0.1em solid white"), "2px solid white");
  }

  TEST_F(CssExpressionTest, SaysWhatAValueRefersTo)
  {
    CssValueUses nothing;
    Resolve("12px 50%", &nothing);
    EXPECT_FALSE(nothing.font_size);
    EXPECT_FALSE(nothing.root_font_size);
    EXPECT_FALSE(nothing.viewport);
    EXPECT_FALSE(nothing.variables);

    CssValueUses font;
    Resolve("2em", &font);
    EXPECT_TRUE(font.font_size);
    EXPECT_FALSE(font.root_font_size);

    CssValueUses root;
    Resolve("calc(1rem + 2px)", &root);
    EXPECT_TRUE(root.root_font_size);
    EXPECT_FALSE(root.font_size);

    CssValueUses viewport;
    Resolve("10vmin", &viewport);
    EXPECT_TRUE(viewport.viewport);

    _variables["--gap"] = "1em";
    CssValueUses variable;
    Resolve("var(--gap)", &variable);
    EXPECT_TRUE(variable.variables);
    EXPECT_TRUE(variable.font_size);
  }

  // calc()

  TEST_F(CssExpressionTest, AddsAndSubtractsLengths)
  {
    EXPECT_EQ(Resolve("calc(10px + 5px)"), "15px");
    EXPECT_EQ(Resolve("calc(10px - 25px)"), "-15px");
    EXPECT_EQ(Resolve("calc(1px + 2px + 3px - 4px)"), "2px");
    EXPECT_EQ(Resolve("calc(50% + 25%)"), "75%");
    EXPECT_EQ(Resolve("calc(10px)"), "10px");
  }

  TEST_F(CssExpressionTest, MultipliesAndDividesByNumbers)
  {
    EXPECT_EQ(Resolve("calc(10px * 3)"), "30px");
    EXPECT_EQ(Resolve("calc(3 * 10px)"), "30px");
    EXPECT_EQ(Resolve("calc(10px / 4)"), "2.5px");
    EXPECT_EQ(Resolve("calc(100% / 3)"), "33.3333%");
    EXPECT_EQ(Resolve("calc(2 * 3)"), "6");
    EXPECT_EQ(Resolve("calc(1 / 4)"), "0.25");
    EXPECT_EQ(Resolve("calc(10px*3)"), "30px");
  }

  TEST_F(CssExpressionTest, MultipliesBeforeItAdds)
  {
    EXPECT_EQ(Resolve("calc(10px + 2 * 5px)"), "20px");
    EXPECT_EQ(Resolve("calc(2 * 5px + 10px)"), "20px");
    EXPECT_EQ(Resolve("calc(100px - 10px * 2 - 30px / 3)"), "70px");
  }

  TEST_F(CssExpressionTest, WorksOutBracketsFirst)
  {
    EXPECT_EQ(Resolve("calc((10px + 2px) * 5)"), "60px");
    EXPECT_EQ(Resolve("calc(2 * (3 + 4) * 1px)"), "14px");
    EXPECT_EQ(Resolve("calc(100px - (20px - (5px + 5px)))"), "90px");
    EXPECT_EQ(Resolve("calc(calc(10px + 2px) * 5)"), "60px");
  }

  TEST_F(CssExpressionTest, KeepsAPercentageAndPixelsApartInASum)
  {
    EXPECT_EQ(Resolve("calc(100% - 20px)"), "calc(100% + -20px)");
    EXPECT_EQ(Resolve("calc(50% + 8px)"), "calc(50% + 8px)");
    EXPECT_EQ(Resolve("calc((100% - 20px) / 2)"), "calc(50% + -10px)");
    EXPECT_EQ(Resolve("calc(100% - 20px + 20px)"), "100%");
    EXPECT_EQ(Resolve("calc(10px + 0%)"), "10px");
  }

  TEST_F(CssExpressionTest, WorksOutTheUnitsInsideCalc)
  {
    EXPECT_EQ(Resolve("calc(1em + 2px)"), "22px");
    EXPECT_EQ(Resolve("calc(2rem * 2)"), "40px");
    EXPECT_EQ(Resolve("calc(100vw - 100px)"), "900px");
    EXPECT_EQ(Resolve("calc(100% - 2em)"), "calc(100% + -40px)");
    EXPECT_EQ(Resolve("calc(50vh + 1in)"), "346px");
  }

  TEST_F(CssExpressionTest, WorksOutCalcNextToOtherValues)
  {
    EXPECT_EQ(Resolve("calc(1px + 1px) calc(2px * 2)"), "2px 4px");
    EXPECT_EQ(Resolve("calc(4px / 2) solid red"), "2px solid red");
    EXPECT_EQ(Resolve("CALC(1px + 1px)"), "2px");
  }

  TEST_F(CssExpressionTest, TakesASignInFrontOfANumberForPartOfTheNumber)
  {
    EXPECT_EQ(Resolve("calc(10px + -5px)"), "5px");
    EXPECT_EQ(Resolve("calc(10px - -5px)"), "15px");
    EXPECT_EQ(Resolve("calc(-1 * 10px)"), "-10px");
  }

  TEST_F(CssExpressionTest, SaysWhatIsWrongWithCalc)
  {
    EXPECT_EQ(ProblemOf("calc()"), "calc() holds nothing");
    EXPECT_EQ(ProblemOf("calc(10px"), "calc( is not closed");
    EXPECT_EQ(ProblemOf("calc((10px + 2px)"), "calc( is not closed");
    EXPECT_EQ(ProblemOf("calc(10px +)"), "calc() has '+' without a space on both sides, which CSS asks for");
    EXPECT_EQ(ProblemOf("calc(10px + )"), "calc() ends where a value was expected");
    EXPECT_EQ(ProblemOf("calc(10px+5px)"), "calc() has '+' without a space on both sides, which CSS asks for");
    EXPECT_EQ(ProblemOf("calc(10px -5px)"), "calc() has '-' without a space on both sides, which CSS asks for");
    EXPECT_EQ(ProblemOf("calc(10px + 5)"), "calc() adds a number and a length, where two of a kind were expected");
    EXPECT_EQ(
      ProblemOf("calc(5 - 10px)"),
      "calc() subtracts a number and a length, where two of a kind were expected");
    EXPECT_EQ(
      ProblemOf("calc(10px * 5px)"),
      "calc() multiplies two lengths, where one of the two has to be a number");
    EXPECT_EQ(ProblemOf("calc(10 / 5px)"), "calc() divides by a length, where a number was expected");
    EXPECT_EQ(ProblemOf("calc(10px / 0)"), "calc() divides by 0");
    EXPECT_EQ(
      ProblemOf("calc(10px + wide)"),
      "calc() holds 'wide', where a number, a length, or a percentage was expected");
    EXPECT_EQ(
      ProblemOf("calc(10px + 2s)"),
      "calc() holds the unit 's', where px, %, em, rem, vw, vh, vmin, or vmax was expected");
    EXPECT_EQ(ProblemOf("calc(10px 5px)"), "calc() holds '5px', where +, -, *, / or its end was expected");
  }

  // custom properties

  TEST_F(CssExpressionTest, PutsWhatACustomPropertyHoldsInItsPlace)
  {
    _variables["--accent"] = "#ff8000";
    _variables["--gap"] = "8px";

    EXPECT_EQ(Resolve("var(--accent)"), "#ff8000");
    EXPECT_EQ(Resolve("var( --accent )"), "#ff8000");
    EXPECT_EQ(Resolve("2px solid var(--accent)"), "2px solid #ff8000");
    EXPECT_EQ(Resolve("var(--gap) var(--gap)"), "8px 8px");
    EXPECT_EQ(Resolve("VAR(--gap)"), "8px");
  }

  TEST_F(CssExpressionTest, TellsTheNamesOfCustomPropertiesApartByCase)
  {
    _variables["--Gap"] = "8px";

    EXPECT_EQ(Resolve("var(--Gap)"), "8px");
    EXPECT_EQ(Resolve("var(--gap, 1px)"), "1px");
  }

  TEST_F(CssExpressionTest, TakesWhatIsBehindTheCommaWhenTheCustomPropertyIsNotSet)
  {
    EXPECT_EQ(Resolve("var(--missing, 12px)"), "12px");
    EXPECT_EQ(Resolve("var(--missing, rgb(1, 2, 3))"), "rgb(1, 2, 3)");
    EXPECT_EQ(Resolve("var(--missing,8px 16px)"), "8px 16px");

    // as the specification says, everything behind the first comma
    EXPECT_EQ(Resolve("var(--missing, a, b)"), "a, b");
  }

  TEST_F(CssExpressionTest, LeavesOutWhatIsBehindTheCommaWhenTheCustomPropertyIsSet)
  {
    _variables["--gap"] = "8px";
    EXPECT_EQ(Resolve("var(--gap, 12px)"), "8px");
  }

  TEST_F(CssExpressionTest, FollowsACustomPropertyThatRefersToAnother)
  {
    _variables["--base"] = "4px";
    _variables["--gap"] = "calc(var(--base) * 2)";
    _variables["--double"] = "var(--gap) var(--gap)";

    EXPECT_EQ(Resolve("var(--gap)"), "8px");
    EXPECT_EQ(Resolve("var(--double)"), "8px 8px");
    EXPECT_EQ(Resolve("var(--missing, var(--base))"), "4px");
    EXPECT_EQ(Resolve("var(--missing, var(--gone, 3px))"), "3px");
  }

  TEST_F(CssExpressionTest, WorksOutACustomPropertyInsideCalc)
  {
    _variables["--gap"] = "8px";
    _variables["--columns"] = "3";

    EXPECT_EQ(Resolve("calc(var(--gap) * 2)"), "16px");
    EXPECT_EQ(Resolve("calc(100% - var(--gap) * var(--columns))"), "calc(100% + -24px)");
    EXPECT_EQ(Resolve("calc(var(--missing, 2em) + 1px)"), "41px");
  }

  TEST_F(CssExpressionTest, WorksOutTheUnitsOfWhatACustomPropertyHolds)
  {
    _variables["--size"] = "1.5em";
    EXPECT_EQ(Resolve("var(--size)"), "30px");
  }

  TEST_F(CssExpressionTest, TakesAnEmptyCustomPropertyForNothing)
  {
    _variables["--empty"] = "";
    EXPECT_EQ(Resolve("8px var(--empty) 4px"), "8px  4px");
  }

  TEST_F(CssExpressionTest, KnowsNoCustomPropertyWithoutSomethingToLookThemUpWith)
  {
    _context.variable = nullptr;

    EXPECT_EQ(Resolve("var(--gap, 2px)"), "2px");
    EXPECT_EQ(
      ProblemOf("var(--gap)"),
      "the custom property '--gap' is not set, and var() names nothing in its place");
  }

  TEST_F(CssExpressionTest, SaysWhatIsWrongWithVar)
  {
    EXPECT_EQ(
      ProblemOf("var(--missing)"),
      "the custom property '--missing' is not set, and var() names nothing in its place");
    EXPECT_EQ(ProblemOf("var(--gap"), "var( is not closed");
    EXPECT_EQ(
      ProblemOf("var(gap)"),
      "var() holds 'gap', where the name of a custom property was expected, such as --accent");
    EXPECT_EQ(
      ProblemOf("var()"),
      "var() holds '', where the name of a custom property was expected, such as --accent");
  }

  TEST_F(CssExpressionTest, RefusesCustomPropertiesThatReferToEachOtherInACircle)
  {
    _variables["--a"] = "var(--b)";
    _variables["--b"] = "var(--a)";
    _variables["--self"] = "calc(var(--self) + 1px)";

    EXPECT_EQ(ProblemOf("var(--a)"), "custom properties refer to each other in a circle");
    EXPECT_EQ(ProblemOf("var(--self)"), "custom properties refer to each other in a circle");
  }

  TEST(CssVariablesTest, FindsVarInAValue)
  {
    EXPECT_TRUE(neon::HasCssVariables("var(--a)"));
    EXPECT_TRUE(neon::HasCssVariables("2px solid var(--a, red)"));
    EXPECT_TRUE(neon::HasCssVariables("calc(var(--a) * 2)"));
    EXPECT_TRUE(neon::HasCssVariables("VAR(--a)"));

    EXPECT_FALSE(neon::HasCssVariables("12px"));
    EXPECT_FALSE(neon::HasCssVariables("variable"));
    EXPECT_FALSE(neon::HasCssVariables("invar(--a)"));
    EXPECT_FALSE(neon::HasCssVariables("var"));
    EXPECT_FALSE(neon::HasCssVariables(""));
  }

  // numbers

  TEST(CssNumberTest, WritesANumberWithoutDigitsThatSayNothing)
  {
    EXPECT_EQ(neon::FormatCssNumber(12.0f), "12");
    EXPECT_EQ(neon::FormatCssNumber(12.5f), "12.5");
    EXPECT_EQ(neon::FormatCssNumber(-0.25f), "-0.25");
    EXPECT_EQ(neon::FormatCssNumber(0.0f), "0");
    EXPECT_EQ(neon::FormatCssNumber(-0.0f), "0");
    EXPECT_EQ(neon::FormatCssNumber(0.1f + 0.2f), "0.3");
    EXPECT_EQ(neon::FormatCssNumber(100.0f / 3.0f), "33.3333");
    EXPECT_EQ(neon::FormatCssNumber(1920.0f), "1920");
  }

  // colors

  TEST(CssColorsTest, WritesTheNamesOfCssAsRgba)
  {
    EXPECT_EQ(neon::ResolveCssColors("red"), "rgba(255, 0, 0, 1)");
    EXPECT_EQ(neon::ResolveCssColors("rebeccapurple"), "rgba(102, 51, 153, 1)");
    EXPECT_EQ(neon::ResolveCssColors("cornflowerblue"), "rgba(100, 149, 237, 1)");
    EXPECT_EQ(neon::ResolveCssColors("DarkSlateGray"), "rgba(47, 79, 79, 1)");
    EXPECT_EQ(neon::ResolveCssColors("grey"), "rgba(128, 128, 128, 1)");
    EXPECT_EQ(neon::ResolveCssColors("gray"), "rgba(128, 128, 128, 1)");
    EXPECT_EQ(neon::ResolveCssColors("lime"), "rgba(0, 255, 0, 1)");
    EXPECT_EQ(neon::ResolveCssColors("green"), "rgba(0, 128, 0, 1)");
  }

  TEST(CssColorsTest, WritesANameNextToOtherValues)
  {
    EXPECT_EQ(neon::ResolveCssColors("2px solid red"), "2px solid rgba(255, 0, 0, 1)");
    EXPECT_EQ(neon::ResolveCssColors("gold navy"), "rgba(255, 215, 0, 1) rgba(0, 0, 128, 1)");
  }

  TEST(CssColorsTest, LeavesWhatIsNoNameOfAColorAsItIs)
  {
    EXPECT_EQ(neon::ResolveCssColors("#ff8000"), "#ff8000");
    EXPECT_EQ(neon::ResolveCssColors("#bed"), "#bed");
    EXPECT_EQ(neon::ResolveCssColors("#red"), "#red");
    EXPECT_EQ(neon::ResolveCssColors("rgb(255, 128, 0)"), "rgb(255, 128, 0)");
    EXPECT_EQ(neon::ResolveCssColors("transparent"), "transparent");
    EXPECT_EQ(neon::ResolveCssColors("solid"), "solid");
    EXPECT_EQ(neon::ResolveCssColors("reddish"), "reddish");
    EXPECT_EQ(neon::ResolveCssColors("dark-red"), "dark-red");
    EXPECT_EQ(neon::ResolveCssColors("\"red\""), "\"red\"");
    EXPECT_EQ(neon::ResolveCssColors("url(red.png)"), "url(red.png)");
    EXPECT_EQ(neon::ResolveCssColors("12px"), "12px");
    EXPECT_EQ(neon::ResolveCssColors(""), "");
  }

  TEST(CssColorsTest, WorksOutHsl)
  {
    // the examples of the specification
    EXPECT_EQ(neon::ResolveCssColors("hsl(0, 100%, 50%)"), "rgba(255, 0, 0, 1)");
    EXPECT_EQ(neon::ResolveCssColors("hsl(120, 100%, 50%)"), "rgba(0, 255, 0, 1)");
    EXPECT_EQ(neon::ResolveCssColors("hsl(240, 100%, 50%)"), "rgba(0, 0, 255, 1)");
    EXPECT_EQ(neon::ResolveCssColors("hsl(120, 100%, 25%)"), "rgba(0, 128, 0, 1)");
    EXPECT_EQ(neon::ResolveCssColors("hsl(120deg 75% 75%)"), "rgba(143, 239, 143, 1)");
    EXPECT_EQ(neon::ResolveCssColors("hsl(0, 0%, 100%)"), "rgba(255, 255, 255, 1)");
    EXPECT_EQ(neon::ResolveCssColors("hsl(0, 0%, 0%)"), "rgba(0, 0, 0, 1)");
    EXPECT_EQ(neon::ResolveCssColors("hsl(210, 50%, 40%)"), "rgba(51, 102, 153, 1)");
  }

  TEST(CssColorsTest, WorksOutHslWithAlphaAndWithOtherUnits)
  {
    EXPECT_EQ(neon::ResolveCssColors("hsla(0, 100%, 50%, 0.5)"), "rgba(255, 0, 0, 0.5)");
    EXPECT_EQ(neon::ResolveCssColors("hsl(0 100% 50% / 25%)"), "rgba(255, 0, 0, 0.25)");
    EXPECT_EQ(neon::ResolveCssColors("hsl(0.5turn 100% 50%)"), "rgba(0, 255, 255, 1)");
    EXPECT_EQ(neon::ResolveCssColors("hsl(480, 100%, 50%)"), "rgba(0, 255, 0, 1)");
    EXPECT_EQ(neon::ResolveCssColors("hsl(-120, 100%, 50%)"), "rgba(0, 0, 255, 1)");
  }

  TEST(CssColorsTest, LeavesHslThatCannotBeReadAsItIs)
  {
    EXPECT_EQ(neon::ResolveCssColors("hsl(red, 1, 2)"), "hsl(red, 1, 2)");
    EXPECT_EQ(neon::ResolveCssColors("hsl(1, 2)"), "hsl(1, 2)");
  }
} // namespace

namespace
{
  TEST_F(CssExpressionTest, PutsCustomPropertiesIntoATextAndLeavesTheRestOfItAlone)
  {
    _variables["--theme"] = "dark";

    std::string resolved;
    std::string error;

    ASSERT_TRUE(neon::SubstituteCssVariables("assets://ui/var(--theme)/2em.png", _context, resolved, error));
    EXPECT_EQ(resolved, "assets://ui/dark/2em.png");

    ASSERT_TRUE(neon::SubstituteCssVariables(" calc(1px + 1px) 2em ", _context, resolved, error));
    EXPECT_EQ(resolved, "calc(1px + 1px) 2em");

    EXPECT_FALSE(neon::SubstituteCssVariables("var(--missing)", _context, resolved, error));
    EXPECT_EQ(error, "the custom property '--missing' is not set, and var() names nothing in its place");
  }
} // namespace
