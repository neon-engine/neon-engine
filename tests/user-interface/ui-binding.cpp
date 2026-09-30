#include "ui-fixture.hpp"

#include <neon/ui/elements/ui-bar.hpp>
#include <neon/ui/elements/ui-button.hpp>
#include <neon/ui/elements/ui-label.hpp>

// The values of a game: it sets them by name, and what a file refers to
// them with follows, without the game knowing an element.

namespace
{
  using neon::testing::LogLevel;
  using neon::testing::UiTest;

  class UiBindingTest : public UiTest
  {
  protected:
    void ShowHud(const std::string &top = "")
    {
      ASSERT_GE(Show(
        top +
        "root:\n"
        "  type: panel\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  align_items: flex-start\n"
        "  children:\n"
        "    - type: label\n"
        "      name: health\n"
        "      text: \"Health: {health}\"\n"
        "    - type: bar\n"
        "      name: bar\n"
        "      value: \"{health}\"\n"
        "      max: 100\n"), 0) << _logger->Messages(LogLevel::Error);
    }

    [[nodiscard]] std::string TextOf(const std::string &name) const
    {
      if (const auto *label = dynamic_cast<const neon::UiLabel *>(&Element(name)); label != nullptr)
      {
        return label->GetText();
      }
      return dynamic_cast<const neon::UiButton &>(Element(name)).GetText();
    }

    [[nodiscard]] float FilledOf(const std::string &name) const
    {
      return dynamic_cast<const neon::UiBar &>(Element(name)).GetFilled();
    }
  };

  TEST_F(UiBindingTest, ShowsTheValueTheGameHasSet)
  {
    _ui->SetNumber("health", 75);
    ShowHud();
    Frame();

    EXPECT_EQ(TextOf("health"), "Health: 75");
    EXPECT_FLOAT_EQ(FilledOf("bar"), 0.75f);
  }

  TEST_F(UiBindingTest, FollowsAValueThatChanges)
  {
    _ui->SetNumber("health", 75);
    ShowHud();
    Frame();

    _ui->SetNumber("health", 40);
    Frame();

    EXPECT_EQ(TextOf("health"), "Health: 40");
    EXPECT_FLOAT_EQ(FilledOf("bar"), 0.4f);
  }

  TEST_F(UiBindingTest, ALabelGrowsWithItsText)
  {
    _ui->SetNumber("health", 5);
    ShowHud();
    Frame();

    // `Health: 5` is nine characters of 8
    ExpectBox("health", 0, 0, 72, 16);
    ExpectBox("bar", 72, 0, 160, 16);

    _ui->SetNumber("health", 100);
    Frame();

    ExpectBox("health", 0, 0, 88, 16);
    ExpectBox("bar", 88, 0, 160, 16);
  }

  TEST_F(UiBindingTest, DrawsWhatTheValueHoldsInTheFrameItWasSetIn)
  {
    _ui->SetNumber("health", 100);
    ShowHud();
    Frame();

    _ui->SetNumber("health", 25);
    Frame();

    bool found = false;
    for (const auto &quad : _renderer.Quads())
    {
      // the filling of the bar, which is green unless the file says otherwise
      if (!quad.textured && quad.color.g > 0.6f && quad.color.r < 0.4f)
      {
        EXPECT_FLOAT_EQ(quad.Width(), 40);
        found = true;
      }
    }
    EXPECT_TRUE(found);
  }

  TEST_F(UiBindingTest, StartsWithTheValuesOfTheFile)
  {
    ShowHud("values:\n  health: 60\n");
    Frame();

    EXPECT_EQ(TextOf("health"), "Health: 60");
    EXPECT_FLOAT_EQ(FilledOf("bar"), 0.6f);
  }

  TEST_F(UiBindingTest, AValueOfTheFileDoesNotReplaceWhatTheGameHasSet)
  {
    _ui->SetNumber("health", 20);
    ShowHud("values:\n  health: 60\n");
    Frame();

    EXPECT_EQ(TextOf("health"), "Health: 20");
  }

  TEST_F(UiBindingTest, ShowsANameWithoutAValueAsItIsWritten)
  {
    ShowHud();
    Frame();

    EXPECT_EQ(TextOf("health"), "Health: {health}");
    EXPECT_FLOAT_EQ(FilledOf("bar"), 0);
  }

