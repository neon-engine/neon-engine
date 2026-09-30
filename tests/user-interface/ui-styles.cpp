#include "ui-fixture.hpp"

// Style sheets in files of CSS: what a file of YAML names under `styles`,
// which rule wins, what an element takes from the one above it, and what is
// said about a sheet that is wrong. What is expected is what
// https://www.w3.org/TR/css-cascade-4/ says.

namespace
{
  using neon::Color;
  using neon::LayoutLength;
  using neon::UiElement;
  using neon::UiStates;
  using neon::UiStyle;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;
  using ::testing::ElementsAre;
  using ::testing::HasSubstr;
  using ::testing::IsEmpty;

  class UiStyleSheetTest : public UiTest
  {
  protected:
    /// Shows a file of YAML that names the sheet `theme.css` next to it.
    void ShowWith(const std::string &css, const std::string &elements)
    {
      WriteAsset("ui/theme.css", css);

      const std::string yaml =
        "ui: test\n"
        "styles: [theme.css]\n"
        "root:\n"
        "  type: panel\n"
        "  name: root\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  children:\n" + Indented(elements, "    ");

      ASSERT_GE(Show(yaml), 0) << _logger->Messages(LogLevel::Error);
      Frame();
    }

    [[nodiscard]] const UiStyle &StyleOf(const std::string &name) const
    {
      return Element(name).GetStyle();
    }

    [[nodiscard]] std::vector<std::string> Warnings() const
    {
      std::vector<std::string> warnings;
      for (const auto &[level, message] : _logger->Entries())
      {
        if (level == LogLevel::Warn) { warnings.push_back(message); }
      }
      return warnings;
    }

    UiStyle StyleIn(const std::string &name, const UiStates &states)
    {
      auto &element = const_cast<UiElement &>(Element(name));
      const UiStates before = element.GetStates();

      element.SetStates(states);
      UiStyle style = element.GetStyle();
      element.SetStates(before);
      return style;
    }

    static void ExpectColor(const Color &color, const float r, const float g, const float b, const float a = 1.0f)
    {
      EXPECT_NEAR(color.r, r, 0.003f);
      EXPECT_NEAR(color.g, g, 0.003f);
      EXPECT_NEAR(color.b, b, 0.003f);
      EXPECT_NEAR(color.a, a, 0.003f);
    }
  };

  // naming sheets

  TEST_F(UiStyleSheetTest, ReadsASheetThatIsNamedNextToTheFile)
  {
    ShowWith("label { font-size: 32px; }", "- type: label\n  name: title\n  text: Hello\n");

    EXPECT_FLOAT_EQ(StyleOf("title").font_size, 32.0f);
    EXPECT_THAT(Warnings(), IsEmpty());
    EXPECT_THAT(Errors(), IsEmpty());
  }

  TEST_F(UiStyleSheetTest, ReadsASheetThatIsNamedByAVirtualPath)
  {
    WriteAsset("themes/dark.css", "label { font-size: 40px; }");

    ASSERT_GE(Show(
      "styles: [assets://themes/dark.css]\n"
      "root:\n"
      "  type: label\n"
      "  name: title\n"
      "  text: Hello\n"), 0);

    EXPECT_FLOAT_EQ(StyleOf("title").font_size, 40.0f);
  }

  TEST_F(UiStyleSheetTest, ReadsOneSheetThatIsNamedWithoutAList)
  {
    WriteAsset("ui/theme.css", "label { font-size: 20px; }");

    ASSERT_GE(Show("styles: theme.css\nroot:\n  type: label\n  name: title\n"), 0);
    EXPECT_FLOAT_EQ(StyleOf("title").font_size, 20.0f);
  }

  TEST_F(UiStyleSheetTest, ReadsSheetsInTheOrderTheyAreNamedIn)
  {
    WriteAsset("ui/first.css", "label { font-size: 20px; opacity: 0.5; }");
    WriteAsset("ui/second.css", "label { font-size: 30px; }");

    ASSERT_GE(Show("styles: [first.css, second.css]\nroot:\n  type: label\n  name: title\n"), 0);

    // of two rules that count the same, the one that is written last wins
    EXPECT_FLOAT_EQ(StyleOf("title").font_size, 30.0f);
    EXPECT_FLOAT_EQ(StyleOf("title").opacity, 0.5f);
  }

  TEST_F(UiStyleSheetTest, RefusesAFileWhoseSheetCannotBeRead)
  {
    ExpectProblems(
      "ui: test\n"
      "styles: [missing.css]\n"
      "root:\n"
      "  type: panel\n",
      {"assets://ui/test.ui.yml:2: the style sheet assets://ui/missing.css cannot be read"});
  }

  TEST_F(UiStyleSheetTest, SaysThatStylesIsNoListOfPaths)
  {
    ExpectProblems(
      "styles: 3\nroot:\n  type: panel\n",
      {
        "assets://ui/test.ui.yml:1: 'styles' of the user interface is a number, where a list of the virtual "
        "paths of style sheets was expected, such as [assets://ui/theme.css]"
      });

    ExpectProblems(
      "styles: [theme.css, 3]\nroot:\n  type: panel\n",
      {
        "assets://ui/test.ui.yml:1: 'styles' of the user interface is another list, where a list of the "
        "virtual paths of style sheets was expected, such as [assets://ui/theme.css]"
      });
  }

  TEST_F(UiStyleSheetTest, ReadsWhatASheetImportsInFrontOfItsOwnRules)
  {
    WriteAsset("ui/shared/base.css", "label { font-size: 20px; opacity: 0.5; }");
    WriteAsset("ui/theme.css", "@import \"shared/base.css\";\nlabel { font-size: 30px; }");

    ASSERT_GE(Show("styles: [theme.css]\nroot:\n  type: label\n  name: title\n"), 0);

    EXPECT_FLOAT_EQ(StyleOf("title").font_size, 30.0f);
    EXPECT_FLOAT_EQ(StyleOf("title").opacity, 0.5f);
  }

  TEST_F(UiStyleSheetTest, LooksForAnImportNextToTheSheetThatNamesIt)
  {
    WriteAsset("themes/parts/colours.css", "label { opacity: 0.25; }");
    WriteAsset("themes/dark.css", "@import url(parts/colours.css);");

    ASSERT_GE(Show("styles: [assets://themes/dark.css]\nroot:\n  type: label\n  name: title\n"), 0);
    EXPECT_FLOAT_EQ(StyleOf("title").opacity, 0.25f);
  }

  TEST_F(UiStyleSheetTest, GoesUpAFolderForAnImport)
  {
    WriteAsset("shared.css", "label { opacity: 0.75; }");
    WriteAsset("ui/theme.css", "@import \"../shared.css\";");

    ASSERT_GE(Show("styles: [theme.css]\nroot:\n  type: label\n  name: title\n"), 0);
    EXPECT_FLOAT_EQ(StyleOf("title").opacity, 0.75f);
  }

  TEST_F(UiStyleSheetTest, RefusesAFileWhoseImportCannotBeRead)
  {
    WriteAsset("ui/theme.css", "\n@import \"gone.css\";\n");

    ExpectProblems(
      "styles: [theme.css]\nroot:\n  type: panel\n",
      {"assets://ui/test.ui.yml:1: assets://ui/theme.css:2: the style sheet assets://ui/gone.css cannot be read"});
  }

