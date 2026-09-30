#include "css-style-sheet.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

// What is expected here is what https://www.w3.org/TR/css-syntax-3/ and the
// specifications of the at-rules say.

namespace
{
  using neon::CssDeclaration;
  using neon::CssEnvironment;
  using neon::CssStyleSheet;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  class CssStyleSheetTest : public ::testing::Test
  {
  protected:
    CssStyleSheet _sheet;
    std::vector<std::string> _problems;

    void Read(const std::string &text)
    {
      _problems.clear();
      neon::ParseCssStyleSheet(text, "theme.css", _sheet, _problems);
    }

    /// Reads a sheet that has nothing wrong with it.
    void ReadWell(const std::string &text)
    {
      Read(text);
      EXPECT_THAT(_problems, IsEmpty()) << text;
    }

    /// `name: value` of every declaration of a rule, with `!` behind what
    /// is important.
    [[nodiscard]] std::vector<std::string> DeclarationsOf(const std::size_t rule) const
    {
      std::vector<std::string> written;
      if (rule >= _sheet.rules.size()) { return written; }

      for (const auto &declaration : _sheet.rules[rule].declarations) { written.push_back(Written(declaration)); }
      return written;
    }

    [[nodiscard]] static std::string Written(const CssDeclaration &declaration)
    {
      return declaration.name + ": " + declaration.value + (declaration.is_important ? " !" : "");
    }
  };

  // rules and declarations

  TEST_F(CssStyleSheetTest, ReadsNothingFromAnEmptySheet)
  {
    ReadWell("");
    EXPECT_TRUE(_sheet.rules.empty());
    EXPECT_EQ(_sheet.path, "theme.css");

    ReadWell("  \n\t\n");
    EXPECT_TRUE(_sheet.rules.empty());
  }

  TEST_F(CssStyleSheetTest, ReadsARule)
  {
    ReadWell("button { color: red; background-color: #334455; }");

    ASSERT_EQ(_sheet.rules.size(), 1u);
    EXPECT_EQ(_sheet.rules[0].selector_text, "button");
    ASSERT_EQ(_sheet.rules[0].selectors.size(), 1u);
    EXPECT_EQ(_sheet.rules[0].line, 1u);
    EXPECT_THAT(DeclarationsOf(0), ElementsAre("color: red", "background-color: #334455"));
  }

  TEST_F(CssStyleSheetTest, ReadsRulesInTheOrderTheyAreWrittenIn)
  {
    ReadWell(
      "panel { padding: 8px }\n"
      "\n"
      ".row,\n"
      ".row:hover { gap: 4px }\n"
      "#close { opacity: 0.5 }\n");

    ASSERT_EQ(_sheet.rules.size(), 3u);
    EXPECT_EQ(_sheet.rules[0].selector_text, "panel");
    EXPECT_EQ(_sheet.rules[0].line, 1u);
    EXPECT_EQ(_sheet.rules[1].selector_text, ".row, .row:hover");
    EXPECT_EQ(_sheet.rules[1].selectors.size(), 2u);
    EXPECT_EQ(_sheet.rules[1].line, 3u);
    EXPECT_EQ(_sheet.rules[2].selector_text, "#close");
    EXPECT_EQ(_sheet.rules[2].line, 5u);
  }

  TEST_F(CssStyleSheetTest, NeedsNoSemicolonBehindTheLastDeclaration)
  {
    ReadWell("a { color: red; width: 10px }");
    EXPECT_THAT(DeclarationsOf(0), ElementsAre("color: red", "width: 10px"));
  }

  TEST_F(CssStyleSheetTest, SkipsSemicolonsThatStandAlone)
  {
    ReadWell("a { ; color: red;; width: 10px;;; }");
    EXPECT_THAT(DeclarationsOf(0), ElementsAre("color: red", "width: 10px"));
  }