  TEST_F(UiBindingTest, SaysOnceThatAValueIsNotSet)
  {
    ShowHud();
    Frame();
    Frame();
    Frame();

    EXPECT_EQ(_logger->Count(LogLevel::Warn), 1u) << _logger->Messages(LogLevel::Warn);
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "The value 'health' is not set"));
  }

  TEST_F(UiBindingTest, ShowsTextAndFlags)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: label\n"
      "  name: status\n"
      "  text: \"{player} is armed: {armed}\"\n"), 0);

    _ui->SetText("player", "Ada");
    _ui->SetFlag("armed", true);
    Frame();

    EXPECT_EQ(TextOf("status"), "Ada is armed: true");
  }

  TEST_F(UiBindingTest, ShowsTextThatIsNotAscii)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: label\n"
      "  name: greeting\n"
      "  text: \"Grüße, {player}\"\n"
      "  align_self: flex-start\n"), 0);

    _ui->SetText("player", "Zoë");
    Frame();

    EXPECT_EQ(TextOf("greeting"), "Grüße, Zoë");

    // ten characters, which are fourteen bytes
    ExpectBox("greeting", 0, 0, 80, 16);
  }

  TEST_F(UiBindingTest, WritesANumberWithTheDigitsTheFileAsksFor)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: label\n"
      "  name: speed\n"
      "  text: \"{speed} / {speed:0} / {speed:1}\"\n"), 0);

    _ui->SetNumber("speed", 12.3456);
    Frame();

    EXPECT_EQ(TextOf("speed"), "12.35 / 12 / 12.3");
  }

  TEST_F(UiBindingTest, ABarIsNeverMoreThanFullNorLessThanEmpty)
  {
    ShowHud();

    _ui->SetNumber("health", 250);
    Frame();
    EXPECT_FLOAT_EQ(FilledOf("bar"), 1);

    _ui->SetNumber("health", -10);
    Frame();
    EXPECT_FLOAT_EQ(FilledOf("bar"), 0);
  }

  TEST_F(UiBindingTest, TheMostOfABarFollowsAValueAsWell)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: bar\n"
      "  name: bar\n"
      "  value: \"{ammo}\"\n"
      "  max: \"{magazine}\"\n"), 0);

    _ui->SetNumber("ammo", 6);
    _ui->SetNumber("magazine", 24);
    Frame();
    EXPECT_FLOAT_EQ(FilledOf("bar"), 0.25f);

    _ui->SetNumber("magazine", 12);
    Frame();
    EXPECT_FLOAT_EQ(FilledOf("bar"), 0.5f);

    // a bar of nothing is empty
    _ui->SetNumber("magazine", 0);
    Frame();
    EXPECT_FLOAT_EQ(FilledOf("bar"), 0);
  }

  TEST_F(UiBindingTest, AnElementIsHiddenByAValue)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: menu\n"
      "  width: 100\n"
      "  hidden: \"{!paused}\"\n"
      "- type: panel\n"
      "  name: crosshair\n"
      "  width: 100\n"
      "  hidden: \"{paused}\"\n"), 0) << _logger->Messages(LogLevel::Error);

    _ui->SetFlag("paused", false);
    Frame();

    EXPECT_TRUE(Element("menu").IsHidden());
    EXPECT_FALSE(Element("crosshair").IsHidden());
    ExpectBox("crosshair", 0, 0, 100, 1080);

    _ui->SetFlag("paused", true);
    Frame();

    EXPECT_FALSE(Element("menu").IsHidden());
    EXPECT_TRUE(Element("crosshair").IsHidden());
    ExpectBox("menu", 0, 0, 100, 1080);
  }

  TEST_F(UiBindingTest, AnElementIsShownUntilTheValueThatHidesItIsSet)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n  name: a\n  hidden: \"{paused}\"\n"
      "- type: panel\n  name: b\n  hidden: \"{!paused}\"\n"), 0);

    Frame();

    EXPECT_FALSE(Element("a").IsHidden());
    EXPECT_FALSE(Element("b").IsHidden());
  }

  TEST_F(UiBindingTest, AButtonIsEnabledByAValue)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: button\n"
      "  name: continue\n"
      "  text: Continue\n"
      "  enabled: \"{has_save}\"\n"), 0) << _logger->Messages(LogLevel::Error);

    _ui->SetFlag("has_save", false);
    Frame();

    EXPECT_FALSE(Element("continue").IsEnabled());
    EXPECT_TRUE(Element("continue").GetStates().disabled);
    EXPECT_FLOAT_EQ(Element("continue").GetStyle().opacity, 0.5f);

    _ui->SetFlag("has_save", true);
    Frame();

    EXPECT_TRUE(Element("continue").IsEnabled());
    EXPECT_FALSE(Element("continue").GetStates().disabled);
    EXPECT_FLOAT_EQ(Element("continue").GetStyle().opacity, 1);
  }

  TEST_F(UiBindingTest, TheTextOfAButtonFollowsAValue)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: button\n"
      "  name: buy\n"
      "  text: \"Buy for {price}\"\n"), 0);

    _ui->SetNumber("price", 250);
    Frame();

    EXPECT_EQ(TextOf("buy"), "Buy for 250");
  }

  TEST_F(UiBindingTest, EveryFileSeesTheSameValues)
  {
    _ui->SetNumber("health", 75);

    ASSERT_GE(Show(
      "root:\n  type: label\n  name: first\n  text: \"{health}\"\n", "ui/first.ui.yml"), 0);
    ASSERT_GE(Show(
      "root:\n  type: label\n  name: second\n  text: \"{health} of 100\"\n", "ui/second.ui.yml"), 0);

    Frame();

    EXPECT_EQ(TextOf("first"), "75");
    EXPECT_EQ(TextOf("second"), "75 of 100");
  }

  TEST_F(UiBindingTest, AValueStaysWhenTheFileThatShowedItIsGone)
  {
    const int first = Show("values:\n  health: 60\nroot:\n  type: label\n  text: \"{health}\"\n", "ui/a.ui.yml");
    ASSERT_GE(first, 0);
    _ui->Unload(first);

    ASSERT_GE(Show("root:\n  type: label\n  name: again\n  text: \"{health}\"\n", "ui/b.ui.yml"), 0);
    Frame();

    EXPECT_EQ(TextOf("again"), "60");
  }

  TEST_F(UiBindingTest, PlainTextIsLeftAlone)
  {
    ASSERT_GE(ShowUnderRoot("- type: label\n  name: title\n  text: \"Paused {{not a value}}\"\n"), 0);

    _ui->SetNumber("health", 1);
    Frame();
    _ui->SetNumber("health", 2);
    Frame();

    EXPECT_EQ(TextOf("title"), "Paused {not a value}");
  }
}
