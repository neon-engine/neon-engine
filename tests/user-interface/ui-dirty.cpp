#include "ui-fixture.hpp"

// What is worked out when: a frame in which nothing changed does no work,
// and a change works out again what depends on it and nothing else.

namespace
{
  using neon::UiStatistics;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;

  class UiDirtyTest : public UiTest
  {
  protected:
    UiStatistics _before;

    /// Two panels of a size of their own next to each other, each with a
    /// label and a button, and a label beside them whose size follows
    /// from its text.
    void ShowTwoPanels()
    {
      ASSERT_GE(Show(
        "ui: test\n"
        "values:\n"
        "  health: 75\n"
        "  score: 10\n"
        "  name: Ada\n"
        "root:\n"
        "  type: panel\n"
        "  name: root\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  children:\n"
        "    - type: panel\n"
        "      name: left\n"
        "      width: 400\n"
        "      height: 300\n"
        "      flex_direction: column\n"
        "      children:\n"
        "        - type: label\n"
        "          name: health\n"
        "          text: \"Health: {health}\"\n"
        "        - type: button\n"
        "          name: start\n"
        "          text: Start\n"
        "          hover:\n"
        "            background_color: \"#ff0000\"\n"
        "    - type: panel\n"
        "      name: right\n"
        "      width: 400\n"
        "      height: 300\n"
        "      flex_direction: column\n"
        "      align_items: flex-start\n"
        "      children:\n"
        "        - type: label\n"
        "          name: score\n"
        "          text: \"Score: {score}\"\n"
        "        - type: bar\n"
        "          name: bar\n"
        "          value: \"{health}\"\n"
        "          max: 100\n"
        "    - type: label\n"
        "      name: free\n"
        "      text: \"{name}\"\n"), 0) << _logger->Messages(LogLevel::Error);

      Frame();
      Frame();
      Remember();
    }

    void Remember()
    {
      _before = _ui->GetStatistics();
    }

    /// What was worked out since Remember().
    [[nodiscard]] UiStatistics Since() const
    {
      const UiStatistics &now = _ui->GetStatistics();

      UiStatistics since;
      since.follows = now.follows - _before.follows;
      since.styles = now.styles - _before.styles;
      since.layouts = now.layouts - _before.layouts;
      since.full_layouts = now.full_layouts - _before.full_layouts;
      since.layouts_spared = now.layouts_spared - _before.layouts_spared;
      since.laid_out_elements = now.laid_out_elements - _before.laid_out_elements;
      since.places = now.places - _before.places;
      since.paints = now.paints - _before.paints;
      since.replays = now.replays - _before.replays;
      return since;
    }
  };

  TEST_F(UiDirtyTest, WorksEverythingOutOnceWhenAFileIsShown)
  {
    ASSERT_GE(ShowUnderRoot("- type: label\n  name: a\n  text: a\n- type: button\n  name: b\n  text: b\n"), 0);
    Frame();

    const UiStatistics &statistics = _ui->GetStatistics();
    EXPECT_EQ(statistics.follows, 1u);
    EXPECT_EQ(statistics.styles, 3u);
    EXPECT_EQ(statistics.layouts, 1u);
    EXPECT_EQ(statistics.full_layouts, 1u);
    EXPECT_EQ(statistics.laid_out_elements, 3u);
    EXPECT_EQ(statistics.paints, 1u);
    EXPECT_EQ(statistics.replays, 0u);
  }

  TEST_F(UiDirtyTest, DoesNoWorkInAFrameInWhichNothingChanged)
  {
    ShowTwoPanels();

    for (int i = 0; i < 10; i++) { Frame(); }

    const UiStatistics since = Since();
    EXPECT_EQ(since.follows, 0u);
    EXPECT_EQ(since.styles, 0u);
    EXPECT_EQ(since.layouts, 0u);
    EXPECT_EQ(since.places, 0u);
    EXPECT_EQ(since.paints, 0u);
    EXPECT_EQ(since.replays, 10u);
  }

  TEST_F(UiDirtyTest, DrawsTheSameFromWhatItKeptAsFromTheElements)
  {
    ShowTwoPanels();

    // drawn from the elements
    _ui->SetNumber("health", 50);
    Frame();
    ASSERT_EQ(Since().paints, 1u);
    const auto painted = _renderer.Quads();
    const auto painted_calls = _ui->GetDrawCalls();

    // and from what was kept
    Frame();
    ASSERT_EQ(Since().replays, 1u);
    const auto replayed = _renderer.Quads();

    ASSERT_EQ(replayed.size(), painted.size());
    ASSERT_FALSE(painted.empty());
    EXPECT_EQ(_ui->GetDrawCalls(), painted_calls);

    for (std::size_t i = 0; i < painted.size(); i++)
    {
      EXPECT_EQ(replayed[i].left, painted[i].left) << i;
      EXPECT_EQ(replayed[i].top, painted[i].top) << i;
      EXPECT_EQ(replayed[i].right, painted[i].right) << i;
      EXPECT_EQ(replayed[i].bottom, painted[i].bottom) << i;
      EXPECT_EQ(replayed[i].color.r, painted[i].color.r) << i;
      EXPECT_EQ(replayed[i].texture, painted[i].texture) << i;
    }
  }

