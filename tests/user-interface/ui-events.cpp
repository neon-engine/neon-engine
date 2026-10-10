#include "ui-fixture.hpp"

#include <algorithm>

// What reaches the game: a click on a button, as an event it asks for and
// as a function it is called with, and a change the player makes to a
// control that names a function with `on_change`.

namespace
{
  using neon::Action;
  using neon::FieldValue;
  using neon::Key;
  using neon::UiEvent;
  using neon::UiValue;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;

  class UiEventTest : public UiTest
  {
  protected:
    /// Two buttons of 100 by 50 next to each other, from the left top
    /// corner of the frame.
    void ShowMenu()
    {
      ASSERT_GE(Show(
        "ui: menu\n"
        "root:\n"
        "  type: panel\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  align_items: flex-start\n"
        "  children:\n"
        "    - type: button\n"
        "      name: start\n"
        "      text: Start\n"
        "      box_sizing: border-box\n"
        "      width: 100\n"
        "      height: 50\n"
        "    - type: button\n"
        "      name: quit\n"
        "      text: Quit\n"
        "      box_sizing: border-box\n"
        "      width: 100\n"
        "      height: 50\n"), 0) << _logger->Messages(LogLevel::Error);
    }
  };

  TEST_F(UiEventTest, NothingHappensWhileNothingIsDone)
  {
    ShowMenu();
    Frame();

    EXPECT_TRUE(_ui->GetEvents().empty());
    EXPECT_FALSE(_ui->WasClicked("start"));
  }

  TEST_F(UiEventTest, AClickOnAButtonIsAnEventWithItsName)
  {
    ShowMenu();
    ClickAt(50, 25);

    ASSERT_EQ(_ui->GetEvents().size(), 1u);

    const UiEvent &event = _ui->GetEvents().front();
    EXPECT_EQ(event.kind, UiEvent::Kind::Click);
    EXPECT_EQ(event.element, "start");
    EXPECT_EQ(event.document, "menu");

    EXPECT_TRUE(_ui->WasClicked("start"));
    EXPECT_FALSE(_ui->WasClicked("quit"));
  }

  TEST_F(UiEventTest, AnEventIsThereForOneFrame)
  {
    ShowMenu();
    ClickAt(50, 25);
    ASSERT_TRUE(_ui->WasClicked("start"));

    Frame();

    EXPECT_TRUE(_ui->GetEvents().empty());
    EXPECT_FALSE(_ui->WasClicked("start"));
  }

  TEST_F(UiEventTest, EventsStayUntilTheNextUpdateAndNotUntilTheNextDraw)
  {
    ShowMenu();
    ClickAt(50, 25);

    // what the game reads between the two
    _ui->Draw();
    _ui->Draw();

    EXPECT_TRUE(_ui->WasClicked("start"));
  }

  TEST_F(UiEventTest, AClickCallsTheFunctionOfItsName)
  {
    ShowMenu();

    int started = 0;
    int quit = 0;
    _ui->OnClick("start", [&started] { started++; });
    _ui->OnClick("quit", [&quit] { quit++; });

    ClickAt(50, 25);

    EXPECT_EQ(started, 1);
    EXPECT_EQ(quit, 0);

    ClickAt(150, 25);
    ClickAt(150, 25);

    EXPECT_EQ(started, 1);
    EXPECT_EQ(quit, 2);
  }

  TEST_F(UiEventTest, AFunctionCanBeRegisteredBeforeItsFileIsShown)
  {
    int started = 0;
    _ui->OnClick("start", [&started] { started++; });

    ShowMenu();
    ClickAt(50, 25);

    EXPECT_EQ(started, 1);
  }

  TEST_F(UiEventTest, ASecondFunctionReplacesTheFirst)
  {
    ShowMenu();

    int first = 0;
    int second = 0;
    _ui->OnClick("start", [&first] { first++; });
    _ui->OnClick("start", [&second] { second++; });

    ClickAt(50, 25);

    EXPECT_EQ(first, 0);
    EXPECT_EQ(second, 1);
  }

  TEST_F(UiEventTest, NoFunctionTakesTheFunctionAway)
  {
    ShowMenu();

    int started = 0;
    _ui->OnClick("start", [&started] { started++; });
    _ui->OnClick("start", {});

    ClickAt(50, 25);

    EXPECT_EQ(started, 0);

    // the event is there all the same
    EXPECT_TRUE(_ui->WasClicked("start"));
  }