  TEST_F(CssStyleSheetTest, ReadsARuleWithoutDeclarations)
  {
    ReadWell("a { }");
    ASSERT_EQ(_sheet.rules.size(), 1u);
    EXPECT_TRUE(_sheet.rules[0].declarations.empty());
  }

  TEST_F(CssStyleSheetTest, KeepsTheLineOfEveryDeclaration)
  {
    ReadWell(
      "a {\n"
      "  color: red;\n"
      "\n"
      "  width:\n"
      "    10px;\n"
      "  height: 20px; gap: 1px;\n"
      "}\n");

    const auto &declarations = _sheet.rules[0].declarations;
    ASSERT_EQ(declarations.size(), 4u);
    EXPECT_EQ(declarations[0].line, 2u);
    EXPECT_EQ(declarations[1].line, 4u);
    EXPECT_EQ(declarations[1].value, "10px");
    EXPECT_EQ(declarations[2].line, 6u);
    EXPECT_EQ(declarations[3].line, 6u);
  }

  TEST_F(CssStyleSheetTest, WritesNamesInSmallLettersAndKeepsValues)
  {
    ReadWell("a { COLOR: Red; Font-Family: \"Inter Display\"; }");
    EXPECT_THAT(DeclarationsOf(0), ElementsAre("color: Red", "font-family: \"Inter Display\""));
  }

  TEST_F(CssStyleSheetTest, KeepsAValueWithSeveralPartsAsItIs)
  {
    ReadWell(
      "a {\n"
      "  margin: 8px   16px 4px;\n"
      "  border: 2px solid rgb(255, 128, 0);\n"
      "  transition: opacity 0.2s ease-in, color 1s cubic-bezier(0.1, 0.7, 1, 0.1);\n"
      "  width: calc(100% - (2 * 16px));\n"
      "}\n");

    EXPECT_THAT(
      DeclarationsOf(0),
      ElementsAre(
        "margin: 8px   16px 4px",
        "border: 2px solid rgb(255, 128, 0)",
        "transition: opacity 0.2s ease-in, color 1s cubic-bezier(0.1, 0.7, 1, 0.1)",
        "width: calc(100% - (2 * 16px))"));
  }

  TEST_F(CssStyleSheetTest, KeepsWhatIsInQuotesAsItIs)
  {
    ReadWell("a { font-family: \"a; b } c: d /* e */\"; background-image: url('assets://ui/a;b.png'); }");

    EXPECT_THAT(
      DeclarationsOf(0),
      ElementsAre("font-family: \"a; b } c: d /* e */\"", "background-image: url('assets://ui/a;b.png')"));
  }

  TEST_F(CssStyleSheetTest, ReadsWhatIsImportant)
  {
    ReadWell("a { color: red !important; width: 1px!important; height: 2px ! IMPORTANT ; gap: 3px }");

    EXPECT_THAT(
      DeclarationsOf(0),
      ElementsAre("color: red !", "width: 1px !", "height: 2px !", "gap: 3px"));
  }

  // comments

  TEST_F(CssStyleSheetTest, LeavesCommentsOut)
  {
    ReadWell(
      "/* the theme */\n"
      "a /* every link */ { /* first */ color: /* the colour */ red; /* last */ }\n"
      "/* b { color: blue; } */\n"
      "c { width: 1px }\n");

    ASSERT_EQ(_sheet.rules.size(), 2u);
    EXPECT_EQ(_sheet.rules[0].selector_text, "a");
    EXPECT_THAT(DeclarationsOf(0), ElementsAre("color: red"));
    EXPECT_EQ(_sheet.rules[1].selector_text, "c");
  }

  TEST_F(CssStyleSheetTest, CountsTheLinesOfAComment)
  {
    ReadWell(
      "/* one\n"
      "   two\n"
      "   three */\n"
      "a { color: red }\n");

    EXPECT_EQ(_sheet.rules[0].line, 4u);
  }

