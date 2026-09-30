#include "ui-fixture.hpp"

// Moving through a user interface: what cancel does, files on top of each
// other and where the focus returns to, the order of the tab key, moving
// by direction to what lies that way, and the events a game makes sounds
// from.

namespace
{
  using neon::Action;
  using neon::Key;
  using neon::UiElementEvent;
  using neon::UiHandle;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;

  class UiNavigationTest : public UiTest
  {
  protected:
    /// A menu of three buttons in a column, at the left top.
    [[nodiscard]] static std::string Menu(const std::string &name, const std::string &top, const std::string &buttons)
    {
      return
        top +
        "ui: " + name + "\n"
        "root:\n"
        "  type: panel\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  flex_direction: column\n"
        "  align_items: flex-start\n"
        "  children:\n" +
        UiTest::Indented(buttons, "    ");
    }

    int ShowMenu(const std::string &name, const std::string &top, const std::string &buttons)
    {
      const int document = Show(Menu(name, top, buttons), "ui/" + name + ".ui.yml");
      Frame();
      return document;
    }

    void PressKey(const Key key, const bool shift = false)
    {
      Release();
      _input.state.AddKeyEvent({key, true, false, {.shift = shift}});
      _input.state.AddKeyEvent({key, false, false, {.shift = shift}});
      Frame();
      Release();
    }

    [[nodiscard]] std::vector<std::string> Happened() const
    {
      std::vector<std::string> happened;
      for (const auto &event : _ui->GetElementEvents())
      {
        happened.push_back(event.name + " " + event.target_name);
      }
      return happened;
    }
  };

  // cancel

  TEST_F(UiNavigationTest, CancelTakesTheFocusAwayFromAFileThatIsNotModal)
  {
    ShowMenu("menu", "", "- {type: button, name: a, text: A, autofocus: true}\n- {type: button, name: b, text: B}\n");
    EXPECT_EQ(_ui->GetFocused(), "a");

    Press(Action::Ui_Cancel);
    EXPECT_EQ(_ui->GetFocused(), "");
    EXPECT_EQ(_ui->GetDocuments().size(), 1u) << "the file stays";
  }

  TEST_F(UiNavigationTest, CancelClosesAFileThatIsModal)
  {
    ShowMenu("menu", "modal: true\n", "- {type: button, name: a, text: A}\n");
    EXPECT_EQ(_ui->GetFocused(), "a");

    Press(Action::Ui_Cancel);
    EXPECT_EQ(_ui->GetDocuments().size(), 0u);
  }

  TEST_F(UiNavigationTest, AFileSaysWhatCancelDoes)
  {
    ShowMenu("stays", "cancel: none\n", "- {type: button, name: a, text: A, autofocus: true}\n");
    Press(Action::Ui_Cancel);
    EXPECT_EQ(_ui->GetFocused(), "a") << "nothing happens, but the game hears of it";
    EXPECT_EQ(_ui->GetDocuments().size(), 1u);

    _ui->Unload(_ui->GetDocuments().front()->id);
    Frame();

    ShowMenu("closes", "cancel: close\n", "- {type: button, name: b, text: B}\n");
    Press(Action::Ui_Cancel);
    EXPECT_EQ(_ui->GetDocuments().size(), 0u);

    ShowMenu("blurs", "modal: true\ncancel: blur\n", "- {type: button, name: c, text: C}\n");
    EXPECT_EQ(_ui->GetFocused(), "c");
    Press(Action::Ui_Cancel);
    EXPECT_EQ(_ui->GetFocused(), "");
    EXPECT_EQ(_ui->GetDocuments().size(), 1u);
  }

  TEST_F(UiNavigationTest, TheGameHearsOfCancelInEveryCase)
  {
    ShowMenu("menu", "cancel: none\n", "- {type: button, name: a, text: A, autofocus: true}\n");

    std::vector<std::string> heard;
    _ui->OnAny("cancel", [&heard](const UiElementEvent &event) { heard.push_back(event.target_name); });

    Release();
    _input.state.SetAction(Action::Ui_Cancel);
    Frame();
    EXPECT_THAT(heard, ElementsAre("a")) << "on what has the focus";

    Release();
    Frame();
    _ui->Blur();
    Frame();

    heard.clear();
    Release();
    _input.state.SetAction(Action::Ui_Cancel);
    Frame();
    EXPECT_THAT(heard, ElementsAre("")) << "on the root of the file on top, which has no name";
  }