  TEST_F(UiStyleSheetTest, LeavesOutSheetsThatImportEachOther)
  {
    WriteAsset("ui/a.css", "@import \"b.css\";\nlabel { opacity: 0.5; }");
    WriteAsset("ui/b.css", "@import \"a.css\";\nlabel { font-size: 20px; }");

    ASSERT_GE(Show("styles: [a.css]\nroot:\n  type: label\n  name: title\n"), 0);

    EXPECT_FLOAT_EQ(StyleOf("title").opacity, 0.5f);
    EXPECT_FLOAT_EQ(StyleOf("title").font_size, 20.0f);
    EXPECT_THAT(
      Warnings(),
      ElementsAre(
        "assets://ui/b.css:1: imports assets://ui/a.css, which imports it in turn. The sheet is left out"));
  }

  TEST_F(UiStyleSheetTest, ReadsTheFontsOfASheet)
  {
    WriteAsset("fonts/title.ttf", "a font");
    WriteAsset(
      "ui/theme.css",
      "@font-face { font-family: \"Title\"; src: url(\"../fonts/title.ttf\"); font-weight: 700; }\n"
      "label { font-family: \"Title\"; font-weight: bold; }");

    ASSERT_GE(Show("styles: [theme.css]\nroot:\n  type: label\n  name: title\n  text: Hello\n"), 0);
    Frame();

    EXPECT_EQ(StyleOf("title").font_family, "Title");
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Loaded the font assets://fonts/title.ttf"));
    EXPECT_THAT(Errors(), IsEmpty());
  }

  // writing properties

  TEST_F(UiStyleSheetTest, WritesPropertiesWithHyphensWhereAFileHasUnderscores)
  {
    ShowWith(
      "#box {\n"
      "  background-color: #ff8000;\n"
      "  flex-direction: column;\n"
      "  justify-content: space-between;\n"
      "  min-width: 50%;\n"
      "  border-width: 1px 2px 3px 4px;\n"
      "  z-index: 3;\n"
      "  pointer-events: auto;\n"
      "}\n",
      "- type: panel\n  name: box\n");

    const UiStyle &style = StyleOf("box");
    ExpectColor(style.background_color, 1.0f, 0.502f, 0.0f);
    EXPECT_EQ(style.layout.flex_direction, neon::FlexDirection::Column);
    EXPECT_EQ(style.layout.justify_content, neon::JustifyContent::SpaceBetween);
    EXPECT_EQ(style.layout.min_width, LayoutLength::Percent(50.0f));
    EXPECT_FLOAT_EQ(style.layout.border.left, 4.0f);
    EXPECT_EQ(style.z_index, 3);
    EXPECT_EQ(style.pointer_events, neon::UiPointerEvents::Auto);
    EXPECT_THAT(Warnings(), IsEmpty());
  }

  TEST_F(UiStyleSheetTest, ReadsShorthandsAndWhatTheyStandForInTheOrderTheyAreWrittenIn)
  {
    ShowWith(
      "#first { margin-top: 1px; margin: 8px 16px; }\n"
      "#second { margin: 8px 16px; margin-top: 1px; }\n"
      "#third { border: 2px solid #ff0000; padding: 4px; flex: 1; gap: 3px 6px; }\n",
      "- type: panel\n  name: first\n"
      "- type: panel\n  name: second\n"
      "- type: panel\n  name: third\n");

    // as CSS says, and other than in a file of YAML: what is written last
    EXPECT_EQ(StyleOf("first").layout.margin.top, LayoutLength::Pixels(8.0f));
    EXPECT_EQ(StyleOf("second").layout.margin.top, LayoutLength::Pixels(1.0f));
    EXPECT_EQ(StyleOf("second").layout.margin.left, LayoutLength::Pixels(16.0f));

    const UiStyle &third = StyleOf("third");
    EXPECT_FLOAT_EQ(third.layout.border.top, 2.0f);
    ExpectColor(third.BorderColor(), 1.0f, 0.0f, 0.0f);
    EXPECT_EQ(third.layout.padding.left, LayoutLength::Pixels(4.0f));
    EXPECT_FLOAT_EQ(third.layout.flex_grow, 1.0f);
    EXPECT_EQ(third.layout.flex_basis, LayoutLength::Pixels(0.0f));
    EXPECT_FLOAT_EQ(third.layout.row_gap, 3.0f);
    EXPECT_FLOAT_EQ(third.layout.column_gap, 6.0f);
  }

  TEST_F(UiStyleSheetTest, ReadsAPathInUrlAndLooksForItNextToTheSheet)
  {
    WriteAsset("themes/dark.css", "#box { background-image: url(\"images/panel.png\"); }");

    ASSERT_GE(Show(
      "styles: [assets://themes/dark.css]\n"
      "root:\n  type: panel\n  name: box\n"), 0);

    EXPECT_EQ(StyleOf("box").background_image, "assets://themes/images/panel.png");
  }

  TEST_F(UiStyleSheetTest, ReadsTheNamesOfColoursAndHslInASheet)
  {
    ShowWith(
      "#a { background-color: rebeccapurple; color: hsl(120, 100%, 50%); border: 1px solid red; }",
      "- type: panel\n  name: a\n");

    ExpectColor(StyleOf("a").background_color, 0.4f, 0.2f, 0.6f);
    ExpectColor(StyleOf("a").color, 0.0f, 1.0f, 0.0f);
    ExpectColor(StyleOf("a").BorderColor(), 1.0f, 0.0f, 0.0f);
  }

  TEST_F(UiStyleSheetTest, TakesTheFirstOfAListOfFamilies)
  {
    ShowWith("label { font-family: \"sans-serif\", Arial, serif; }", "- type: label\n  name: a\n  text: a\n");
    EXPECT_EQ(StyleOf("a").font_family, "sans-serif");
  }

  // which rule wins

  TEST_F(UiStyleSheetTest, LetsTheRuleWinWhoseSelectorCountsMore)
  {
    ShowWith(
      "#title { font-size: 40px; }\n"
      ".heading { font-size: 30px; opacity: 0.5; }\n"
      "label { font-size: 20px; opacity: 0.25; z-index: 7; }\n"
      "* { font-size: 10px; }\n",
      "- type: label\n  name: title\n  class: heading\n  text: a\n"
      "- type: label\n  name: other\n  class: heading\n  text: b\n"
      "- type: label\n  name: plain\n  text: c\n"
      "- type: panel\n  name: box\n");

    // an identifier counts more than a class, a class more than a type,
    // and a type more than the selector for everything
    EXPECT_FLOAT_EQ(StyleOf("title").font_size, 40.0f);
    EXPECT_FLOAT_EQ(StyleOf("title").opacity, 0.5f);
    EXPECT_EQ(StyleOf("title").z_index, 7);

    EXPECT_FLOAT_EQ(StyleOf("other").font_size, 30.0f);
    EXPECT_FLOAT_EQ(StyleOf("plain").font_size, 20.0f);
    EXPECT_FLOAT_EQ(StyleOf("plain").opacity, 0.25f);
    EXPECT_FLOAT_EQ(StyleOf("box").font_size, 10.0f);
  }