  TEST_F(CssStyleSheetTest, TakesACommentForWhatSetsTwoWordsApart)
  {
    ReadWell("a { margin: 1px/**/2px }");
    EXPECT_THAT(DeclarationsOf(0), ElementsAre("margin: 1px 2px"));
  }

  TEST_F(CssStyleSheetTest, LeavesOutTheMarkOfTheOrderOfBytes)
  {
    ReadWell("\xEF\xBB\xBF" "a { color: red }");
    ASSERT_EQ(_sheet.rules.size(), 1u);
    EXPECT_EQ(_sheet.rules[0].selector_text, "a");
  }

  // custom properties

  TEST_F(CssStyleSheetTest, ReadsCustomPropertiesAndKeepsTheirLetters)
  {
    ReadWell(":root { --Accent: #ff8000; --gap-Large: 24px; --empty:; --list: 1px 2px , 3px }");

    EXPECT_THAT(
      DeclarationsOf(0),
      ElementsAre("--Accent: #ff8000", "--gap-Large: 24px", "--empty: ", "--list: 1px 2px , 3px"));
    EXPECT_TRUE(_sheet.rules[0].declarations[0].IsCustomProperty());
  }

  TEST_F(CssStyleSheetTest, KeepsVarAsItIsWritten)
  {
    ReadWell("a { color: var(--accent, #fff); padding: var( --gap ) calc(var(--gap) * 2) }");

    EXPECT_THAT(
      DeclarationsOf(0),
      ElementsAre("color: var(--accent, #fff)", "padding: var( --gap ) calc(var(--gap) * 2)"));
    EXPECT_FALSE(_sheet.rules[0].declarations[0].IsCustomProperty());
  }

  // @import

  TEST_F(CssStyleSheetTest, ReadsImportsInEveryForm)
  {
    ReadWell(
      "@import \"base.css\";\n"
      "@import 'assets://ui/fonts.css';\n"
      "@import url(\"colours.css\");\n"
      "@import url(plain.css);\n"
      "@IMPORT url( 'spaced.css' ) ;\n"
      "a { color: red }\n");

    ASSERT_EQ(_sheet.imports.size(), 5u);
    EXPECT_EQ(_sheet.imports[0].path, "base.css");
    EXPECT_EQ(_sheet.imports[0].line, 1u);
    EXPECT_EQ(_sheet.imports[0].media, -1);
    EXPECT_EQ(_sheet.imports[1].path, "assets://ui/fonts.css");
    EXPECT_EQ(_sheet.imports[2].path, "colours.css");
    EXPECT_EQ(_sheet.imports[3].path, "plain.css");
    EXPECT_EQ(_sheet.imports[4].path, "spaced.css");
    EXPECT_EQ(_sheet.imports[4].line, 5u);
    EXPECT_EQ(_sheet.rules.size(), 1u);
  }

  TEST_F(CssStyleSheetTest, ReadsTheConditionOfAnImport)
  {
    ReadWell("@import \"small.css\" screen and (max-width: 800px);");

    ASSERT_EQ(_sheet.imports.size(), 1u);
    ASSERT_EQ(_sheet.imports[0].media, 0);
    ASSERT_EQ(_sheet.media.size(), 1u);
    EXPECT_TRUE(_sheet.media[0].Matches(CssEnvironment{640.0f, 480.0f}));
    EXPECT_FALSE(_sheet.media[0].Matches(CssEnvironment{1280.0f, 720.0f}));
  }

  TEST_F(CssStyleSheetTest, LeavesOutAnImportBehindARule)
  {
    Read(
      "@import \"first.css\";\n"
      "a { color: red }\n"
      "@import \"late.css\";\n");

    ASSERT_EQ(_sheet.imports.size(), 1u);
    EXPECT_EQ(_sheet.imports[0].path, "first.css");
    EXPECT_THAT(
      _problems,
      ElementsAre(
        "theme.css:3: @import of 'late.css' comes behind a rule and is left out. It has to come first, as "
        "CSS says"));
  }