  TEST_F(UiEventTest, AFunctionSeesTheEventItIsCalledFor)
  {
    ShowMenu();

    bool was_clicked = false;
    _ui->OnClick("start", [this, &was_clicked] { was_clicked = _ui->WasClicked("start"); });

    ClickAt(50, 25);

    EXPECT_TRUE(was_clicked);
  }

  TEST_F(UiEventTest, AFunctionCanTakeTheFileAwayThatItIsCalledFrom)
  {
    ASSERT_GE(Show(
      "root:\n  type: panel\n  name: hud\n  width: 10\n  height: 10\n", "ui/hud.ui.yml"), 0);

    const int menu = Show(
      "ui: menu\n"
      "modal: true\n"
      "root:\n"
      "  type: button\n"
      "  name: resume\n"
      "  text: Resume\n"
      "  width: 100\n"
      "  height: 50\n", "ui/menu.ui.yml");
    ASSERT_GE(menu, 0) << _logger->Messages(LogLevel::Error);

    _ui->OnClick("resume", [this, menu] { _ui->Unload(menu); });

    ClickAt(50, 25);

    EXPECT_EQ(_ui->Find("resume"), nullptr);
    EXPECT_NE(_ui->Find("hud"), nullptr);
    EXPECT_EQ(_ui->GetFocused(), "");

    // and the frames after it run as if it had never been there
    Frame();
    ClickAt(50, 25);
    EXPECT_TRUE(_ui->GetEvents().empty());
  }

  TEST_F(UiEventTest, AFunctionCanShowAnotherFile)
  {
    ShowMenu();
    WriteAsset(
      "ui/options.ui.yml",
      "ui: options\nmodal: true\nroot:\n  type: button\n  name: back\n  text: Back\n");

    _ui->OnClick("start", [this] { _ui->Load("assets://ui/options.ui.yml"); });

    ClickAt(50, 25);
    Frame();

    EXPECT_NE(_ui->Find("back"), nullptr);
    EXPECT_EQ(_ui->GetFocused(), "back");
  }

  TEST_F(UiEventTest, AButtonWithoutANameReportsNothing)
  {
    ASSERT_GE(Show("root:\n  type: button\n  text: Start\n  width: 100\n  height: 50\n"), 0);

    ClickAt(50, 25);

    EXPECT_TRUE(_ui->GetEvents().empty());
  }

  TEST_F(UiEventTest, AClickWithTheKeysIsTheSameEvent)
  {
    ShowMenu();
    ASSERT_TRUE(_ui->Focus("quit"));

    int quit = 0;
    _ui->OnClick("quit", [&quit] { quit++; });

    Release();
    _input.state.SetAction(Action::Ui_Accept);
    Frame();

    ASSERT_EQ(_ui->GetEvents().size(), 1u);
    EXPECT_EQ(_ui->GetEvents().front().element, "quit");
    EXPECT_EQ(_ui->GetEvents().front().document, "menu");
    EXPECT_EQ(quit, 1);
  }

  TEST_F(UiEventTest, EventsOfOneFrameAreInTheOrderTheyHappenedIn)
  {
    ShowMenu();
    ASSERT_TRUE(_ui->Focus("quit"));

    // the pointer lets go of one button in the frame the key is pressed
    // for the other
    Release();
    PointAt(50, 25);
    _input.state.SetAction(Action::Pointer_Primary);
    Frame();

    Release();
    ASSERT_TRUE(_ui->Focus("quit"));
    _input.state.SetAction(Action::Ui_Accept);
    Frame();

    ASSERT_EQ(_ui->GetEvents().size(), 2u);
    EXPECT_EQ(_ui->GetEvents()[0].element, "start");
    EXPECT_EQ(_ui->GetEvents()[1].element, "quit");
  }

  TEST_F(UiEventTest, AFunctionThatThrowsEndsTheUpdate)
  {
    ShowMenu();
    _ui->OnClick("start", [] { throw std::runtime_error("the game could not start"); });

    Release();
    PointAt(50, 25);
    _input.state.SetAction(Action::Pointer_Primary);
    Frame();

    Release();
    EXPECT_THROW(_ui->Update(), std::runtime_error);

    // the user interface goes on afterwards
    Release();
    Frame();
    EXPECT_TRUE(_ui->GetEvents().empty());
  }