  TEST_F(UiStyleSheetTest, CountsEveryPartOfASelector)
  {
    ShowWith(
      "panel label.heading { opacity: 0.1; }\n"
      "label.heading { opacity: 0.2; }\n"
      ".heading { opacity: 0.3; }\n"
      "label.heading.large { opacity: 0.4; }\n",
      "- type: label\n  name: a\n  class: heading\n  text: a\n"
      "- type: label\n  name: b\n  class: [heading, large]\n  text: b\n");

    // two types and a class count more than one type and a class
    EXPECT_FLOAT_EQ(StyleOf("a").opacity, 0.1f);

    // and two classes more than one, however many types there are
    EXPECT_FLOAT_EQ(StyleOf("b").opacity, 0.4f);
  }

  TEST_F(UiStyleSheetTest, LetsWhatTheFileWritesForAnElementWinOverEverySheet)
  {
    ShowWith(
      "#title { font-size: 40px; opacity: 0.5; }",
      "- type: label\n  name: title\n  text: a\n  font_size: 12\n");

    EXPECT_FLOAT_EQ(StyleOf("title").font_size, 12.0f);
    EXPECT_FLOAT_EQ(StyleOf("title").opacity, 0.5f);
  }

  TEST_F(UiStyleSheetTest, LetsWhatIsImportantWinOverWhatTheFileWrites)
  {
    ShowWith(
      "label { font-size: 40px !important; opacity: 0.5 !important; }\n"
      "#title { opacity: 0.25; }\n",
      "- type: label\n  name: title\n  text: a\n  font_size: 12\n");

    EXPECT_FLOAT_EQ(StyleOf("title").font_size, 40.0f);
    EXPECT_FLOAT_EQ(StyleOf("title").opacity, 0.5f);
  }

  TEST_F(UiStyleSheetTest, LetsTheSelectorThatCountsMoreWinAmongWhatIsImportant)
  {
    ShowWith(
      "#title { opacity: 0.25 !important; }\n"
      "label { opacity: 0.5 !important; }\n",
      "- type: label\n  name: title\n  text: a\n");

    EXPECT_FLOAT_EQ(StyleOf("title").opacity, 0.25f);
  }

  TEST_F(UiStyleSheetTest, LetsASheetWinOverWhatTheEngineGivesAnElement)
  {
    ShowWith(
      "button { background-color: #ff0000; padding: 0; }",
      "- type: button\n  name: start\n  text: Start\n");

    ExpectColor(StyleOf("start").background_color, 1.0f, 0.0f, 0.0f);
    EXPECT_EQ(StyleOf("start").layout.padding.left, LayoutLength::Pixels(0.0f));

    // and over what the engine gives its states
    ExpectColor(StyleIn("start", {.hover = true}).background_color, 1.0f, 0.0f, 0.0f);

    // what no sheet writes is as the engine gives it
    EXPECT_EQ(StyleOf("start").text_align, neon::TextAlign::Center);
    EXPECT_FLOAT_EQ(StyleIn("start", {.focus = true}).outline_width, 2.0f);
  }

  // states

  TEST_F(UiStyleSheetTest, FollowsTheStatesOfAnElement)
  {
    ShowWith(
      "button { background-color: #000000; }\n"
      "button:hover { background-color: #ff0000; }\n"
      "button:active { background-color: #00ff00; }\n"
      "button:focus { opacity: 0.5; }\n"
      "button:disabled { background-color: #0000ff; }\n",
      "- type: button\n  name: start\n  text: Start\n");

    ExpectColor(StyleIn("start", {}).background_color, 0.0f, 0.0f, 0.0f);
    ExpectColor(StyleIn("start", {.hover = true}).background_color, 1.0f, 0.0f, 0.0f);
    ExpectColor(StyleIn("start", {.hover = true, .active = true}).background_color, 0.0f, 1.0f, 0.0f);
    EXPECT_FLOAT_EQ(StyleIn("start", {.focus = true}).opacity, 0.5f);
    ExpectColor(StyleIn("start", {.disabled = true}).background_color, 0.0f, 0.0f, 1.0f);
  }

  TEST_F(UiStyleSheetTest, TakesTheOrderOfTheSheetForStatesAndNotAnOrderOfItsOwn)
  {
    ShowWith(
      "button:active { background-color: #00ff00; }\n"
      "button:hover { background-color: #ff0000; }\n",
      "- type: button\n  name: start\n  text: Start\n");

    // both count the same, and hover is written last
    ExpectColor(StyleIn("start", {.hover = true, .active = true}).background_color, 1.0f, 0.0f, 0.0f);
  }

  TEST_F(UiStyleSheetTest, CountsTheStatesOfAFileAsWrittenForTheElement)
  {
    ShowWith(
      "#start:hover { background-color: #ff0000; opacity: 0.5; }",
      "- type: button\n"
      "  name: start\n"
      "  text: Start\n"
      "  hover:\n"
      "    background_color: \"#0000ff\"\n");

    const UiStyle style = StyleIn("start", {.hover = true});
    ExpectColor(style.background_color, 0.0f, 0.0f, 1.0f);
    EXPECT_FLOAT_EQ(style.opacity, 0.5f);

    ExpectColor(StyleIn("start", {}).background_color, 0.231f, 0.259f, 0.322f);
  }

  TEST_F(UiStyleSheetTest, FollowsTheStateOfAnElementItIsUnderThePointerWith)
  {
    ShowWith(
      "button { width: 100px; height: 50px; box-sizing: border-box; }\n"
      "button:hover { background-color: #ff0000; }\n",
      "- type: button\n  name: start\n  text: Start\n");

    PointAt(50, 25);
    Frame();
    ExpectColor(StyleOf("start").background_color, 1.0f, 0.0f, 0.0f);

    PointAt(500, 500);
    Frame();
    ExpectColor(StyleOf("start").background_color, 0.231f, 0.259f, 0.322f);
  }

  TEST_F(UiStyleSheetTest, StylesWhatIsInsideAnElementThatIsUnderThePointer)
  {
    ShowWith(
      "#row { width: 200px; height: 50px; pointer-events: auto; }\n"
      "#row:hover label { color: #ff0000; }\n"
      "#row:hover + label { color: #00ff00; }\n",
      "- type: panel\n"
      "  name: row\n"
      "  children:\n"
      "    - type: label\n"
      "      name: inside\n"
      "      text: a\n"
      "- type: label\n"
      "  name: behind\n"
      "  text: b\n");

    ExpectColor(StyleOf("inside").color, 1.0f, 1.0f, 1.0f);
    ExpectColor(StyleOf("behind").color, 1.0f, 1.0f, 1.0f);

    PointAt(100, 25);
    Frame();
    ExpectColor(StyleOf("inside").color, 1.0f, 0.0f, 0.0f);
    ExpectColor(StyleOf("behind").color, 0.0f, 1.0f, 0.0f);

    PointAt(900, 900);
    Frame();
    ExpectColor(StyleOf("inside").color, 1.0f, 1.0f, 1.0f);
    ExpectColor(StyleOf("behind").color, 1.0f, 1.0f, 1.0f);
  }