  TEST_F(CssStyleSheetTest, TakesAnImportBehindTheCharset)
  {
    ReadWell("@charset \"utf-8\";\n@import \"base.css\";");
    ASSERT_EQ(_sheet.imports.size(), 1u);
  }

  TEST_F(CssStyleSheetTest, SaysWhatIsWrongWithAnImport)
  {
    Read(
      "@import;\n"
      "@import base.css;\n"
      "@import \"a.css\" (min-colour: 1);\n"
      "@import \"b.css\" { }\n");

    EXPECT_TRUE(_sheet.imports.empty());
    EXPECT_THAT(
      _problems,
      ElementsAre(
        "theme.css:1: @import is followed by '', where the path of a style sheet was expected, such as "
        "\"theme.css\" or url(\"theme.css\")",
        "theme.css:2: @import is followed by 'base.css', where the path of a style sheet was expected, "
        "such as \"theme.css\" or url(\"theme.css\")",
        "theme.css:3: the condition '(min-colour: 1)' of @import cannot be read: 'colour' is not a feature "
        "that is known. Known are: width, height, aspect-ratio, orientation, resolution, "
        "prefers-reduced-motion. The sheet is left out",
        "theme.css:4: @import is followed by a block, where a path and a semicolon were expected"));
  }

  // @font-face

  TEST_F(CssStyleSheetTest, ReadsAFont)
  {
    ReadWell(
      "@font-face {\n"
      "  font-family: \"Title\";\n"
      "  src: url(\"assets://fonts/title.ttf\") format(\"truetype\");\n"
      "  font-weight: 700;\n"
      "  font-style: normal;\n"
      "  font-display: swap;\n"
      "}\n"
      "@font-face { font-family: Body; src: url(fonts/body.ttf) }\n"
      "@font-face { font-family: Body; src: url(fonts/body-bold.ttf), url(other.ttf); font-weight: bold }\n");

    ASSERT_EQ(_sheet.fonts.size(), 3u);
    EXPECT_EQ(_sheet.fonts[0].family, "Title");
    EXPECT_EQ(_sheet.fonts[0].source, "assets://fonts/title.ttf");
    EXPECT_EQ(_sheet.fonts[0].weight, 700);
    EXPECT_EQ(_sheet.fonts[0].line, 1u);

    EXPECT_EQ(_sheet.fonts[1].family, "Body");
    EXPECT_EQ(_sheet.fonts[1].source, "fonts/body.ttf");
    EXPECT_EQ(_sheet.fonts[1].weight, 400);
    EXPECT_EQ(_sheet.fonts[1].line, 8u);

    EXPECT_EQ(_sheet.fonts[2].source, "fonts/body-bold.ttf");
    EXPECT_EQ(_sheet.fonts[2].weight, 700);
  }

  TEST_F(CssStyleSheetTest, SaysWhatIsWrongWithAFont)
  {
    Read(
      "@font-face { src: url(a.ttf) }\n"
      "@font-face { font-family: A }\n"
      "@font-face {\n"
      "  font-family: B;\n"
      "  src: a.ttf;\n"
      "}\n"
      "@font-face {\n"
      "  font-family: C;\n"
      "  src: url(c.ttf);\n"
      "  font-weight: heavy;\n"
      "  font-colour: red;\n"
      "}\n");

    EXPECT_TRUE(_sheet.fonts.empty());
    EXPECT_THAT(
      _problems,
      ElementsAre(
        "theme.css:1: @font-face has no 'font-family', where the name it is asked for by was expected",
        "theme.css:2: @font-face has no 'src', where url() with the path of a font was expected",
        "theme.css:5: 'src' of @font-face is 'a.ttf', where url() with the path of a font was expected, such "
        "as url(\"fonts/title.ttf\")",
        "theme.css:10: 'font-weight' of @font-face is 'heavy', where a number from 1 to 1000, normal, or "
        "bold was expected",
        "theme.css:11: 'font-colour' is not known to @font-face. Known are: font-family, src, font-weight"));
  }