  TEST_F(UiDirtyTest, DoesNoWorkWhenAValueIsSetToWhatItHolds)
  {
    ShowTwoPanels();

    _ui->SetNumber("health", 75);
    _ui->SetText("name", "Ada");
    Frame();

    const UiStatistics since = Since();
    EXPECT_EQ(since.follows, 0u);
    EXPECT_EQ(since.layouts, 0u);
    EXPECT_EQ(since.paints, 0u);
  }

  TEST_F(UiDirtyTest, PlacesOnlyThePanelATextChangedIn)
  {
    ShowTwoPanels();

    _ui->SetNumber("score", 12345);
    Frame();

    const UiStatistics since = Since();
    EXPECT_EQ(since.follows, 1u);
    EXPECT_EQ(since.layouts, 1u);
    EXPECT_EQ(since.full_layouts, 0u);

    // the panel on the right with its label and its bar, of 8 elements
    EXPECT_EQ(since.laid_out_elements, 3u);
    EXPECT_EQ(since.paints, 1u);

    EXPECT_NEAR(Element("score").GetBox().Width(), 12 * 8.0f, 0.01f);
  }

  TEST_F(UiDirtyTest, PlacesEverythingWhenWhatChangedIsInsideNothingOfASizeOfItsOwn)
  {
    ShowTwoPanels();

    _ui->SetText("name", "Ada Lovelace");
    Frame();

    const UiStatistics since = Since();
    EXPECT_EQ(since.layouts, 1u);
    EXPECT_EQ(since.full_layouts, 1u);
    EXPECT_EQ(since.laid_out_elements, 8u);
  }

  TEST_F(UiDirtyTest, PlacesNothingWhenAValueChangesThatOnlyChangesWhatIsDrawn)
  {
    ASSERT_GE(Show(
      "values:\n  health: 75\n"
      "root:\n"
      "  type: panel\n"
      "  children:\n"
      "    - type: bar\n"
      "      name: bar\n"
      "      value: \"{health}\"\n"
      "      max: 100\n"), 0);
    Frame();
    Frame();
    Remember();

    _ui->SetNumber("health", 50);
    Frame();

    const UiStatistics since = Since();
    EXPECT_EQ(since.follows, 1u);
    EXPECT_EQ(since.layouts, 0u);
    EXPECT_EQ(since.styles, 0u);
    EXPECT_EQ(since.paints, 1u);
  }

  TEST_F(UiDirtyTest, PlacesNothingWhenTheTextMeasuresAsItDidAndOnlyDrawsAgain)
  {
    ShowTwoPanels();

    // the label is measured again with what the engine last gave it, and
    // comes to the same size, so that nothing has to move
    _ui->SetText("name", "Bob");
    Frame();

    EXPECT_EQ(Since().layouts, 0u);
    EXPECT_EQ(Since().layouts_spared, 1u);
    EXPECT_EQ(Since().paints, 1u);
  }

  TEST_F(UiDirtyTest, PlacesAgainWhenTheTextMeasuresOtherwise)
  {
    ShowTwoPanels();

    _ui->SetText("name", "Bartholomew");
    Frame();

    EXPECT_EQ(Since().layouts, 1u);
    EXPECT_EQ(Since().layouts_spared, 0u);
    EXPECT_EQ(Since().paints, 1u);
  }

  TEST_F(UiDirtyTest, WorksOutOneStyleAndPlacesNothingWhenTheColorOfAButtonChangesUnderThePointer)
  {
    ShowTwoPanels();

    const auto &box = Element("start").GetBox();
    PointAt(box.left + 5.0, box.top + 5.0);
    Frame();

    const UiStatistics since = Since();

    // the button, and the two panels it is inside of, which are under the
    // pointer with it
    EXPECT_EQ(since.styles, 3u);
    EXPECT_EQ(since.layouts, 0u);
    EXPECT_EQ(since.follows, 0u);
    EXPECT_EQ(since.paints, 1u);

    Remember();
    Frame();
    EXPECT_EQ(Since().styles, 0u);
    EXPECT_EQ(Since().paints, 0u);
    EXPECT_EQ(Since().replays, 1u);
  }

  TEST_F(UiDirtyTest, DoesNoWorkWhileThePointerMovesOverTheSameElement)
  {
    ShowTwoPanels();

    const auto &box = Element("start").GetBox();
    PointAt(box.left + 5.0, box.top + 5.0);
    Frame();
    Remember();

    for (int i = 0; i < 5; i++)
    {
      PointAt(box.left + 6.0 + i, box.top + 5.0);
      Frame();
    }

    const UiStatistics since = Since();
    EXPECT_EQ(since.styles, 0u);
    EXPECT_EQ(since.layouts, 0u);
    EXPECT_EQ(since.paints, 0u);
    EXPECT_EQ(since.replays, 5u);
  }

