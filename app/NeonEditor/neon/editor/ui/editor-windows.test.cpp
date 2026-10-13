#include "editor-windows.hpp"

#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/recording-tool-ui.hpp>

namespace
{
  using neon::EditorWindow;
  using neon::EditorWindows;
  using neon::ToolUiDockLayout;
  using neon::ToolUiDockSide;
  using neon::testing::RecordingToolUi;
  using ::testing::Contains;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;
  using ::testing::Not;

  /// A registry with two windows, each of which says in the calls that it
  /// was drawn.
  class EditorWindowsTest : public ::testing::Test
  {
  protected:
    RecordingToolUi _ui;
    EditorWindows _windows;

    void SetUp() override
    {
      ASSERT_TRUE(_windows.Add({
        .title = "Log",
        .shortcut = "Ctrl+L",
        .side = ToolUiDockSide::Bottom,
        .share = 0.25f,
        .draw = [](neon::ToolUiContext &ui) { ui.Text("the log"); }
      }));
      ASSERT_TRUE(_windows.Add({
        .title = "Inspector",
        .shortcut = "Ctrl+I",
        .side = ToolUiDockSide::Right,
        .share = 0.3f,
        .draw = [](neon::ToolUiContext &ui) { ui.Text("the inspector"); }
      }));
    }
  };

  TEST_F(EditorWindowsTest, EveryWindowIsHiddenUntilItIsShown)
  {
    EXPECT_FALSE(_windows.IsShown("Log"));
    EXPECT_FALSE(_windows.IsShown("Inspector"));
    EXPECT_THAT(_windows.GetShownTitles(), IsEmpty());

    _windows.DrawShown(_ui);
    EXPECT_THAT(_ui.calls, IsEmpty());
  }

  TEST_F(EditorWindowsTest, IsRegisteredHiddenWhateverWasHandedIn)
  {
    EditorWindows windows;
    ASSERT_TRUE(windows.Add({.title = "Shown", .shown = true}));
    EXPECT_FALSE(windows.IsShown("Shown"));
  }

  TEST_F(EditorWindowsTest, RefusesATitleThatIsTakenOrEmpty)
  {
    EXPECT_FALSE(_windows.Add({.title = "Log"}));
    EXPECT_FALSE(_windows.Add({.title = ""}));
    EXPECT_EQ(_windows.GetWindows().size(), 2u);
  }

  TEST_F(EditorWindowsTest, ShowsHidesAndTogglesAWindowByItsTitle)
  {
    EXPECT_TRUE(_windows.Show("Log", true));
    EXPECT_TRUE(_windows.IsShown("Log"));
    EXPECT_TRUE(_windows.Show("Log", false));
    EXPECT_FALSE(_windows.IsShown("Log"));

    EXPECT_TRUE(_windows.Toggle("Inspector"));
    EXPECT_TRUE(_windows.IsShown("Inspector"));
    EXPECT_TRUE(_windows.Toggle("Inspector"));
    EXPECT_FALSE(_windows.IsShown("Inspector"));

    EXPECT_FALSE(_windows.Show("Hierarchy", true));
    EXPECT_FALSE(_windows.Toggle("Hierarchy"));
    EXPECT_FALSE(_windows.Has("Hierarchy"));
  }

  TEST_F(EditorWindowsTest, DrawsOnlyTheWindowsThatAreShown)
  {
    _windows.Show("Inspector", true);

    _windows.DrawShown(_ui);

    EXPECT_THAT(_ui.calls, ElementsAre("text the inspector"));
    EXPECT_THAT(_windows.GetShownTitles(), ElementsAre("Inspector"));
  }

  TEST_F(EditorWindowsTest, ListsEveryWindowInTheViewMenuWithItsShortcutAndMarksTheShown)
  {
    _windows.Show("Log", true);

    _windows.DrawViewMenu(_ui);

    EXPECT_THAT(_ui.calls, ElementsAre(
                  "begin-menu View",
                  "menu-item Log Ctrl+L selected",
                  "menu-item Inspector Ctrl+I",
                  "end-menu"));
  }

  TEST_F(EditorWindowsTest, AnItemOfTheViewMenuShowsAndHidesItsWindow)
  {
    _ui.chosen_items.insert("Inspector");

    _windows.DrawViewMenu(_ui);
    EXPECT_TRUE(_windows.IsShown("Inspector"));
    EXPECT_FALSE(_windows.IsShown("Log"));

    _windows.DrawViewMenu(_ui);
    EXPECT_FALSE(_windows.IsShown("Inspector"));
  }

  TEST_F(EditorWindowsTest, AShortcutShowsAndHidesItsWindow)
  {
    _ui.pressed_shortcuts.insert("Ctrl+L");

    _windows.HandleShortcuts(_ui);
    EXPECT_TRUE(_windows.IsShown("Log"));
    EXPECT_FALSE(_windows.IsShown("Inspector"));

    _windows.HandleShortcuts(_ui);
    EXPECT_FALSE(_windows.IsShown("Log"));
  }

  TEST_F(EditorWindowsTest, AWindowWithoutAShortcutIsShownFromTheMenuAlone)
  {
    ASSERT_TRUE(_windows.Add({.title = "Build output"}));
    _ui.pressed_shortcuts.insert("");

    _windows.HandleShortcuts(_ui);
    EXPECT_FALSE(_windows.IsShown("Build output"));

    _windows.DrawViewMenu(_ui);
    EXPECT_THAT(_ui.calls, Contains("menu-item Build output"));
  }

  TEST_F(EditorWindowsTest, LaysTheWindowsOutAroundTheMiddle)
  {
    const ToolUiDockLayout layout = _windows.LayoutAround("Viewport");

    EXPECT_EQ(layout.center, "Viewport");
    ASSERT_EQ(layout.sides.size(), 2u);
    EXPECT_EQ(layout.sides[0].window, "Log");
    EXPECT_EQ(layout.sides[0].side, ToolUiDockSide::Bottom);
    EXPECT_FLOAT_EQ(layout.sides[0].share, 0.25f);
    EXPECT_EQ(layout.sides[1].window, "Inspector");
    EXPECT_EQ(layout.sides[1].side, ToolUiDockSide::Right);
    EXPECT_FLOAT_EQ(layout.sides[1].share, 0.3f);

    _ui.DockSpace(layout);
    EXPECT_THAT(_ui.calls, ElementsAre("dock-space Viewport Log:bottom Inspector:right"));
  }
}