  // @keyframes

  TEST_F(CssStyleSheetTest, ReadsKeyframes)
  {
    ReadWell(
      "@keyframes fade-in {\n"
      "  from { opacity: 0; }\n"
      "  50% { opacity: 0.8; width: 10px }\n"
      "  to { opacity: 1 }\n"
      "}\n");

    ASSERT_EQ(_sheet.keyframes.size(), 1u);
    const auto &keyframes = _sheet.keyframes[0];

    EXPECT_EQ(keyframes.name, "fade-in");
    EXPECT_EQ(keyframes.line, 1u);
    ASSERT_EQ(keyframes.frames.size(), 3u);

    EXPECT_FLOAT_EQ(keyframes.frames[0].offset, 0.0f);
    EXPECT_EQ(keyframes.frames[0].line, 2u);
    EXPECT_EQ(Written(keyframes.frames[0].declarations[0]), "opacity: 0");

    EXPECT_FLOAT_EQ(keyframes.frames[1].offset, 0.5f);
    ASSERT_EQ(keyframes.frames[1].declarations.size(), 2u);

    EXPECT_FLOAT_EQ(keyframes.frames[2].offset, 1.0f);
    EXPECT_EQ(Written(keyframes.frames[2].declarations[0]), "opacity: 1");
  }

  TEST_F(CssStyleSheetTest, SortsKeyframesByTheirPartOfTheWay)
  {
    ReadWell("@keyframes a { to { width: 3px } 25.5% { width: 2px } 0% { width: 1px } 100% { height: 4px } }");

    const auto &frames = _sheet.keyframes[0].frames;
    ASSERT_EQ(frames.size(), 4u);
    EXPECT_FLOAT_EQ(frames[0].offset, 0.0f);
    EXPECT_FLOAT_EQ(frames[1].offset, 0.255f);
    EXPECT_FLOAT_EQ(frames[2].offset, 1.0f);
    EXPECT_FLOAT_EQ(frames[3].offset, 1.0f);

    // two at the same part stay in the order they were written in
    EXPECT_EQ(frames[2].declarations[0].name, "width");
    EXPECT_EQ(frames[3].declarations[0].name, "height");
  }

  TEST_F(CssStyleSheetTest, ReadsOneBlockForSeveralPartsOfTheWay)
  {
    ReadWell("@keyframes pulse { from, 50%, to { opacity: 1 } 25%, 75% { opacity: 0 } }");

    const auto &frames = _sheet.keyframes[0].frames;
    ASSERT_EQ(frames.size(), 5u);
    EXPECT_FLOAT_EQ(frames[0].offset, 0.0f);
    EXPECT_FLOAT_EQ(frames[1].offset, 0.25f);
    EXPECT_FLOAT_EQ(frames[2].offset, 0.5f);
    EXPECT_FLOAT_EQ(frames[3].offset, 0.75f);
    EXPECT_FLOAT_EQ(frames[4].offset, 1.0f);
  }

  TEST_F(CssStyleSheetTest, ReadsTheNameOfKeyframesInQuotes)
  {
    ReadWell("@keyframes \"slide in\" { to { left: 0 } }");
    EXPECT_EQ(_sheet.keyframes[0].name, "slide in");
  }

  TEST_F(CssStyleSheetTest, ReadsATimingFunctionInsideAKeyframe)
  {
    ReadWell("@keyframes a { from { animation-timing-function: ease-in; top: 0 } to { top: 10px } }");
    EXPECT_EQ(Written(_sheet.keyframes[0].frames[0].declarations[0]), "animation-timing-function: ease-in");
  }