  TEST_F(UiDirtyTest, PlacesThePanelAStateChangedTheSizeOfSomethingIn)
  {
    ASSERT_GE(Show(
      "root:\n"
      "  type: panel\n"
      "  width: 100%\n"
      "  height: 100%\n"
      "  children:\n"
      "    - type: panel\n"
      "      name: box\n"
      "      width: 400\n"
      "      height: 300\n"
      "      children:\n"
      "        - type: button\n"
      "          name: grow\n"
      "          text: a\n"
      "          width: 100\n"
      "          height: 50\n"
      "          box_sizing: border-box\n"
      "          hover:\n"
      "            width: 200\n"
      "    - type: panel\n"
      "      name: other\n"
      "      width: 50\n"
      "      height: 50\n"), 0);
    Frame();
    Frame();
    Remember();

    PointAt(10, 10);
    Frame();

    ExpectBox("grow", 0.0f, 0.0f, 200.0f, 50.0f);

    const UiStatistics since = Since();
    EXPECT_EQ(since.layouts, 1u);
    EXPECT_EQ(since.full_layouts, 0u);
    EXPECT_EQ(since.laid_out_elements, 2u);
  }

  TEST_F(UiDirtyTest, PlacesEverythingWhenTheSizeOfWhatIsShownChanges)
  {
    ShowTwoPanels();

    _renderer.SetResolution(1280, 720);
    Frame();

    const UiStatistics since = Since();
    EXPECT_EQ(since.full_layouts, 1u);
    EXPECT_EQ(since.styles, 8u);
    EXPECT_EQ(since.paints, 1u);

    Remember();
    Frame();
    EXPECT_EQ(Since().layouts, 0u);
    EXPECT_EQ(Since().replays, 1u);
  }

  TEST_F(UiDirtyTest, PlacesWhatIsHiddenAndShownAgain)
  {
    ASSERT_GE(Show(
      "values:\n  paused: false\n"
      "root:\n"
      "  type: panel\n"
      "  width: 100%\n"
      "  height: 100%\n"
      "  children:\n"
      "    - type: label\n"
      "      name: a\n"
      "      text: Paused\n"
      "      hidden: \"{!paused}\"\n"
      "    - type: label\n"
      "      name: b\n"
      "      text: b\n"), 0);
    Frame();
    Frame();
    Remember();

    EXPECT_TRUE(Element("a").IsHidden());
    EXPECT_NEAR(Element("b").GetBox().left, 0.0f, 0.01f);

    _ui->SetFlag("paused", true);
    Frame();

    EXPECT_FALSE(Element("a").IsHidden());
    EXPECT_NEAR(Element("b").GetBox().left, 48.0f, 0.01f);
    EXPECT_EQ(Since().layouts, 1u);

    Remember();
    _ui->SetFlag("paused", false);
    Frame();

    EXPECT_TRUE(Element("a").IsHidden());
    EXPECT_NEAR(Element("b").GetBox().left, 0.0f, 0.01f);
    EXPECT_EQ(Since().layouts, 1u);
  }

  TEST_F(UiDirtyTest, DrawsAgainWhenAFileIsShownAndWhenItIsTakenAway)
  {
    ShowTwoPanels();

    const int second = Show("root:\n  type: label\n  text: on top\n", "ui/second.ui.yml");
    ASSERT_GE(second, 0);
    Frame();
    EXPECT_EQ(Since().paints, 1u);

    Remember();
    Frame();
    EXPECT_EQ(Since().paints, 0u);

    const auto with_second = _renderer.Quads().size();

    _ui->Unload(second);
    Frame();
    EXPECT_EQ(Since().paints, 1u);
    EXPECT_LT(_renderer.Quads().size(), with_second);
  }

  TEST_F(UiDirtyTest, DrawsNothingWhenNoFileIsShown)
  {
    Frame();
    Frame();

    EXPECT_EQ(_ui->GetStatistics().paints, 0u);
    EXPECT_EQ(_ui->GetStatistics().replays, 0u);
    EXPECT_TRUE(_renderer.batches.empty());
  }

  TEST_F(UiDirtyTest, GivesUpTheFontsOfASizeItNoLongerDrawsAt)
  {
    ShowTwoPanels();

    const std::size_t fonts = _ui->GetFontCount();
    const std::size_t textures = _renderer.TextureCount();
    ASSERT_GT(fonts, 0u);

    // every size of the window draws text at another size, and none of
    // them leaves its fonts behind
    for (const auto &[width, height] : {
           std::pair{1280, 720}, std::pair{2560, 1440}, std::pair{3840, 2160}, std::pair{1600, 900},
           std::pair{1920, 1080}
         })
    {
      _renderer.SetResolution(width, height);
      Frame();

      EXPECT_EQ(_ui->GetFontCount(), fonts) << width;
      EXPECT_EQ(_renderer.TextureCount(), textures) << width;
    }
  }
} // namespace