  // a function the element names

  TEST_F(UiEventTest, AButtonNamesTheFunctionItsClickCallsWithWhatItIsHanded)
  {
    ASSERT_GE(Show(
      "ui: terminal\n"
      "values:\n"
      "  door: locked\n"
      "  power: 0.4\n"
      "  armed: true\n"
      "root:\n"
      "  type: panel\n"
      "  align_items: flex-start\n"
      "  children:\n"
      "    - type: button\n"
      "      name: unlock\n"
      "      on_click: open('safe', 2, door, power, armed, gone, $event)\n"
      "      width: 100\n"
      "      height: 50\n"
      "    - type: button\n"
      "      on_click: alarm\n"
      "      width: 100\n"
      "      height: 50\n"), 0) << _logger->Messages(LogLevel::Error);

    _ui->SetTextOf("terminal", "door", "unlocked");
    ClickAt(50, 25);

    ASSERT_EQ(_ui->GetEvents().size(), 1u);
    const UiEvent &event = _ui->GetEvents().front();
    EXPECT_EQ(event.element, "unlock");
    EXPECT_GE(event.document_id, 0);
    EXPECT_EQ(event.call.function, "open");

    using Kind = neon::UiCallArgument::Kind;
    const auto &arguments = event.call.arguments;
    ASSERT_EQ(arguments.size(), 7u);
    EXPECT_EQ(arguments[0].kind, Kind::Text);
    EXPECT_EQ(arguments[0].text, "safe");
    EXPECT_EQ(arguments[1].kind, Kind::Number);
    EXPECT_DOUBLE_EQ(arguments[1].number, 2.0);

    // a value is what it holds when the button is chosen
    EXPECT_EQ(arguments[2].kind, Kind::Text);
    EXPECT_EQ(arguments[2].text, "unlocked");
    EXPECT_EQ(arguments[3].kind, Kind::Number);
    EXPECT_DOUBLE_EQ(arguments[3].number, 0.4);
    EXPECT_EQ(arguments[4].kind, Kind::Flag);
    EXPECT_TRUE(arguments[4].flag);
    EXPECT_EQ(arguments[5].kind, Kind::Nothing);
    EXPECT_EQ(arguments[6].kind, Kind::Event);

    // a button without a name is told of when it calls something
    Release();
    Frame();
    ClickAt(150, 25);
    ASSERT_EQ(_ui->GetEvents().size(), 1u);
    EXPECT_EQ(_ui->GetEvents().front().element, "");
    EXPECT_EQ(_ui->GetEvents().front().call.function, "alarm");
    EXPECT_TRUE(_ui->GetEvents().front().call.arguments.empty());
  }

  TEST_F(UiEventTest, AClickThatCallsNothingHasNoCall)
  {
    ShowMenu();
    ClickAt(50, 25);

    ASSERT_EQ(_ui->GetEvents().size(), 1u);
    EXPECT_TRUE(_ui->GetEvents().front().call.IsEmpty());
  }

  // a change the player makes, which calls the function `on_change` names

  class UiChangeTest : public UiTest
  {
  protected:
    std::vector<UiEvent> _changes;

    /// Elements under a column that starts at the left top corner.
    void ShowColumn(const std::string &children, const std::string &top = "")
    {
      ASSERT_GE(Show(
        top +
        "ui: settings\n"
        "root:\n"
        "  type: panel\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  flex_direction: column\n"
        "  align_items: flex-start\n"
        "  children:\n" +
        Indented(children, "    ")), 0)
        << _logger->Messages(LogLevel::Error);

      Step();
      _changes.clear();
    }

    /// A frame, after which the changes of it are kept.
    void Step()
    {
      Frame();
      for (const UiEvent &event : _ui->GetEvents())
      {
        if (event.kind == UiEvent::Kind::Change) { _changes.push_back(event); }
      }
    }

    /// The changes since the last call.
    [[nodiscard]] std::vector<UiEvent> TakeChanges()
    {
      std::vector<UiEvent> changes;
      changes.swap(_changes);
      return changes;
    }