  TEST_F(CssStyleSheetTest, SaysWhatIsWrongWithKeyframes)
  {
    Read(
      "@keyframes { to { opacity: 1 } }\n"
      "@keyframes a {\n"
      "  start { opacity: 0 }\n"
      "  120% { opacity: 1 }\n"
      "  -5% { opacity: 1 }\n"
      "  50% { opacity: 1 !important; width: 1px }\n"
      "  to;\n"
      "}\n");

    ASSERT_EQ(_sheet.keyframes.size(), 1u);
    EXPECT_EQ(_sheet.keyframes[0].name, "a");
    ASSERT_EQ(_sheet.keyframes[0].frames.size(), 1u);
    EXPECT_THAT(_sheet.keyframes[0].frames[0].declarations.size(), 1u);

    EXPECT_THAT(
      _problems,
      ElementsAre(
        "theme.css:1: @keyframes is followed by '', where the name of the animation was expected",
        "theme.css:3: 'start' of @keyframes a is no part of the way, where from, to, or a percentage from "
        "0% to 100% was expected",
        "theme.css:4: '120%' of @keyframes a is no part of the way, where from, to, or a percentage from 0% "
        "to 100% was expected",
        "theme.css:5: '-5%' of @keyframes a is no part of the way, where from, to, or a percentage from 0% "
        "to 100% was expected",
        "theme.css:6: 'opacity' of @keyframes a is !important, which a keyframe cannot be. It is left out",
        "theme.css:7: 'to' of @keyframes a has no block, where from, to, or a percentage and a block were "
        "expected"));
  }

  // @media

  TEST_F(CssStyleSheetTest, ReadsTheRulesOfAMediaQuery)
  {
    ReadWell(
      "a { color: red }\n"
      "@media (max-width: 800px) {\n"
      "  a { color: blue }\n"
      "  b { color: green }\n"
      "}\n"
      "c { color: white }\n");

    ASSERT_EQ(_sheet.rules.size(), 4u);
    ASSERT_EQ(_sheet.media.size(), 1u);

    EXPECT_TRUE(_sheet.rules[0].media.empty());
    EXPECT_THAT(_sheet.rules[1].media, ElementsAre(0u));
    EXPECT_THAT(_sheet.rules[2].media, ElementsAre(0u));
    EXPECT_TRUE(_sheet.rules[3].media.empty());

    EXPECT_EQ(_sheet.rules[1].line, 3u);
    EXPECT_EQ(_sheet.rules[3].selector_text, "c");
    EXPECT_EQ(_sheet.media[0].text, "(max-width: 800px)");
  }

  TEST_F(CssStyleSheetTest, ReadsAMediaQueryInsideAnother)
  {
    ReadWell(
      "@media screen {\n"
      "  @media (orientation: landscape) {\n"
      "    a { color: blue }\n"
      "  }\n"
      "  b { color: red }\n"
      "}\n");

    ASSERT_EQ(_sheet.rules.size(), 2u);
    EXPECT_THAT(_sheet.rules[0].media, ElementsAre(0u, 1u));
    EXPECT_THAT(_sheet.rules[1].media, ElementsAre(0u));
  }

  TEST_F(CssStyleSheetTest, ReadsKeyframesInsideAMediaQuery)
  {
    ReadWell("@media (prefers-reduced-motion: no-preference) { @keyframes a { to { opacity: 1 } } }");

    ASSERT_EQ(_sheet.keyframes.size(), 1u);
    EXPECT_THAT(_sheet.keyframes[0].media, ElementsAre(0u));
  }

  TEST_F(CssStyleSheetTest, LeavesOutWhatAMediaQueryHoldsThatCannotBeRead)
  {
    Read(
      "@media (min-colour: 1) {\n"
      "  a { color: blue }\n"
      "}\n"
      "@media { b { color: red } }\n"
      "c { color: white }\n");

    ASSERT_EQ(_sheet.rules.size(), 1u);
    EXPECT_EQ(_sheet.rules[0].selector_text, "c");
    EXPECT_EQ(_sheet.rules[0].line, 5u);
    EXPECT_THAT(
      _problems,
      ElementsAre(
        "theme.css:1: the condition '(min-colour: 1)' of @media cannot be read: 'colour' is not a feature "
        "that is known. Known are: width, height, aspect-ratio, orientation, resolution, "
        "prefers-reduced-motion. What it holds is left out",
        "theme.css:4: @media has no condition, where one such as (min-width: 800px) was expected"));
  }

