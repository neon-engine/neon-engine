#include "recording-tool-ui.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
  using neon::ToolUiEvents;
  using neon::ToolUiItem;
  using neon::ToolUiItemKind;
  using neon::ToolUiPoint;
  using neon::testing::RecordingToolUi;
  using ::testing::SizeIs;

  TEST(RecordingToolUiTest, RecordsEveryItemWithItsLabelAndAPlaceOfItsOwn)
  {
    RecordingToolUi ui;
    ui.NextFrame();
    ui.BeginWindow("Log", true);
    ui.Text("a line");
    ui.Button("Close");
    bool checked = false;
    ui.Checkbox("Shadows", checked);
    std::string name;
    ui.InputText("Name", name);
    ui.EndWindow();

    ASSERT_THAT(ui.GetItemsDrawn(), SizeIs(5));
    EXPECT_EQ(ui.GetItemsDrawn()[0].kind, ToolUiItemKind::Window);
    EXPECT_EQ(ui.GetItemsDrawn()[0].label, "Log");
    EXPECT_FLOAT_EQ(ui.GetItemsDrawn()[0].rect.width, ui.available.width);

    const std::optional<ToolUiItem> text = ui.FindItem("a line", ToolUiItemKind::Text);
    ASSERT_TRUE(text.has_value());
    const std::optional<ToolUiItem> button = ui.FindItem("Close", ToolUiItemKind::Button);
    ASSERT_TRUE(button.has_value());
    EXPECT_GT(button->rect.y, text->rect.y);
    EXPECT_FALSE(button->rect.Contains(text->rect.Center()));
    EXPECT_TRUE(button->rect.Contains(button->rect.Center()));

    EXPECT_TRUE(ui.FindItem("Shadows").has_value());
    EXPECT_FALSE(ui.FindItem("Shadows", ToolUiItemKind::Button).has_value());
    EXPECT_TRUE(ui.FindItem("Name", ToolUiItemKind::InputText).has_value());
  }

  TEST(RecordingToolUiTest, AButtonIsPressedWhenTheButtonOfThePointerIsReleasedOverWhereItWas)
  {
    RecordingToolUi ui;
    ui.NextFrame();
    EXPECT_FALSE(ui.Button("Close"));
    const ToolUiPoint center = ui.FindItem("Close")->rect.Center();

    ToolUiEvents down;
    down.pointer = center;
    down.button_down = true;
    ui.FeedEvents(down);
    ui.NextFrame();
    EXPECT_FALSE(ui.Button("Close"));

    ToolUiEvents up;
    up.button_down = false;
    ui.FeedEvents(up);
    ui.NextFrame();
    EXPECT_TRUE(ui.Button("Close"));

    // released already: nothing more happens
    ui.NextFrame();
    EXPECT_FALSE(ui.Button("Close"));
    ASSERT_THAT(ui.events_fed, SizeIs(2));
  }

  TEST(RecordingToolUiTest, AReleaseElsewherePressesNothing)
  {
    RecordingToolUi ui;
    ui.NextFrame();
    ui.Button("Close");

    ToolUiEvents down;
    down.pointer = ToolUiPoint{500.0f, 300.0f};
    down.button_down = true;
    ui.FeedEvents(down);
    ui.NextFrame();
    ui.Button("Close");

    ToolUiEvents up;
    up.button_down = false;
    ui.FeedEvents(up);
    ui.NextFrame();
    EXPECT_FALSE(ui.Button("Close"));
  }

  TEST(RecordingToolUiTest, AFieldTakesTheTextOnceItWasClicked)
  {
    RecordingToolUi ui;
    std::string name;
    ui.NextFrame();
    ui.InputText("Name", name);
    const ToolUiPoint center = ui.FindItem("Name")->rect.Center();

    ToolUiEvents typed_before;
    typed_before.text = "no";
    ui.FeedEvents(typed_before);
    ui.NextFrame();
    EXPECT_FALSE(ui.InputText("Name", name));
    EXPECT_EQ(name, "");

    ToolUiEvents down;
    down.pointer = center;
    down.button_down = true;
    ui.FeedEvents(down);
    ui.NextFrame();
    ui.InputText("Name", name);
    ToolUiEvents up;
    up.button_down = false;
    ui.FeedEvents(up);
    ui.NextFrame();
    ui.InputText("Name", name);

    ToolUiEvents typed;
    typed.text = "my-game";
    ui.FeedEvents(typed);
    ui.NextFrame();
    EXPECT_TRUE(ui.InputText("Name", name));
    EXPECT_EQ(name, "my-game");
  }

  TEST(RecordingToolUiTest, MenusOpenWithAClickWhenTheyFollowClicks)
  {
    RecordingToolUi ui;
    ui.menus_follow_clicks = true;
    ui.NextFrame();
    EXPECT_FALSE(ui.BeginMenu("View"));
    const ToolUiPoint center = ui.FindItem("View", ToolUiItemKind::Menu)->rect.Center();

    ToolUiEvents down;
    down.pointer = center;
    down.button_down = true;
    ui.FeedEvents(down);
    ui.NextFrame();
    EXPECT_FALSE(ui.BeginMenu("View"));

    ToolUiEvents up;
    up.button_down = false;
    ui.FeedEvents(up);
    ui.NextFrame();
    ASSERT_TRUE(ui.BeginMenu("View"));
    ui.MenuItem("Log", "", false, true);
    ui.EndMenu();
    EXPECT_TRUE(ui.FindItem("Log", ToolUiItemKind::MenuItem).has_value());
  }
}