    /// A frame with the action held down, and one with it released.
    void PressAction(const Action action)
    {
      Release();
      _input.state.SetAction(action);
      Step();
      Release();
      Step();
    }

    void PressKey(const Key key)
    {
      Release();
      _input.state.AddKeyEvent({key, true, false, {}});
      _input.state.AddKeyEvent({key, false, false, {}});
      Step();
      Release();
    }

    void Type(const std::string &text)
    {
      Release();
      _input.state.AddText(text);
      Step();
      Release();
    }

    [[nodiscard]] bool ToldListeners(const std::string &element) const
    {
      return std::ranges::any_of(_ui->GetElementEvents(), [&element](const neon::UiElementEvent &event)
      {
        return event.name == "changed" && event.target_name == element;
      });
    }
  };

  TEST_F(UiChangeTest, ASliderThePlayerMovesCallsItsFunctionWithTheNumber)
  {
    ShowColumn(
      "- type: slider\n"
      "  name: volume\n"
      "  min: 0\n"
      "  max: 100\n"
      "  step: 5\n"
      "  value: 50\n"
      "  autofocus: true\n"
      "  on_change: \"tune(3, $event)\"\n");

    PressAction(Action::Ui_Right);

    auto changes = TakeChanges();
    ASSERT_EQ(changes.size(), 1u);
    const UiEvent &event = changes.front();
    EXPECT_EQ(event.kind, UiEvent::Kind::Change);
    EXPECT_EQ(event.element, "volume");
    EXPECT_EQ(event.document, "settings");
    EXPECT_GE(event.document_id, 0);
    EXPECT_EQ(event.surface, "window");
    EXPECT_EQ(event.value, UiValue::Number(55.0));

    using Kind = neon::UiCallArgument::Kind;
    EXPECT_EQ(event.call.function, "tune");
    ASSERT_EQ(event.call.arguments.size(), 2u);
    EXPECT_EQ(event.call.arguments[0].kind, Kind::Number);
    EXPECT_DOUBLE_EQ(event.call.arguments[0].number, 3.0);
    EXPECT_EQ(event.call.arguments[1].kind, Kind::Event);

    // nothing while it stays where it is
    Step();
    Step();
    EXPECT_TRUE(TakeChanges().empty());

    PressKey(Key::End);
    changes = TakeChanges();
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes.front().value, UiValue::Number(100.0));