  // what is wrong

  TEST_F(CssStyleSheetTest, LeavesOutADeclarationThatCannotBeReadAndKeepsTheRest)
  {
    Read(
      "a {\n"
      "  color red;\n"
      "  width: 10px;\n"
      "  : 5px;\n"
      "  height:;\n"
      "  2col: 1;\n"
      "  gap: 4px !imporant;\n"
      "  opacity: 0.5;\n"
      "}\n");

    EXPECT_THAT(DeclarationsOf(0), ElementsAre("width: 10px", "opacity: 0.5"));
    EXPECT_THAT(
      _problems,
      ElementsAre(
        "theme.css:2: 'color red' of the rule 'a' has no colon, where a declaration such as color: red was "
        "expected",
        "theme.css:4: '' of the rule 'a' is not the name of a property, which is made of letters, digits, "
        "and hyphens",
        "theme.css:5: 'height' of the rule 'a' has no value, where one was expected behind the colon",
        "theme.css:6: '2col' of the rule 'a' is not the name of a property, which is made of letters, "
        "digits, and hyphens",
        "theme.css:7: 'gap' of the rule 'a' has '!imporant' behind its value, where !important or nothing "
        "was expected"));
  }

  TEST_F(CssStyleSheetTest, LeavesOutARuleWhoseSelectorCannotBeReadAndKeepsTheRest)
  {
    Read(
      "a { color: red }\n"
      "b:visited, c { color: blue }\n"
      "d > { color: green }\n"
      "e { color: white }\n");

    ASSERT_EQ(_sheet.rules.size(), 2u);
    EXPECT_EQ(_sheet.rules[0].selector_text, "a");
    EXPECT_EQ(_sheet.rules[1].selector_text, "e");
    EXPECT_EQ(_sheet.rules[1].line, 4u);

    ASSERT_EQ(_problems.size(), 2u);
    EXPECT_TRUE(_problems[0].starts_with(
      "theme.css:2: the selector 'b:visited, c' cannot be read: ':visited' is not a pseudo-class that is "
      "known."));
    EXPECT_TRUE(_problems[0].ends_with(". The rule is left out"));
    EXPECT_EQ(
      _problems[1],
      "theme.css:3: the selector 'd >' cannot be read: a selector ends with a combinator, where a selector "
      "was expected behind it. The rule is left out");
  }

  TEST_F(CssStyleSheetTest, SaysThatARuleInsideARuleIsNotRead)
  {
    Read(
      "a {\n"
      "  color: red;\n"
      "  b { color: blue }\n"
      "  width: 1px;\n"
      "}\n");

    EXPECT_THAT(DeclarationsOf(0), ElementsAre("color: red", "width: 1px"));
    EXPECT_THAT(
      _problems,
      ElementsAre(
        "theme.css:3: 'b' of the rule 'a' starts a block, where a declaration such as color: red was "
        "expected. Rules inside of rules are not read"));
  }

  TEST_F(CssStyleSheetTest, SaysThatAnAtRuleIsNotKnown)
  {
    Read(
      "@supports (display: grid) { a { color: red } }\n"
      "@layer base;\n"
      "b { color: blue }\n");

    ASSERT_EQ(_sheet.rules.size(), 1u);
    EXPECT_EQ(_sheet.rules[0].selector_text, "b");
    EXPECT_THAT(
      _problems,
      ElementsAre(
        "theme.css:1: @supports is not known, and what it holds is left out. Known are: @import, @media, "
        "@font-face, @keyframes",
        "theme.css:2: @layer has no block, where { was expected"));
  }

