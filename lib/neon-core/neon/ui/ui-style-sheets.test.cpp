#include "ui-style-sheets.hpp"

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::CssEnvironment;
  using neon::UiStyleSheets;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  // where what is named next to something else is

  TEST(UiPathTest, LooksNextToWhatNamesAPath)
  {
    EXPECT_EQ(neon::ResolveUiPath("assets://ui/menu.ui.yml", "theme.css"), "assets://ui/theme.css");
    EXPECT_EQ(neon::ResolveUiPath("assets://ui/menu.ui.yml", "themes/dark.css"), "assets://ui/themes/dark.css");
    EXPECT_EQ(neon::ResolveUiPath("assets://menu.ui.yml", "theme.css"), "assets://theme.css");
    EXPECT_EQ(neon::ResolveUiPath("assets://a/b/c/d.css", "e.css"), "assets://a/b/c/e.css");
  }

  TEST(UiPathTest, TakesAPathWithASchemeAsItIs)
  {
    EXPECT_EQ(neon::ResolveUiPath("assets://ui/menu.ui.yml", "user://themes/own.css"), "user://themes/own.css");
    EXPECT_EQ(neon::ResolveUiPath("", "assets://ui/theme.css"), "assets://ui/theme.css");
  }

  TEST(UiPathTest, GoesUpAFolderAndNotAboveTheScheme)
  {
    EXPECT_EQ(neon::ResolveUiPath("assets://ui/menus/main.ui.yml", "../theme.css"), "assets://ui/theme.css");
    EXPECT_EQ(neon::ResolveUiPath("assets://ui/menus/main.ui.yml", "../../fonts/a.ttf"), "assets://fonts/a.ttf");
    EXPECT_EQ(neon::ResolveUiPath("assets://ui/main.ui.yml", "../../../a.css"), "assets://a.css");
    EXPECT_EQ(neon::ResolveUiPath("assets://ui/main.ui.yml", "./theme.css"), "assets://ui/theme.css");
    EXPECT_EQ(neon::ResolveUiPath("assets://ui/main.ui.yml", "a/./b/../c.css"), "assets://ui/a/c.css");
  }

  TEST(UiPathTest, StartsAtTheTopOfTheSchemeForAPathWithASlashInFront)
  {
    EXPECT_EQ(neon::ResolveUiPath("assets://ui/menus/main.ui.yml", "/fonts/a.ttf"), "assets://fonts/a.ttf");
  }

  class UiStyleSheetsTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _file_system{SettingsConfig{}, _logger};
    UiStyleSheets _sheets;
    std::vector<std::string> _errors;
    std::vector<std::string> _warnings;

    void SetUp() override
    {
      _file_system.Initialize();
    }

    void Write(const std::string &name, const std::string &text)
    {
      _file_system.AddNativeFile("/assets/" + name, text);
    }

    bool Load(const std::vector<std::string> &paths)
    {
      _errors.clear();
      _warnings.clear();
      return _sheets.Load(paths, "assets://ui/menu.ui.yml", _file_system, _errors, _warnings);
    }

    [[nodiscard]] std::vector<std::string> Selectors() const
    {
      std::vector<std::string> selectors;
      for (const auto &rule : _sheets.GetRules()) { selectors.push_back(rule.selector.text); }
      return selectors;
    }
  };

  TEST_F(UiStyleSheetsTest, HoldsNothingBeforeAnythingIsRead)
  {
    EXPECT_TRUE(_sheets.IsEmpty());
    EXPECT_TRUE(_sheets.GetRules().empty());
    EXPECT_EQ(_sheets.FindKeyframes("fade"), nullptr);
    EXPECT_TRUE(_sheets.Holds({}));
  }

  TEST_F(UiStyleSheetsTest, PutsEverySelectorOfARuleInTheOrderOfTheSheets)
  {
    Write("ui/a.css", "button, .row { opacity: 0.5 }\nlabel { opacity: 1 }");
    Write("ui/b.css", "#close { opacity: 0 }");

    ASSERT_TRUE(Load({"a.css", "b.css"}));

    EXPECT_THAT(Selectors(), ElementsAre("button", ".row", "label", "#close"));
    EXPECT_THAT(_sheets.GetPaths(), ElementsAre("assets://ui/a.css", "assets://ui/b.css"));

    for (std::size_t i = 0; i < _sheets.GetRules().size(); i++) { EXPECT_EQ(_sheets.GetRules()[i].order, i); }

    EXPECT_EQ(_sheets.GetRules()[0].sheet, "assets://ui/a.css");
    EXPECT_EQ(_sheets.GetRules()[0].where, "the rule 'button, .row'");
    EXPECT_EQ(_sheets.GetRules()[3].sheet, "assets://ui/b.css");

    // the selectors of a rule share its declarations
    EXPECT_EQ(_sheets.GetRules()[0].declarations, _sheets.GetRules()[1].declarations);
  }

  TEST_F(UiStyleSheetsTest, PutsWhatIsImportedInFrontOfTheSheetThatImportsIt)
  {
    Write("ui/base.css", "@import \"reset.css\";\n.base { opacity: 1 }");
    Write("ui/reset.css", ".reset { opacity: 1 }");
    Write("ui/theme.css", "@import \"base.css\";\n@import \"extra.css\";\n.theme { opacity: 1 }");
    Write("ui/extra.css", ".extra { opacity: 1 }");

    ASSERT_TRUE(Load({"theme.css"}));
    EXPECT_THAT(Selectors(), ElementsAre(".reset", ".base", ".extra", ".theme"));
    EXPECT_THAT(_warnings, IsEmpty());
  }

  TEST_F(UiStyleSheetsTest, SaysWhichSheetCannotBeReadAndWhatNamesIt)
  {
    Write("ui/theme.css", "\n\n@import \"gone.css\";\n.theme { opacity: 1 }");

    EXPECT_FALSE(Load({"missing.css", "theme.css"}));
    EXPECT_THAT(
      _errors,
      ElementsAre(
        "assets://ui/menu.ui.yml: the style sheet assets://ui/missing.css cannot be read",
        "assets://ui/theme.css:3: the style sheet assets://ui/gone.css cannot be read"));

    // what could be read is there
    EXPECT_THAT(Selectors(), ElementsAre(".theme"));
  }

  TEST_F(UiStyleSheetsTest, LeavesOutWhatCannotBeReadAndSaysSo)
  {
    Write(
      "ui/theme.css",
      "a {\n"
      "  opacity: 0.5;\n"
      "  opacity: most;\n"
      "  colour: red;\n"
      "  --anything: goes here;\n"
      "  width: var(--whatever);\n"
      "}\n");

    ASSERT_TRUE(Load({"theme.css"}));

    ASSERT_EQ(_sheets.GetRules().size(), 1u);
    const auto &declarations = *_sheets.GetRules()[0].declarations;

    ASSERT_EQ(declarations.size(), 3u);
    EXPECT_EQ(declarations[0].value, "0.5");
    EXPECT_EQ(declarations[1].name, "--anything");
    EXPECT_EQ(declarations[2].name, "width");

    EXPECT_THAT(
      _warnings,
      ElementsAre(
        "assets://ui/theme.css:3: 'opacity' of the rule 'a' is 'most', where a number from 0 to 1 was "
        "expected. The declaration is left out",
        "assets://ui/theme.css:4: 'colour' of the rule 'a' is not a property that is known. Near to it are: "
        "color. The declaration is left out"));
  }

  TEST_F(UiStyleSheetsTest, WritesThePathsOfASheetAsVirtualPaths)
  {
    Write(
      "themes/dark/theme.css",
      "@font-face { font-family: Title; src: url(../fonts/title.ttf); }\n"
      "a { background-image: url(\"images/panel.png\"); border-image-source: url(assets://ui/frame.png); }");

    ASSERT_TRUE(Load({"assets://themes/dark/theme.css"}));

    ASSERT_EQ(_sheets.GetFonts().size(), 1u);
    EXPECT_EQ(_sheets.GetFonts()[0].family, "Title");
    EXPECT_EQ(_sheets.GetFonts()[0].path, "assets://themes/fonts/title.ttf");
    EXPECT_EQ(_sheets.GetFonts()[0].weight, 400);

    const auto &declarations = *_sheets.GetRules()[0].declarations;
    EXPECT_EQ(declarations[0].value, "url(\"assets://themes/dark/images/panel.png\")");
    EXPECT_EQ(declarations[1].value, "url(\"assets://ui/frame.png\")");
  }

  TEST_F(UiStyleSheetsTest, FindsKeyframesByTheirNameAndTakesTheLastOfTwo)
  {
    Write("ui/a.css", "@keyframes fade { from { opacity: 0 } to { opacity: 1 } }");
    Write("ui/b.css", "@keyframes fade { from { opacity: 0.5 } }\n@keyframes slide { to { left: 10px } }");

    ASSERT_TRUE(Load({"a.css", "b.css"}));

    const auto *fade = _sheets.FindKeyframes("fade");
    ASSERT_NE(fade, nullptr);
    EXPECT_EQ(fade->sheet, "assets://ui/b.css");
    ASSERT_EQ(fade->frames.size(), 1u);
    EXPECT_EQ(fade->frames[0].declarations[0].value, "0.5");

    ASSERT_NE(_sheets.FindKeyframes("slide"), nullptr);
    EXPECT_EQ(_sheets.FindKeyframes("Fade"), nullptr);
    EXPECT_EQ(_sheets.FindKeyframes("missing"), nullptr);
  }

  TEST_F(UiStyleSheetsTest, LeavesOutWhatAKeyframeCannotHold)
  {
    Write(
      "ui/a.css",
      "@keyframes fade {\n"
      "  from { opacity: 0; animation-name: other; colour: red; animation-timing-function: linear }\n"
      "}\n");

    ASSERT_TRUE(Load({"a.css"}));

    const auto *fade = _sheets.FindKeyframes("fade");
    ASSERT_NE(fade, nullptr);
    ASSERT_EQ(fade->frames[0].declarations.size(), 2u);
    EXPECT_EQ(fade->frames[0].declarations[0].name, "opacity");
    EXPECT_EQ(fade->frames[0].declarations[1].name, "animation-timing-function");

    EXPECT_THAT(
      _warnings,
      ElementsAre(
        "assets://ui/a.css:2: 'animation-name' of @keyframes fade cannot be animated. The declaration is "
        "left out",
        "assets://ui/a.css:2: 'colour' of @keyframes fade is not a property that is known. Near to it are: "
        "color. The declaration is left out"));
  }

  TEST_F(UiStyleSheetsTest, KnowsWhetherTheConditionsOfARuleHold)
  {
    Write(
      "ui/a.css",
      "a { opacity: 1 }\n"
      "@media (max-width: 800px) {\n"
      "  b { opacity: 1 }\n"
      "  @media (orientation: portrait) { c { opacity: 1 } }\n"
      "  @keyframes small { to { opacity: 1 } }\n"
      "}\n");

    ASSERT_TRUE(Load({"a.css"}));
    ASSERT_EQ(_sheets.GetRules().size(), 3u);

    // a window of 1920 by 1080 to start with
    EXPECT_TRUE(_sheets.Holds(_sheets.GetRules()[0].media));
    EXPECT_FALSE(_sheets.Holds(_sheets.GetRules()[1].media));
    EXPECT_FALSE(_sheets.Holds(_sheets.GetRules()[2].media));
    EXPECT_EQ(_sheets.FindKeyframes("small"), nullptr);

    EXPECT_TRUE(_sheets.SetEnvironment(CssEnvironment{800.0f, 600.0f}));
    EXPECT_TRUE(_sheets.Holds(_sheets.GetRules()[1].media));
    EXPECT_FALSE(_sheets.Holds(_sheets.GetRules()[2].media));
    EXPECT_NE(_sheets.FindKeyframes("small"), nullptr);

    EXPECT_TRUE(_sheets.SetEnvironment(CssEnvironment{600.0f, 800.0f}));
    EXPECT_TRUE(_sheets.Holds(_sheets.GetRules()[2].media));
  }

  TEST_F(UiStyleSheetsTest, SaysWhetherAChangeOfTheWindowChangedWhatHolds)
  {
    Write("ui/a.css", "@media (max-width: 800px) { b { opacity: 1 } }");
    ASSERT_TRUE(Load({"a.css"}));

    EXPECT_FALSE(_sheets.SetEnvironment(CssEnvironment{1280.0f, 720.0f}));
    EXPECT_TRUE(_sheets.SetEnvironment(CssEnvironment{640.0f, 480.0f}));
    EXPECT_FALSE(_sheets.SetEnvironment(CssEnvironment{320.0f, 240.0f}));
    EXPECT_TRUE(_sheets.SetEnvironment(CssEnvironment{1920.0f, 1080.0f}));
  }

  TEST_F(UiStyleSheetsTest, KeepsWhatItIsAskedAgainstWhenItIsReadAgain)
  {
    Write("ui/a.css", "@media (max-width: 800px) { b { opacity: 1 } }");

    _sheets.SetEnvironment(CssEnvironment{640.0f, 480.0f});
    ASSERT_TRUE(Load({"a.css"}));

    EXPECT_TRUE(_sheets.Holds(_sheets.GetRules()[0].media));
    EXPECT_EQ(_sheets.GetEnvironment().width, 640.0f);
  }

  TEST_F(UiStyleSheetsTest, KnowsWhatItsSelectorsDependOn)
  {
    Write("ui/a.css", ".row:hover > label { opacity: 1 }\ninput:checked + label { opacity: 1 }");
    ASSERT_TRUE(Load({"a.css"}));

    const auto &dependencies = _sheets.GetDependencies();
    EXPECT_TRUE(dependencies.HasStateAbove("hover"));
    EXPECT_TRUE(dependencies.HasStateBefore("checked"));
    EXPECT_TRUE(dependencies.classes_above);
    EXPECT_FALSE(dependencies.HasStateAbove("focus"));
  }

  TEST_F(UiStyleSheetsTest, TakesASheetFromText)
  {
    _sheets.AddText("a { opacity: 0.5; colour: red }\n@import \"b.css\";", "inline", _warnings);

    EXPECT_THAT(Selectors(), ElementsAre("a"));
    EXPECT_EQ(_warnings.size(), 2u);
  }
} // namespace