    // a step beyond the end changes nothing, and calls nothing
    PressAction(Action::Ui_Right);
    EXPECT_TRUE(TakeChanges().empty());
  }

  TEST_F(UiChangeTest, ASliderThatIsDraggedCallsOnceForEachFrameItMovedIn)
  {
    ShowColumn(
      "- type: slider\n"
      "  name: volume\n"
      "  min: 0\n"
      "  max: 100\n"
      "  step: 1\n"
      "  value: 0\n"
      "  on_change: tune\n");

    // the knob is 16 wide, and moves over 144 of the 160
    Release();
    PointAt(8.0 + 72.0, 12.0);
    _input.state.SetAction(Action::Pointer_Primary);
    Step();

    auto changes = TakeChanges();
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes.front().value, UiValue::Number(50.0));
    EXPECT_TRUE(changes.front().call.arguments.empty());

    PointAt(8.0 + 36.0, 12.0);
    Step();
    changes = TakeChanges();
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes.front().value, UiValue::Number(25.0));

    // held where it is
    Step();
    EXPECT_TRUE(TakeChanges().empty());

    Release();
    Step();
    EXPECT_TRUE(TakeChanges().empty());
  }

  TEST_F(UiChangeTest, WhatTheGameSetsCallsNothing)
  {
    ShowColumn(
      "- type: slider\n"
      "  name: volume\n"
      "  min: 0\n"
      "  max: 100\n"
      "  value: \"{volume}\"\n"
      "  on_change: tune\n"
      "- type: toggle\n"
      "  name: vsync\n"
      "  on_change: tune\n"
      "- type: select\n"
      "  name: quality\n"
      "  options: [low, high]\n"
      "  on_change: tune\n"
      "- type: input\n"
      "  name: player\n"
      "  value: \"{player}\"\n"
      "  on_change: tune\n");

    _ui->SetNumber("volume", 40);
    _ui->SetText("player", "Ada");
    Step();
    EXPECT_TRUE(TakeChanges().empty());

    FieldValue value;
    ASSERT_TRUE(_ui->GetField(_ui->FindByName("volume"), "value", value));
    EXPECT_FLOAT_EQ(std::get<float>(value), 40.0f) << "the slider follows the value";

    // through the fields, as a script sets them: the listeners hear of it,
    // and no function is called
    EXPECT_TRUE(_ui->SetField(_ui->FindByName("volume"), "value", 70.0f));
    EXPECT_TRUE(_ui->SetField(_ui->FindByName("vsync"), "checked", true));
    EXPECT_TRUE(_ui->SetField(_ui->FindByName("quality"), "value", std::string("high")));
    EXPECT_TRUE(_ui->SetField(_ui->FindByName("player"), "value", std::string("Grace")));
    Step();
    EXPECT_TRUE(ToldListeners("volume"));
    EXPECT_TRUE(ToldListeners("vsync"));
    EXPECT_TRUE(ToldListeners("quality"));
    EXPECT_TRUE(TakeChanges().empty());
  }

  TEST_F(UiChangeTest, AToggleAndACheckboxCallWithAFlag)
  {
    ShowColumn(
      "- type: toggle\n"
      "  name: vsync\n"
      "  text: V-Sync\n"
      "  autofocus: true\n"
      "  on_change: \"tune('vsync', $event)\"\n"
      "- type: checkbox\n"
      "  name: subtitles\n"
      "  text: Subtitles\n"
      "  checked: true\n"
      "  on_change: tune($event)\n");

    PressAction(Action::Ui_Right);
    auto changes = TakeChanges();
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes.front().element, "vsync");
    EXPECT_EQ(changes.front().value, UiValue::Flag(true));
    ASSERT_EQ(changes.front().call.arguments.size(), 2u);
    EXPECT_EQ(changes.front().call.arguments[0].text, "vsync");

    // right again leaves it on
    PressAction(Action::Ui_Right);
    EXPECT_TRUE(TakeChanges().empty());

    PressAction(Action::Ui_Accept);
    changes = TakeChanges();
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes.front().value, UiValue::Flag(false));

    // a click ticks the checkbox, and is a click as well, which calls the
    // callback of the name and no function
    int clicked = 0;
    _ui->OnClick("subtitles", [&clicked] { clicked++; });
    _ui->FocusElement(_ui->FindByName("subtitles"));
    PressAction(Action::Ui_Accept);

    changes = TakeChanges();
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes.front().element, "subtitles");
    EXPECT_EQ(changes.front().value, UiValue::Flag(false));
    EXPECT_EQ(clicked, 1) << "a change is no click";
  }

  TEST_F(UiChangeTest, ASelectCallsWithTheValueOfTheChoice)
  {
    ShowColumn(
      "- type: select\n"
      "  name: quality\n"
      "  autofocus: true\n"
      "  value: \"{quality}\"\n"
      "  on_change: tune(quality, $event)\n"
      "  options:\n"
      "    - value: 1\n"
      "      text: Low\n"
      "    - value: 2\n"
      "      text: High\n");

    // closed, down moves the focus and chooses nothing; accept opens the
    // list, down moves through it, and accept chooses
    PressAction(Action::Ui_Down);
    EXPECT_TRUE(TakeChanges().empty());

    PressAction(Action::Ui_Accept);
    PressAction(Action::Ui_Down);
    PressAction(Action::Ui_Accept);
    auto changes = TakeChanges();
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes.front().value, UiValue::Text("2"));

    // a value it follows is handed over as it is after the change
    using Kind = neon::UiCallArgument::Kind;
    ASSERT_EQ(changes.front().call.arguments.size(), 2u);
    EXPECT_EQ(changes.front().call.arguments[0].kind, Kind::Text);
    EXPECT_EQ(changes.front().call.arguments[0].text, "2");
    EXPECT_EQ(_ui->GetValue("quality", nullptr), "2");

    // opening and moving through the list chooses nothing until accept
    PressAction(Action::Ui_Accept);
    PressAction(Action::Ui_Up);
    EXPECT_TRUE(TakeChanges().empty());

    PressAction(Action::Ui_Accept);
    changes = TakeChanges();
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes.front().value, UiValue::Text("1"));
  }

  TEST_F(UiChangeTest, AnInputAndATextAreaCallWithTheTextOnceAFrame)
  {
    ShowColumn(
      "- type: input\n"
      "  name: player\n"
      "  autofocus: true\n"
      "  on_change: rename($event)\n"
      "- type: textarea\n"
      "  name: notes\n"
      "  on_change: \"rename('notes', $event)\"\n");

    Type("Ada");
    auto changes = TakeChanges();
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes.front().element, "player");
    EXPECT_EQ(changes.front().value, UiValue::Text("Ada"));

    PressKey(Key::Backspace);
    changes = TakeChanges();
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes.front().value, UiValue::Text("Ad"));

    // moving the caret changes nothing
    PressKey(Key::Left);
    EXPECT_TRUE(TakeChanges().empty());

    _ui->FocusElement(_ui->FindByName("notes"));
    Type("one");
    PressKey(Key::Enter);
    Type("two");
    changes = TakeChanges();
    ASSERT_EQ(changes.size(), 3u);
    EXPECT_EQ(changes.back().element, "notes");
    EXPECT_EQ(changes.back().value, UiValue::Text("one\ntwo"));
  }

  TEST_F(UiChangeTest, AControlWithoutOnChangeIsNoEvent)
  {
    ShowColumn(
      "- type: slider\n"
      "  name: volume\n"
      "  min: 0\n"
      "  max: 100\n"
      "  value: 50\n"
      "  autofocus: true\n");

    PressAction(Action::Ui_Right);
    EXPECT_TRUE(TakeChanges().empty());
    EXPECT_TRUE(_ui->GetEvents().empty());
  }

  TEST_F(UiChangeTest, OnChangeIsAFieldAScriptReadsAndSets)
  {
    ShowColumn(
      "- type: slider\n"
      "  name: volume\n"
      "  autofocus: true\n"
      "  on_change: \"tune(3, $event)\"\n");
    const neon::UiHandle volume = _ui->FindByName("volume");

    FieldValue value;
    ASSERT_TRUE(_ui->GetField(volume, "on_change", value));
    EXPECT_EQ(std::get<std::string>(value), "tune(3, $event)");

    EXPECT_TRUE(_ui->SetField(volume, "on_change", std::string("louder")));
    PressAction(Action::Ui_Right);
    auto changes = TakeChanges();
    ASSERT_EQ(changes.size(), 1u);
    EXPECT_EQ(changes.front().call.function, "louder");

    // an empty text takes it away, and what is no call is refused
    EXPECT_FALSE(_ui->SetField(volume, "on_change", std::string("louder(")));
    EXPECT_TRUE(_ui->SetField(volume, "on_change", std::string("")));
    PressAction(Action::Ui_Right);
    EXPECT_TRUE(TakeChanges().empty());
  }

  TEST_F(UiChangeTest, WhatIsNoCallIsAProblemOfTheFile)
  {
    ExpectProblemsUnderRoot(
      "- type: slider\n"
      "  name: volume\n"
      "  on_change: \"tune(\"\n"
      "- type: toggle\n"
      "  name: vsync\n"
      "  on_change: 3\n"
      "- type: select\n"
      "  name: quality\n"
      "  options: [low]\n"
      "  on_change: \"$volume\"\n"
      "- type: input\n"
      "  name: player\n"
      "  on_change: \"rename(,)\"\n",
      {
        "assets://ui/test.ui.yml:6: 'on_change' of slider 'volume' is 'tune(', which is no call of a function: "
        "the ( is not closed with )",
        "assets://ui/test.ui.yml:9: 'on_change' of toggle 'vsync' is a number, where the call of a function was "
        "expected, such as unlock or open('safe')",
        "assets://ui/test.ui.yml:13: 'on_change' of select 'quality' is '$volume', which is no call of a function: "
        "it has to start with the name of a function: letters, digits, and _, and no digit first",
        "assets://ui/test.ui.yml:16: 'on_change' of input 'player' is 'rename(,)', which is no call of a function: "
        "',)' is no argument: a number, a text between quotes, true, false, the name of a value, or $event"
      });
  }
}