  TEST_F(UiNavigationTest, TheFocusReturnsToWhereItWasWhenAFileOnTopIsClosed)
  {
    ShowMenu("game", "", "- {type: button, name: inventory, text: Inventory}\n- {type: button, name: map, text: Map}\n");
    ASSERT_TRUE(_ui->Focus("map"));
    Frame();

    const int dialog = ShowMenu(
      "dialog", "modal: true\n", "- {type: button, name: yes, text: Yes}\n- {type: button, name: no, text: No}\n");
    EXPECT_EQ(_ui->GetFocused(), "yes") << "a modal file takes the focus";

    Press(Action::Ui_Down);
    EXPECT_EQ(_ui->GetFocused(), "no");

    _ui->Unload(dialog);
    Frame();
    EXPECT_EQ(_ui->GetFocused(), "map") << "back to where it was";

    // and the same when cancel closes it
    ShowMenu("dialog", "modal: true\n", "- {type: button, name: yes, text: Yes}\n");
    EXPECT_EQ(_ui->GetFocused(), "yes");
    Press(Action::Ui_Cancel);
    EXPECT_EQ(_ui->GetFocused(), "map");
  }

  TEST_F(UiNavigationTest, TheFocusDoesNotReturnToWhatIsGoneOrCannotBeUsed)
  {
    ShowMenu("game", "", "- {type: button, name: map, text: Map, enabled: \"{can_map}\"}\n");
    _ui->SetFlag("can_map", true);
    Frame();
    ASSERT_TRUE(_ui->Focus("map"));
    Frame();

    const int dialog = ShowMenu("dialog", "modal: true\n", "- {type: button, name: yes, text: Yes}\n");
    _ui->SetFlag("can_map", false);
    Frame();

    _ui->Unload(dialog);
    Frame();
    EXPECT_EQ(_ui->GetFocused(), "");
  }

  // the tab key

  TEST_F(UiNavigationTest, TabMovesTheFocusInTheOrderOfTheFileAndAroundAgain)
  {
    ShowMenu(
      "menu", "",
      "- {type: button, name: a, text: A}\n"
      "- {type: input, name: b}\n"
      "- {type: checkbox, name: c, text: C}\n"
      "- {type: label, name: d, text: D}\n");

    PressKey(Key::Tab);
    EXPECT_EQ(_ui->GetFocused(), "a");

    PressKey(Key::Tab);
    EXPECT_EQ(_ui->GetFocused(), "b");

    PressKey(Key::Tab);
    EXPECT_EQ(_ui->GetFocused(), "c");

    PressKey(Key::Tab);
    EXPECT_EQ(_ui->GetFocused(), "a") << "a label cannot have the focus";

    PressKey(Key::Tab, true);
    EXPECT_EQ(_ui->GetFocused(), "c") << "shift goes back";
  }

  TEST_F(UiNavigationTest, TabIndexPutsElementsFirstAndLeavesSomeOut)
  {
    ShowMenu(
      "menu", "",
      "- {type: button, name: a, text: A}\n"
      "- {type: button, name: b, text: B, tab_index: 2}\n"
      "- {type: button, name: c, text: C, tab_index: 1}\n"
      "- {type: button, name: d, text: D, tab_index: -1}\n"
      "- {type: button, name: e, text: E}\n");

    // as HTML: the lowest above 0 first, then those of 0 as the file has
    // them, and -1 never
    std::vector<std::string> order;
    for (int i = 0; i < 5; i++)
    {
      PressKey(Key::Tab);
      order.push_back(_ui->GetFocused());
    }
    EXPECT_THAT(order, ElementsAre("c", "b", "a", "e", "c"));

    // what is left out can still be clicked
    ClickAt(Element("d").GetBox().left + 5.0f, Element("d").GetBox().top + 5.0f);
    EXPECT_EQ(_ui->GetFocused(), "d");
  }

  TEST_F(UiNavigationTest, AScriptSetsTheTabIndex)
  {
    ShowMenu("menu", "", "- {type: button, name: a, text: A}\n- {type: button, name: b, text: B}\n");

    EXPECT_TRUE(_ui->SetField(_ui->FindByName("b"), "tab_index", 1));
    PressKey(Key::Tab);
    EXPECT_EQ(_ui->GetFocused(), "b");

    neon::FieldValue value;
    ASSERT_TRUE(_ui->GetField(_ui->FindByName("b"), "tab_index", value));
    EXPECT_EQ(std::get<int>(value), 1);
  }