  TEST_F(CssStyleSheetTest, SaysThatSomethingIsNotClosed)
  {
    Read("a { color: red;\nb { width: 1px }\n");
    EXPECT_THAT(DeclarationsOf(0), ElementsAre("color: red"));

    // the rule that was opened last is the one that is missing its end
    ASSERT_FALSE(_problems.empty());
    EXPECT_EQ(_problems.back(), "theme.css:1: the block of the rule 'a' is not closed, where } was expected");

    Read("a { color: red }\n/* never ends\n");
    EXPECT_EQ(_sheet.rules.size(), 1u);
    EXPECT_THAT(_problems, ElementsAre("theme.css:2: a comment is not closed, where */ was expected"));

    Read("@media screen {\n  a { color: red }\n");
    EXPECT_EQ(_sheet.rules.size(), 1u);
    EXPECT_THAT(_problems, ElementsAre("theme.css:1: the block of @media is not closed, where } was expected"));

    Read("a { font-family: \"Inter;\n  color: red }\n");
    ASSERT_FALSE(_problems.empty());
    EXPECT_EQ(_problems[0], "theme.css:1: a text in quotes is not closed");
  }

  TEST_F(CssStyleSheetTest, SaysThatABracketClosesNothing)
  {
    Read("a { color: red }\n}\nb { color: blue }\n");

    EXPECT_EQ(_sheet.rules.size(), 2u);
    EXPECT_THAT(_problems, ElementsAre("theme.css:2: '}' closes a block that was not opened"));
  }

  TEST_F(CssStyleSheetTest, SaysThatASelectorHasNoBlock)
  {
    Read("a;\nb { color: blue }\nc");

    ASSERT_EQ(_sheet.rules.size(), 1u);
    EXPECT_THAT(
      _problems,
      ElementsAre(
        "theme.css:1: 'a' is followed by no block, where a rule such as button { color: red; } was expected",
        "theme.css:3: 'c' is followed by no block, where a rule such as button { color: red; } was expected"));
  }

  TEST_F(CssStyleSheetTest, FindsEveryProblemOfASheetAndNotOnlyTheFirst)
  {
    Read(
      "a { colour red }\n"
      "b:visited { color: blue }\n"
      "@media (loud) { c { color: red } }\n"
      "@keyframes k { half { opacity: 0 } }\n"
      "@font-face { font-family: F }\n"
      "d { width: 1px }\n");

    EXPECT_EQ(_problems.size(), 5u);
    for (std::size_t i = 0; i < _problems.size(); i++)
    {
      EXPECT_TRUE(_problems[i].starts_with("theme.css:" + std::to_string(i + 1) + ": ")) << _problems[i];
    }

    // and what is right is kept
    ASSERT_EQ(_sheet.rules.size(), 2u);
    EXPECT_EQ(_sheet.rules[1].selector_text, "d");
  }

  // declarations alone

  TEST(CssDeclarationsTest, ReadsDeclarationsWithoutARule)
  {
    std::vector<CssDeclaration> declarations;
    std::vector<std::string> problems;

    neon::ParseCssDeclarations("color: red; width: 10px", "inline", 7, declarations, problems);

    EXPECT_THAT(problems, IsEmpty());
    ASSERT_EQ(declarations.size(), 2u);
    EXPECT_EQ(declarations[0].name, "color");
    EXPECT_EQ(declarations[0].line, 7u);
    EXPECT_EQ(declarations[1].value, "10px");
  }

  TEST(CssDeclarationsTest, SaysWhatIsWrongWithTheLineItWasGiven)
  {
    std::vector<CssDeclaration> declarations;
    std::vector<std::string> problems;

    neon::ParseCssDeclarations("color red;\nwidth: 1px }", "inline", 3, declarations, problems);

    ASSERT_EQ(declarations.size(), 1u);
    EXPECT_THAT(
      problems,
      ElementsAre(
        "inline:3: 'color red' of the style has no colon, where a declaration such as color: red was "
        "expected",
        "inline:4: '}' closes a block that was not opened"));
  }
} // namespace