  TEST_F(UiStyleSheetTest, KnowsWhenTheFocusIsInsideAnElement)
  {
    ShowWith(
      "#group:focus-within { background-color: #ff0000; }\n"
      "button:focus { opacity: 0.5; }\n",
      "- type: panel\n"
      "  name: group\n"
      "  children:\n"
      "    - type: button\n"
      "      name: inside\n"
      "      text: a\n"
      "- type: button\n"
      "  name: outside\n"
      "  text: b\n");

    ASSERT_TRUE(_ui->Focus("inside"));
    Frame();
    ExpectColor(StyleOf("group").background_color, 1.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(StyleOf("inside").opacity, 0.5f);

    ASSERT_TRUE(_ui->Focus("outside"));
    Frame();
    ExpectColor(StyleOf("group").background_color, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(StyleOf("inside").opacity, 1.0f);
  }

  TEST_F(UiStyleSheetTest, FollowsWhetherAnElementCanBeUsed)
  {
    ShowWith(
      "button:disabled { background-color: #ff0000; }\n"
      "button:enabled { opacity: 0.5; }\n",
      "- type: button\n  name: load\n  text: Load\n  enabled: \"{has_save}\"\n");

    _ui->SetFlag("has_save", false);
    Frame();
    ExpectColor(StyleOf("load").background_color, 1.0f, 0.0f, 0.0f);

    _ui->SetFlag("has_save", true);
    Frame();
    ExpectColor(StyleOf("load").background_color, 0.231f, 0.259f, 0.322f);
    EXPECT_FLOAT_EQ(StyleOf("load").opacity, 0.5f);
  }

  // what is inherited

  TEST_F(UiStyleSheetTest, HandsWhatCssInheritsToWhatIsInside)
  {
    ShowWith(
      "#menu {\n"
      "  color: #ff0000;\n"
      "  font-size: 24px;\n"
      "  font-weight: bold;\n"
      "  line-height: 1.5;\n"
      "  text-align: right;\n"
      "  visibility: hidden;\n"
      "  cursor: pointer;\n"
      "}\n",
      "- type: panel\n"
      "  name: menu\n"
      "  children:\n"
      "    - type: panel\n"
      "      name: row\n"
      "      children:\n"
      "        - type: label\n"
      "          name: text\n"
      "          text: a\n");

    for (const std::string name : {"row", "text"})
    {
      const UiStyle &style = StyleOf(name);
      ExpectColor(style.color, 1.0f, 0.0f, 0.0f);
      EXPECT_FLOAT_EQ(style.font_size, 24.0f) << name;
      EXPECT_EQ(style.font_weight, 700) << name;
      EXPECT_FLOAT_EQ(style.line_height, 1.5f) << name;
      EXPECT_TRUE(style.line_height_is_multiple) << name;
      EXPECT_EQ(style.text_align, neon::TextAlign::Right) << name;
      EXPECT_EQ(style.visibility, neon::UiVisibility::Hidden) << name;
      EXPECT_EQ(style.cursor, neon::UiCursor::Pointer) << name;
    }
  }

  TEST_F(UiStyleSheetTest, KeepsWhatCssDoesNotInheritToTheElement)
  {
    ShowWith(
      "#menu { background-color: #ff0000; opacity: 0.5; padding: 8px; width: 300px; border-width: 2px; }",
      "- type: panel\n"
      "  name: menu\n"
      "  children:\n"
      "    - type: label\n"
      "      name: text\n"
      "      text: a\n");

    const UiStyle &style = StyleOf("text");
    ExpectColor(style.background_color, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(style.opacity, 1.0f);
    EXPECT_EQ(style.layout.padding.left, LayoutLength::Pixels(0.0f));
    EXPECT_TRUE(style.layout.width.IsAuto());
    EXPECT_FLOAT_EQ(style.layout.border.left, 0.0f);
  }

  TEST_F(UiStyleSheetTest, InheritsFromAFileOfYamlAsWell)
  {
    ASSERT_GE(Show(
      "root:\n"
      "  type: panel\n"
      "  name: root\n"
      "  color: \"#00ff00\"\n"
      "  font_size: 30\n"
      "  children:\n"
      "    - type: label\n"
      "      name: text\n"
      "      text: a\n"), 0);

    ExpectColor(StyleOf("text").color, 0.0f, 1.0f, 0.0f);
    EXPECT_FLOAT_EQ(StyleOf("text").font_size, 30.0f);
  }

  TEST_F(UiStyleSheetTest, LetsAnElementSayItselfWhatItWouldInherit)
  {
    ShowWith(
      "#menu { color: #ff0000; }\n"
      "#own { color: #0000ff; }\n",
      "- type: panel\n"
      "  name: menu\n"
      "  children:\n"
      "    - type: label\n"
      "      name: own\n"
      "      text: a\n"
      "    - type: label\n"
      "      name: other\n"
      "      text: b\n");

    ExpectColor(StyleOf("own").color, 0.0f, 0.0f, 1.0f);
    ExpectColor(StyleOf("other").color, 1.0f, 0.0f, 0.0f);
  }

  TEST_F(UiStyleSheetTest, DoesNotInheritWhatTheEngineGivesAnElementItself)
  {
    // a button has a colour of its own, as it has in a browser
    ShowWith(
      "#menu { color: #ff0000; font-size: 24px; }",
      "- type: panel\n"
      "  name: menu\n"
      "  children:\n"
      "    - type: button\n"
      "      name: start\n"
      "      text: Start\n");

    ExpectColor(StyleOf("start").color, 1.0f, 1.0f, 1.0f);
    EXPECT_FLOAT_EQ(StyleOf("start").font_size, 24.0f);
  }

  TEST_F(UiStyleSheetTest, FollowsWhatIsInheritedWhenItChanges)
  {
    ShowWith(
      "#menu { pointer-events: auto; width: 200px; height: 100px; }\n"
      "#menu:hover { color: #ff0000; }\n",
      "- type: panel\n"
      "  name: menu\n"
      "  children:\n"
      "    - type: panel\n"
      "      children:\n"
      "        - type: label\n"
      "          name: text\n"
      "          text: a\n");

    ExpectColor(StyleOf("text").color, 1.0f, 1.0f, 1.0f);

    PointAt(100, 50);
    Frame();
    ExpectColor(StyleOf("text").color, 1.0f, 0.0f, 0.0f);

    PointAt(900, 900);
    Frame();
    ExpectColor(StyleOf("text").color, 1.0f, 1.0f, 1.0f);
  }

  TEST_F(UiStyleSheetTest, TakesTheValueOfTheElementAboveWithInherit)
  {
    ShowWith(
      "#menu { background-color: #ff0000; opacity: 0.5; padding: 8px 16px; border: 2px solid #00ff00; }\n"
      "#text { background-color: inherit; opacity: inherit; padding: inherit; border: inherit; }\n",
      "- type: panel\n"
      "  name: menu\n"
      "  children:\n"
      "    - type: label\n"
      "      name: text\n"
      "      text: a\n");

    const UiStyle &style = StyleOf("text");
    ExpectColor(style.background_color, 1.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(style.opacity, 0.5f);
    EXPECT_EQ(style.layout.padding.top, LayoutLength::Pixels(8.0f));
    EXPECT_EQ(style.layout.padding.left, LayoutLength::Pixels(16.0f));
    EXPECT_FLOAT_EQ(style.layout.border.left, 2.0f);
    ExpectColor(style.BorderColor(), 0.0f, 1.0f, 0.0f);
  }

  TEST_F(UiStyleSheetTest, TakesTheInitialValueWithInitial)
  {
    ShowWith(
      "#menu { color: #ff0000; font-size: 40px; }\n"
      "#text { color: initial; font-size: initial; }\n"
      "button { background-color: initial; padding: initial; }\n",
      "- type: panel\n"
      "  name: menu\n"
      "  children:\n"
      "    - type: label\n"
      "      name: text\n"
      "      text: a\n"
      "    - type: button\n"
      "      name: start\n"
      "      text: b\n");

    // what CSS starts a property with, and neither what is inherited nor
    // what the engine gives a button
    ExpectColor(StyleOf("text").color, 1.0f, 1.0f, 1.0f);
    EXPECT_FLOAT_EQ(StyleOf("text").font_size, 16.0f);
    ExpectColor(StyleOf("start").background_color, 0.0f, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(StyleOf("start").layout.padding.left, LayoutLength::Pixels(0.0f));
  }

  TEST_F(UiStyleSheetTest, InheritsWhatIsInheritedAndStartsTheRestWithUnset)
  {
    ShowWith(
      "#menu { color: #ff0000; opacity: 0.5; }\n"
      "label { color: #0000ff; opacity: 0.25; }\n"
      "#text { color: unset; opacity: unset; }\n",
      "- type: panel\n"
      "  name: menu\n"
      "  children:\n"
      "    - type: label\n"
      "      name: text\n"
      "      text: a\n");

    ExpectColor(StyleOf("text").color, 1.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(StyleOf("text").opacity, 1.0f);
  }

  TEST_F(UiStyleSheetTest, GoesBackToWhatTheEngineGivesWithRevert)
  {
    ShowWith(
      "button { background-color: #ff0000; padding: 0; }\n"
      "#start { background-color: revert; padding: revert; }\n",
      "- type: button\n  name: start\n  text: a\n");

    ExpectColor(StyleOf("start").background_color, 0.231f, 0.259f, 0.322f);
    EXPECT_EQ(StyleOf("start").layout.padding.left, LayoutLength::Pixels(16.0f));
  }

  TEST_F(UiStyleSheetTest, ReadsInheritInAFileOfYamlAsWell)
  {
    ASSERT_GE(Show(
      "root:\n"
      "  type: panel\n"
      "  name: root\n"
      "  opacity: 0.5\n"
      "  color: \"#ff0000\"\n"
      "  children:\n"
      "    - type: label\n"
      "      name: text\n"
      "      text: a\n"
      "      opacity: inherit\n"
      "      color: initial\n"), 0) << _logger->Messages(LogLevel::Error);

    EXPECT_FLOAT_EQ(StyleOf("text").opacity, 0.5f);
    ExpectColor(StyleOf("text").color, 1.0f, 1.0f, 1.0f);
  }

  // units

  TEST_F(UiStyleSheetTest, TakesAnEmForTheSizeOfTheFontOfTheElement)
  {
    ShowWith(
      "#menu { font-size: 20px; }\n"
      "#text { width: 10em; padding: 0.5em; }\n"
      "#large { font-size: 2em; width: 10em; }\n",
      "- type: panel\n"
      "  name: menu\n"
      "  children:\n"
      "    - type: label\n"
      "      name: text\n"
      "      text: a\n"
      "    - type: label\n"
      "      name: large\n"
      "      text: b\n");

    EXPECT_EQ(StyleOf("text").layout.width, LayoutLength::Pixels(200.0f));
    EXPECT_EQ(StyleOf("text").layout.padding.left, LayoutLength::Pixels(10.0f));

    // of the size of the font, an `em` is that of the element above. Of
    // everything else it is what the size of the font came to
    EXPECT_FLOAT_EQ(StyleOf("large").font_size, 40.0f);
    EXPECT_EQ(StyleOf("large").layout.width, LayoutLength::Pixels(400.0f));
  }

  TEST_F(UiStyleSheetTest, TakesTheSizeOfTheFontForAnEmWhereverTheTwoAreWritten)
  {
    ShowWith(
      "#text { width: 10em; }\n"
      "#text { font-size: 30px; }\n",
      "- type: label\n  name: text\n  text: a\n");

    EXPECT_EQ(StyleOf("text").layout.width, LayoutLength::Pixels(300.0f));
  }

  TEST_F(UiStyleSheetTest, TakesAPercentageOfTheSizeOfTheFontOfTheElementAbove)
  {
    ShowWith(
      "#menu { font-size: 20px; }\n"
      "#text { font-size: 150%; }\n",
      "- type: panel\n"
      "  name: menu\n"
      "  children:\n"
      "    - type: label\n"
      "      name: text\n"
      "      text: a\n");

    EXPECT_FLOAT_EQ(StyleOf("text").font_size, 30.0f);
  }

  TEST_F(UiStyleSheetTest, TakesARemForTheSizeOfTheFontOfTheRoot)
  {
    ShowWith(
      ":root { font-size: 10px; }\n"
      "#menu { font-size: 40px; }\n"
      "#text { width: 12rem; font-size: 2rem; }\n",
      "- type: panel\n"
      "  name: menu\n"
      "  children:\n"
      "    - type: label\n"
      "      name: text\n"
      "      text: a\n");

    EXPECT_EQ(StyleOf("text").layout.width, LayoutLength::Pixels(120.0f));
    EXPECT_FLOAT_EQ(StyleOf("text").font_size, 20.0f);
  }

  TEST_F(UiStyleSheetTest, TakesTheUnitsOfTheViewportForAHundredthOfWhatIsShown)
  {
    // 1920 by 1080, at a scale of 1
    ShowWith(
      "#box { width: 50vw; height: 25vh; margin-left: 10vmin; margin-top: 10vmax; }",
      "- type: panel\n  name: box\n");

    const UiStyle &style = StyleOf("box");
    EXPECT_EQ(style.layout.width, LayoutLength::Pixels(960.0f));
    EXPECT_EQ(style.layout.height, LayoutLength::Pixels(270.0f));
    EXPECT_EQ(style.layout.margin.left, LayoutLength::Pixels(108.0f));
    EXPECT_EQ(style.layout.margin.top, LayoutLength::Pixels(192.0f));

    ExpectBox("box", 108.0f, 192.0f, 960.0f, 270.0f);
  }

  TEST_F(UiStyleSheetTest, FollowsTheViewportWhenItChanges)
  {
    ShowWith("#box { width: 50vw; height: 50vh; }", "- type: panel\n  name: box\n");
    ExpectBox("box", 0.0f, 0.0f, 960.0f, 540.0f);

    // 1280 by 1024 is 1920 by 1536 in units of a file made for 1920 by 1080
    _renderer.SetResolution(1280, 1024);
    Frame();

    ExpectBox("box", 0.0f, 0.0f, 960.0f, 768.0f);
  }

  TEST_F(UiStyleSheetTest, WorksOutCalc)
  {
    ShowWith(
      "#menu { width: 400px; height: 200px; padding: 0; }\n"
      "#box {\n"
      "  width: calc(100% - 40px);\n"
      "  height: calc(50% + 2 * 10px);\n"
      "  margin-left: calc((4px + 6px) * 2);\n"
      "  flex-grow: calc(1 + 1);\n"
      "  flex-shrink: 0;\n"
      "}\n",
      "- type: panel\n"
      "  name: menu\n"
      "  children:\n"
      "    - type: panel\n"
      "      name: box\n");

    const UiStyle &style = StyleOf("box");
    EXPECT_EQ(style.layout.width, LayoutLength::Sum(-40.0f, 100.0f));
    EXPECT_EQ(style.layout.height, LayoutLength::Sum(20.0f, 50.0f));
    EXPECT_EQ(style.layout.margin.left, LayoutLength::Pixels(20.0f));
    EXPECT_FLOAT_EQ(style.layout.flex_grow, 2.0f);

    // of a box of 400 by 200: 360 wide, and 120 high
    EXPECT_NEAR(Element("box").GetBox().Height(), 120.0f, 0.01f);
    EXPECT_NEAR(Element("box").GetBox().left, 20.0f, 0.01f);
  }

  TEST_F(UiStyleSheetTest, ReadsUnitsAndCalcInAFileOfYamlAsWell)
  {
    ASSERT_GE(Show(
      "root:\n"
      "  type: panel\n"
      "  name: root\n"
      "  font_size: 20\n"
      "  width: 50vw\n"
      "  height: calc(100vh - 80px)\n"
      "  padding: 1em 2rem\n"
      "  children:\n"
      "    - type: panel\n"
      "      name: box\n"
      "      width: calc(100% - 2em)\n"
      "      height: 3em\n"), 0) << _logger->Messages(LogLevel::Error);

    const UiStyle &root = StyleOf("root");
    EXPECT_EQ(root.layout.width, LayoutLength::Pixels(960.0f));
    EXPECT_EQ(root.layout.height, LayoutLength::Pixels(1000.0f));
    EXPECT_EQ(root.layout.padding.top, LayoutLength::Pixels(20.0f));
    EXPECT_EQ(root.layout.padding.right, LayoutLength::Pixels(40.0f));

    EXPECT_EQ(StyleOf("box").layout.width, LayoutLength::Sum(-40.0f, 100.0f));
    EXPECT_EQ(StyleOf("box").layout.height, LayoutLength::Pixels(60.0f));
  }

  TEST_F(UiStyleSheetTest, SaysWhatIsWrongWithCalcInAFileOfYaml)
  {
    ExpectProblems(
      "root:\n"
      "  type: panel\n"
      "  name: box\n"
      "  width: calc(100% - )\n"
      "  height: calc(10px * 2px)\n",
      {
        "assets://ui/test.ui.yml:4: 'width' of panel 'box' is 'calc(100% - )': calc() ends where a value was "
        "expected",
        "assets://ui/test.ui.yml:5: 'height' of panel 'box' is 'calc(10px * 2px)': calc() multiplies two "
        "lengths, where one of the two has to be a number"
      });
  }

  // custom properties

  TEST_F(UiStyleSheetTest, MakesAThemeOfAHandfulOfVariables)
  {
    ShowWith(
      ":root {\n"
      "  --accent: #ff8000;\n"
      "  --gap: 8px;\n"
      "  --panel: rgba(0, 0, 0, 0.5);\n"
      "}\n"
      "panel { background-color: var(--panel); padding: var(--gap) calc(var(--gap) * 2); }\n"
      "button { background-color: var(--accent); margin: var(--gap); }\n"
      "label { color: var(--accent); }\n",
      "- type: panel\n"
      "  name: menu\n"
      "  children:\n"
      "    - type: label\n"
      "      name: title\n"
      "      text: a\n"
      "    - type: button\n"
      "      name: start\n"
      "      text: b\n");

    ExpectColor(StyleOf("menu").background_color, 0.0f, 0.0f, 0.0f, 0.5f);
    EXPECT_EQ(StyleOf("menu").layout.padding.top, LayoutLength::Pixels(8.0f));
    EXPECT_EQ(StyleOf("menu").layout.padding.left, LayoutLength::Pixels(16.0f));
    ExpectColor(StyleOf("start").background_color, 1.0f, 0.502f, 0.0f);
    EXPECT_EQ(StyleOf("start").layout.margin.left, LayoutLength::Pixels(8.0f));
    ExpectColor(StyleOf("title").color, 1.0f, 0.502f, 0.0f);
    EXPECT_THAT(Warnings(), IsEmpty());
  }

  TEST_F(UiStyleSheetTest, InheritsCustomPropertiesAndLetsAnElementSetItsOwn)
  {
    ShowWith(
      ":root { --accent: #ff0000; }\n"
      "#danger { --accent: #0000ff; }\n"
      "label { color: var(--accent); }\n",
      "- type: label\n"
      "  name: plain\n"
      "  text: a\n"
      "- type: panel\n"
      "  name: danger\n"
      "  children:\n"
      "    - type: label\n"
      "      name: warning\n"
      "      text: b\n");

    ExpectColor(StyleOf("plain").color, 1.0f, 0.0f, 0.0f);
    ExpectColor(StyleOf("warning").color, 0.0f, 0.0f, 1.0f);
  }

  TEST_F(UiStyleSheetTest, TakesWhatStandsInForACustomPropertyThatIsNotSet)
  {
    ShowWith(
      "#a { opacity: var(--missing, 0.25); width: var(--missing, var(--gone, 120px)); }",
      "- type: panel\n  name: a\n");

    EXPECT_FLOAT_EQ(StyleOf("a").opacity, 0.25f);
    EXPECT_EQ(StyleOf("a").layout.width, LayoutLength::Pixels(120.0f));
    EXPECT_THAT(Warnings(), IsEmpty());
  }

  TEST_F(UiStyleSheetTest, LeavesOutADeclarationWhoseCustomPropertyIsNotSetAndSaysSo)
  {
    ShowWith(
      "#a {\n"
      "  opacity: 0.5;\n"
      "  opacity: var(--missing);\n"
      "  width: 100px;\n"
      "}\n",
      "- type: panel\n  name: a\n");

    EXPECT_FLOAT_EQ(StyleOf("a").opacity, 0.5f);
    EXPECT_EQ(StyleOf("a").layout.width, LayoutLength::Pixels(100.0f));
    EXPECT_THAT(
      Warnings(),
      ElementsAre(
        "assets://ui/theme.css:3: 'opacity' of the rule '#a' is 'var(--missing)': the custom property "
        "'--missing' is not set, and var() names nothing in its place. The declaration is left out"));
  }

  TEST_F(UiStyleSheetTest, SaysWhatIsWrongWithACustomPropertyOnceAndNotInEveryFrame)
  {
    ShowWith("#a:hover, #a { opacity: var(--missing); }", "- type: panel\n  name: a\n  pointer_events: auto\n");

    for (int i = 0; i < 5; i++)
    {
      PointAt(i % 2 == 0 ? 10 : 1900, 10);
      Frame();
    }

    EXPECT_EQ(Warnings().size(), 1u);
  }

  TEST_F(UiStyleSheetTest, UsesACustomPropertyInAFileOfYaml)
  {
    WriteAsset("ui/theme.css", ":root { --accent: #ff8000; --gap: 12px; }");

    ASSERT_GE(Show(
      "styles: [theme.css]\n"
      "root:\n"
      "  type: panel\n"
      "  name: root\n"
      "  background_color: var(--accent)\n"
      "  padding: var(--gap)\n"
      "  gap: var(--missing, 4px)\n"), 0) << _logger->Messages(LogLevel::Error);

    ExpectColor(StyleOf("root").background_color, 1.0f, 0.502f, 0.0f);
    EXPECT_EQ(StyleOf("root").layout.padding.left, LayoutLength::Pixels(12.0f));
    EXPECT_FLOAT_EQ(StyleOf("root").layout.row_gap, 4.0f);
  }

  TEST_F(UiStyleSheetTest, SetsACustomPropertyInAFileOfYaml)
  {
    WriteAsset("ui/theme.css", "label { color: var(--accent, #ffffff); }");

    ASSERT_GE(Show(
      "styles: [theme.css]\n"
      "root:\n"
      "  type: panel\n"
      "  --accent: \"#00ff00\"\n"
      "  children:\n"
      "    - type: label\n"
      "      name: text\n"
      "      text: a\n"), 0) << _logger->Messages(LogLevel::Error);

    ExpectColor(StyleOf("text").color, 0.0f, 1.0f, 0.0f);
  }

  // media queries

  TEST_F(UiStyleSheetTest, FollowsTheSizeOfTheWindow)
  {
    ShowWith(
      "#box { opacity: 1; }\n"
      "@media (max-width: 1280px) { #box { opacity: 0.5; } }\n"
      "@media (orientation: portrait) { #box { z-index: 9; } }\n",
      "- type: panel\n  name: box\n");

    EXPECT_FLOAT_EQ(StyleOf("box").opacity, 1.0f);
    EXPECT_EQ(StyleOf("box").z_index, 0);

    _renderer.SetResolution(1280, 720);
    Frame();
    EXPECT_FLOAT_EQ(StyleOf("box").opacity, 0.5f);
    EXPECT_EQ(StyleOf("box").z_index, 0);

    _renderer.SetResolution(720, 1280);
    Frame();
    EXPECT_FLOAT_EQ(StyleOf("box").opacity, 0.5f);
    EXPECT_EQ(StyleOf("box").z_index, 9);

    _renderer.SetResolution(1920, 1080);
    Frame();
    EXPECT_FLOAT_EQ(StyleOf("box").opacity, 1.0f);
    EXPECT_EQ(StyleOf("box").z_index, 0);
  }

  TEST_F(UiStyleSheetTest, AsksWhetherLessMotionIsWanted)
  {
    ShowWith(
      "#box { opacity: 1; }\n"
      "@media (prefers-reduced-motion: reduce) { #box { opacity: 0.5; } }\n",
      "- type: panel\n  name: box\n");

    EXPECT_FLOAT_EQ(StyleOf("box").opacity, 1.0f);

    _ui->SetReducedMotion(true);
    Frame();
    EXPECT_FLOAT_EQ(StyleOf("box").opacity, 0.5f);

    _ui->SetReducedMotion(false);
    Frame();
    EXPECT_FLOAT_EQ(StyleOf("box").opacity, 1.0f);
  }

  TEST_F(UiStyleSheetTest, TakesTheConditionOfAnImportForEverythingItImports)
  {
    WriteAsset("ui/small.css", "#box { opacity: 0.5; }");
    WriteAsset("ui/theme.css", "@import \"small.css\" (max-width: 1280px);");

    ASSERT_GE(Show("styles: [theme.css]\nroot:\n  type: panel\n  name: box\n"), 0);
    Frame();
    EXPECT_FLOAT_EQ(StyleOf("box").opacity, 1.0f);

    _renderer.SetResolution(1280, 720);
    Frame();
    EXPECT_FLOAT_EQ(StyleOf("box").opacity, 0.5f);
  }

  // what is wrong with a sheet

  TEST_F(UiStyleSheetTest, SaysWhatIsWrongWithASheetAndShowsTheFileAllTheSame)
  {
    ShowWith(
      "label {\n"
      "  colour: #ff0000;\n"
      "  font-size: large;\n"
      "  opacity: 0.5;\n"
      "  width: 12ex;\n"
      "  background-color: bright;\n"
      "  display: block;\n"
      "}\n"
      "label:visited { opacity: 0.1; }\n"
      "#title { z-index 3; font-weight: bold }\n",
      "- type: label\n  name: title\n  text: a\n");

    // what can be read is used
    EXPECT_FLOAT_EQ(StyleOf("title").opacity, 0.5f);
    EXPECT_EQ(StyleOf("title").font_weight, 700);
    EXPECT_FLOAT_EQ(StyleOf("title").font_size, 16.0f);

    EXPECT_THAT(Errors(), IsEmpty());

    const auto warnings = Warnings();
    ASSERT_EQ(warnings.size(), 7u) << ::testing::PrintToString(warnings);

    EXPECT_THAT(warnings[0], HasSubstr(
                  "assets://ui/theme.css:9: the selector 'label:visited' cannot be read: ':visited' is not a "
                  "pseudo-class that is known"));
    EXPECT_EQ(
      warnings[1],
      "assets://ui/theme.css:10: 'z-index 3' of the rule '#title' has no colon, where a declaration such as "
      "color: red was expected");
    EXPECT_EQ(
      warnings[2],
      "assets://ui/theme.css:2: 'colour' of the rule 'label' is not a property that is known. Near to it "
      "are: color. The declaration is left out");
    EXPECT_EQ(
      warnings[3],
      "assets://ui/theme.css:3: 'font-size' of the rule 'label' is 'large', where a number of pixels that "
      "is not below 0 was expected. The declaration is left out");
    EXPECT_EQ(
      warnings[4],
      "assets://ui/theme.css:5: 'width' of the rule 'label' is '12ex', where a number of pixels, a "
      "percentage such as 50%, or auto was expected. The declaration is left out");
    EXPECT_THAT(warnings[5], HasSubstr(
                  "assets://ui/theme.css:6: 'background-color' of the rule 'label' is 'bright', where a colour"));
    EXPECT_EQ(
      warnings[6],
      "assets://ui/theme.css:7: 'display' of the rule 'label' is 'block', where one of these was expected: "
      "flex, none. The declaration is left out");
  }

  TEST_F(UiStyleSheetTest, KeepsTheValueOfTheRuleBelowWhenADeclarationCannotBeRead)
  {
    ShowWith(
      "label { opacity: 0.5; }\n"
      "#title { opacity: most; }\n",
      "- type: label\n  name: title\n  text: a\n");

    EXPECT_FLOAT_EQ(StyleOf("title").opacity, 0.5f);
    EXPECT_EQ(Warnings().size(), 1u);
  }

  // reading the sheets again

  TEST_F(UiStyleSheetTest, ReadsTheSheetsAgainWhileTheFileIsShown)
  {
    ShowWith("#box { opacity: 0.5; width: 100px; height: 100px; }", "- type: panel\n  name: box\n");
    EXPECT_FLOAT_EQ(StyleOf("box").opacity, 0.5f);
    ExpectBox("box", 0.0f, 0.0f, 100.0f, 100.0f);

    WriteAsset("ui/theme.css", "#box { opacity: 0.25; width: 300px; height: 50px; z-index: 4; }");
    ASSERT_TRUE(_ui->ReloadStyles());
    Frame();

    EXPECT_FLOAT_EQ(StyleOf("box").opacity, 0.25f);
    EXPECT_EQ(StyleOf("box").z_index, 4);
    ExpectBox("box", 0.0f, 0.0f, 300.0f, 50.0f);
  }

  TEST_F(UiStyleSheetTest, ReadsTheSheetsOfOneFileAgain)
  {
    WriteAsset("ui/theme.css", "#box { opacity: 0.5; }");
    const int document = Show("styles: [theme.css]\nroot:\n  type: panel\n  name: box\n");
    ASSERT_GE(document, 0);
    Frame();

    WriteAsset("ui/theme.css", "#box { opacity: 0.75; }");

    // another file is none of it
    EXPECT_TRUE(_ui->ReloadStyles(document + 7));
    Frame();
    EXPECT_FLOAT_EQ(StyleOf("box").opacity, 0.5f);

    EXPECT_TRUE(_ui->ReloadStyles(document));
    Frame();
    EXPECT_FLOAT_EQ(StyleOf("box").opacity, 0.75f);
  }

  TEST_F(UiStyleSheetTest, KeepsTheSheetsItHasWhenTheyCannotBeReadAgain)
  {
    WriteAsset("ui/parts.css", "#box { opacity: 0.5; }");
    WriteAsset("ui/theme.css", "@import \"parts.css\";");

    ASSERT_GE(Show("styles: [theme.css]\nroot:\n  type: panel\n  name: box\n"), 0);
    Frame();

    WriteAsset("ui/theme.css", "@import \"gone.css\";\n#box { opacity: 0.1; }");

    EXPECT_FALSE(_ui->ReloadStyles());
    Frame();

    EXPECT_FLOAT_EQ(StyleOf("box").opacity, 0.5f);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "the style sheet assets://ui/gone.css cannot be read"));
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "The style sheets of assets://ui/test.ui.yml are left as they were"));
  }

  TEST_F(UiStyleSheetTest, SaysWhatIsWrongWithASheetThatIsReadAgain)
  {
    ShowWith("#box { opacity: 0.5; }", "- type: panel\n  name: box\n");
    EXPECT_THAT(Warnings(), IsEmpty());

    WriteAsset("ui/theme.css", "#box { opacity: 0.25; colour: red; }");
    ASSERT_TRUE(_ui->ReloadStyles());
    Frame();

    EXPECT_FLOAT_EQ(StyleOf("box").opacity, 0.25f);
    EXPECT_EQ(Warnings().size(), 1u);
  }

  // classes

  TEST_F(UiStyleSheetTest, ReadsClassesAsTextAndAsAList)
  {
    ShowWith(
      ".a { opacity: 0.5; }\n.b { z-index: 2; }\n",
      "- type: panel\n  name: one\n  class: a\n"
      "- type: panel\n  name: two\n  class: a b\n"
      "- type: panel\n  name: three\n  class: [b, a]\n"
      "- type: panel\n  name: none\n");

    EXPECT_THAT(Element("one").GetClasses(), ElementsAre("a"));
    EXPECT_THAT(Element("two").GetClasses(), ElementsAre("a", "b"));
    EXPECT_THAT(Element("three").GetClasses(), ElementsAre("b", "a"));
    EXPECT_THAT(Element("none").GetClasses(), IsEmpty());

    EXPECT_FLOAT_EQ(StyleOf("one").opacity, 0.5f);
    EXPECT_EQ(StyleOf("one").z_index, 0);
    EXPECT_EQ(StyleOf("two").z_index, 2);
    EXPECT_FLOAT_EQ(StyleOf("three").opacity, 0.5f);
    EXPECT_FLOAT_EQ(StyleOf("none").opacity, 1.0f);
  }

  TEST_F(UiStyleSheetTest, SaysThatAClassIsNoName)
  {
    ExpectProblemsUnderRoot(
      "- type: panel\n  name: a\n  class: 3\n"
      "- type: panel\n  name: b\n  class: .row\n"
      "- type: panel\n  name: c\n  class: [row, [x]]\n",
      {
        "assets://ui/test.ui.yml:6: 'class' of panel 'a' is a number, where names of classes were expected, "
        "as text with spaces between them or as a list",
        "assets://ui/test.ui.yml:9: 'class' of panel 'b' is '.row', where names of classes were expected, as "
        "text with spaces between them or as a list",
        "assets://ui/test.ui.yml:12: 'class' of panel 'c' is a list, where names of classes were expected, "
        "as text with spaces between them or as a list"
      });
  }

  TEST_F(UiStyleSheetTest, ReachesTheFieldsOfAnElementWithAnAttributeSelector)
  {
    ShowWith(
      "[src] { opacity: 0.5; }\n"
      "image[object_fit=contain] { z-index: 3; }\n"
      "[title~=second] { z-index: 5; }\n",
      "- type: image\n  name: heart\n  src: assets://ui/heart.png\n  object_fit: contain\n"
      "- type: panel\n  name: box\n  title: the second box\n");

    EXPECT_FLOAT_EQ(StyleOf("heart").opacity, 0.5f);
    EXPECT_EQ(StyleOf("heart").z_index, 3);
    EXPECT_FLOAT_EQ(StyleOf("box").opacity, 1.0f);
    EXPECT_EQ(StyleOf("box").z_index, 5);
  }

  TEST_F(UiStyleSheetTest, StylesByWhereAnElementIsAmongThoseNextToIt)
  {
    ShowWith(
      "#list > label:first-child { opacity: 0.1; }\n"
      "#list > label:last-child { opacity: 0.9; }\n"
      "#list > label:nth-child(even) { z-index: 2; }\n",
      "- type: panel\n"
      "  name: list\n"
      "  children:\n"
      "    - {type: label, name: a, text: a}\n"
      "    - {type: label, name: b, text: b}\n"
      "    - {type: label, name: c, text: c}\n"
      "    - {type: label, name: d, text: d}\n");

    EXPECT_FLOAT_EQ(StyleOf("a").opacity, 0.1f);
    EXPECT_FLOAT_EQ(StyleOf("b").opacity, 1.0f);
    EXPECT_FLOAT_EQ(StyleOf("d").opacity, 0.9f);
    EXPECT_EQ(StyleOf("a").z_index, 0);
    EXPECT_EQ(StyleOf("b").z_index, 2);
    EXPECT_EQ(StyleOf("d").z_index, 2);
  }

  TEST_F(UiStyleSheetTest, WorksWithoutAnySheet)
  {
    ASSERT_GE(ShowUnderRoot("- type: label\n  name: a\n  text: a\n  font_size: 20\n"), 0);

    EXPECT_FLOAT_EQ(StyleOf("a").font_size, 20.0f);
    EXPECT_TRUE(_ui->ReloadStyles());
  }
} // namespace