  // by direction

  TEST_F(UiNavigationTest, ADirectionGoesToTheNearestElementThatWay)
  {
    // three buttons that are not in a line: right from the first goes to
    // the one that lies most to the right and least off to the side
    ASSERT_GE(Show(
      "ui: scattered\n"
      "root:\n"
      "  type: panel\n"
      "  width: 100%\n"
      "  height: 100%\n"
      "  children:\n"
      "    - {type: button, name: a, text: A, position: absolute, left: 100, top: 100, width: 80, height: 40}\n"
      "    - {type: button, name: b, text: B, position: absolute, left: 300, top: 140, width: 80, height: 40}\n"
      "    - {type: button, name: c, text: C, position: absolute, left: 500, top: 100, width: 80, height: 40}\n"
      "    - {type: button, name: d, text: D, position: absolute, left: 100, top: 300, width: 80, height: 40}\n"), 0);
    Frame();

    ASSERT_TRUE(_ui->Focus("a"));
    Frame();

    Press(Action::Ui_Right);
    EXPECT_EQ(_ui->GetFocused(), "b");

    Press(Action::Ui_Right);
    EXPECT_EQ(_ui->GetFocused(), "c");

    Press(Action::Ui_Right);
    EXPECT_EQ(_ui->GetFocused(), "c") << "nothing lies further that way";

    Press(Action::Ui_Left);
    Press(Action::Ui_Left);
    EXPECT_EQ(_ui->GetFocused(), "a");

    Press(Action::Ui_Down);
    EXPECT_EQ(_ui->GetFocused(), "d");

    Press(Action::Ui_Up);
    EXPECT_EQ(_ui->GetFocused(), "a");
  }

  TEST_F(UiNavigationTest, ADirectionStaysInsideAFileThatIsModal)
  {
    ShowMenu("game", "", "- {type: button, name: inventory, text: Inventory}\n");
    ShowMenu("dialog", "modal: true\n", "- {type: button, name: yes, text: Yes}\n");
    EXPECT_EQ(_ui->GetFocused(), "yes");

    Press(Action::Ui_Up);
    Press(Action::Ui_Left);
    EXPECT_EQ(_ui->GetFocused(), "yes");
  }

  // sounds

  TEST_F(UiNavigationTest, AGameMakesSoundsFromTheEventsOfMovingAndChoosing)
  {
    ShowMenu(
      "menu", "",
      "- {type: button, name: a, text: A}\n"
      "- {type: button, name: b, text: B}\n"
      "- {type: checkbox, name: c, text: C}\n");

    // a game hooks its sounds to the events of every element at once
    std::vector<std::string> sounds;
    _ui->OnAny("focused", [&sounds](const UiElementEvent &event) { sounds.push_back("tick " + event.target_name); });
    _ui->OnAny("click", [&sounds](const UiElementEvent &event) { sounds.push_back("clack " + event.target_name); });
    _ui->OnAny("changed", [&sounds](const UiElementEvent &event) { sounds.push_back("switch " + event.target_name); });
    _ui->OnAny("cancel", [&sounds](const UiElementEvent &) { sounds.push_back("back"); });

    Press(Action::Ui_Down);
    Press(Action::Ui_Down);
    Press(Action::Ui_Down);
    Press(Action::Ui_Accept);
    Press(Action::Ui_Cancel);

    EXPECT_THAT(sounds, ElementsAre("tick a", "tick b", "tick c", "clack c", "switch c", "back"));
  }

  TEST_F(UiNavigationTest, ThePointerMakesTheSameSounds)
  {
    ShowMenu("menu", "", "- {type: button, name: a, text: A}\n");

    std::vector<std::string> sounds;
    _ui->OnAny("pointer_enter", [&sounds](const UiElementEvent &event) { sounds.push_back("hover " + event.target_name); });
    _ui->OnAny("click", [&sounds](const UiElementEvent &event) { sounds.push_back("clack " + event.target_name); });

    ClickAt(10.0, 10.0);

    // the root hears of the pointer as well, and a game tells by the name
    EXPECT_THAT(sounds, ElementsAre("hover ", "hover a", "clack a"));
  }
}
