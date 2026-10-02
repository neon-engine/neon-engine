#include "ui-fixture.hpp"

// The pointer, the keys, and a controller: what is under the pointer, what
// is chosen, where the focus goes, and what of the input is left for the
// game.

namespace
{
  using neon::Action;
  using neon::Axis;
  using neon::InputState;
  using neon::testing::LogLevel;
  using neon::testing::UiTest;
  using ::testing::_;

  class UiInputTest : public UiTest
  {
  protected:
    /// Buttons of 100 by 50 in a grid of three by two, 20 apart, and 100
    /// from the left top corner of the frame:
    ///
    ///     a b c
    ///     d e f
    void ShowGrid(const std::string &top = "", const std::string &of_e = "")
    {
      std::string yaml =
        top +
        "ui: grid\n"
        "root:\n"
        "  type: panel\n"
        "  name: root\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  children:\n";

      const char *names[2][3] = {{"a", "b", "c"}, {"d", "e", "f"}};
      for (int row = 0; row < 2; row++)
      {
        for (int column = 0; column < 3; column++)
        {
          const std::string name = names[row][column];
          yaml +=
            "    - type: button\n"
            "      name: " + name + "\n"
            "      text: " + name + "\n"
            "      position: absolute\n"
            "      box_sizing: border-box\n"
            "      width: 100\n"
            "      height: 50\n"
            "      left: " + std::to_string(100 + column * 120) + "\n"
            "      top: " + std::to_string(100 + row * 70) + "\n";

          if (name == "e") { yaml += Indented(of_e, "      "); }
        }
      }

      ASSERT_GE(Show(yaml), 0) << _logger->Messages(LogLevel::Error);
    }

    /// The input as the game sees it.
    [[nodiscard]] const InputState &GameInput() const
    {
      return _ui->GetGameInput()->GetInputState();
    }

    /// A frame with everything released but what is given.
    /// A frame with these actions of the user interface held, and these
    /// keys down for the game.
    void FrameWith(const std::initializer_list<Action> actions, const std::initializer_list<neon::Key> keys = {})
    {
      Release();
      for (const Action action : actions) { _input.state.SetAction(action); }
      for (const neon::Key key : keys) { _input.state.SetKeyDown(key); }
      Frame();
    }
  };

  // what is under the pointer

  TEST_F(UiInputTest, NothingIsUnderAPointerThatIsNotThere)
  {
    ShowGrid();
    Frame();

    for (const std::string name : {"a", "b", "c", "d", "e", "f"})
    {
      EXPECT_FALSE(Element(name).GetStates().hover) << name;
    }
  }

  TEST_F(UiInputTest, AButtonUnderThePointerIsInItsStateHover)
  {
    ShowGrid();

    PointAt(150, 125);
    Frame();

    EXPECT_TRUE(Element("a").GetStates().hover);
    EXPECT_FALSE(Element("b").GetStates().hover);
    EXPECT_FALSE(Element("d").GetStates().hover);
  }

  TEST_F(UiInputTest, TheStateEndsWhenThePointerLeaves)
  {
    ShowGrid();

    PointAt(150, 125);
    Frame();
    PointAt(210, 125);
    Frame();

    // between two buttons
    EXPECT_FALSE(Element("a").GetStates().hover);
    EXPECT_FALSE(Element("b").GetStates().hover);

    _input.state.ClearPointer();
    Frame();
    EXPECT_FALSE(Element("a").GetStates().hover);
  }

  TEST_F(UiInputTest, AnEdgeBelongsToTheBoxItStarts)
  {
    ShowGrid();

    PointAt(100, 100);
    Frame();
    EXPECT_TRUE(Element("a").GetStates().hover);

    PointAt(199, 149);
    Frame();
    EXPECT_TRUE(Element("a").GetStates().hover);

    PointAt(200, 149);
    Frame();
    EXPECT_FALSE(Element("a").GetStates().hover);

    PointAt(199, 150);
    Frame();
    EXPECT_FALSE(Element("a").GetStates().hover);
  }

  TEST_F(UiInputTest, TheStyleOfAButtonFollowsThePointer)
  {
    ShowGrid("", "hover:\n  background_color: \"#ff0000\"\n");

    Frame();
    EXPECT_FLOAT_EQ(Element("e").GetStyle().background_color.g, 0.259f);

    PointAt(270, 195);
    Frame();

    EXPECT_FLOAT_EQ(Element("e").GetStyle().background_color.r, 1);
    EXPECT_FLOAT_EQ(Element("e").GetStyle().background_color.g, 0);
  }

  TEST_F(UiInputTest, WhatIsAboveAnElementUnderThePointerIsInTheStateAsWell)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: row\n"
      "  width: 300\n"
      "  height: 50\n"
      "  children:\n"
      "    - type: button\n"
      "      name: inside\n"
      "      text: a\n"
      "      width: 50\n"
      "- type: panel\n"
      "  name: other\n"
      "  width: 300\n"
      "  height: 50\n"), 0) << _logger->Messages(LogLevel::Error);

    PointAt(10, 10);
    Frame();

    EXPECT_TRUE(Element("inside").GetStates().hover);
    EXPECT_TRUE(Element("row").GetStates().hover);
    EXPECT_FALSE(Element("other").GetStates().hover);
  }

  TEST_F(UiInputTest, ThePointerGoesThroughWhatDoesNotTakeIt)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: plain\n"
      "  width: 100\n"
      "  height: 100\n"
      "- type: panel\n"
      "  name: window\n"
      "  width: 100\n"
      "  height: 100\n"
      "  pointer_events: auto\n"
      "- type: button\n"
      "  name: ghost\n"
      "  text: a\n"
      "  width: 100\n"
      "  height: 100\n"
      "  pointer_events: none\n"), 0) << _logger->Messages(LogLevel::Error);

    PointAt(50, 50);
    Frame();
    EXPECT_FALSE(Element("plain").GetStates().hover);

    PointAt(150, 50);
    Frame();
    EXPECT_TRUE(Element("window").GetStates().hover);

    ClickAt(250, 50);
    EXPECT_FALSE(_ui->WasClicked("ghost"));
  }

  TEST_F(UiInputTest, OfTwoElementsTheOneOnTopIsUnderThePointer)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: button\n"
      "  name: first\n"
      "  position: absolute\n"
      "  top: 0\n"
      "  left: 0\n"
      "  width: 100\n"
      "  height: 100\n"
      "- type: button\n"
      "  name: second\n"
      "  position: absolute\n"
      "  top: 0\n"
      "  left: 50\n"
      "  width: 100\n"
      "  height: 100\n"
      "- type: button\n"
      "  name: raised\n"
      "  position: absolute\n"
      "  top: 200\n"
      "  left: 0\n"
      "  width: 100\n"
      "  height: 100\n"
      "  z_index: 1\n"
      "- type: button\n"
      "  name: later\n"
      "  position: absolute\n"
      "  top: 200\n"
      "  left: 50\n"
      "  width: 100\n"
      "  height: 100\n"), 0) << _logger->Messages(LogLevel::Error);

    // what is written later is drawn later, and is on top
    ClickAt(75, 50);
    EXPECT_TRUE(_ui->WasClicked("second"));
    EXPECT_FALSE(_ui->WasClicked("first"));

    // unless the other says that it is above
    ClickAt(75, 250);
    EXPECT_TRUE(_ui->WasClicked("raised"));
    EXPECT_FALSE(_ui->WasClicked("later"));
  }

  TEST_F(UiInputTest, WhatIsCutOffCannotBePointedAt)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  name: window\n"
      "  width: 100\n"
      "  height: 100\n"
      "  overflow: hidden\n"
      "  children:\n"
      "    - type: button\n"
      "      name: wide\n"
      "      text: a\n"
      "      flex_shrink: 0\n"
      "      width: 300\n"
      "      height: 50\n"), 0) << _logger->Messages(LogLevel::Error);

    ClickAt(50, 25);
    EXPECT_TRUE(_ui->WasClicked("wide"));

    ClickAt(150, 25);
    EXPECT_FALSE(_ui->WasClicked("wide"));
  }

  TEST_F(UiInputTest, WhatIsHiddenCannotBePointedAt)
  {
    ShowGrid("", "hidden: \"{hidden}\"\n");
    _ui->SetFlag("hidden", true);

    ClickAt(270, 195);

    EXPECT_FALSE(_ui->WasClicked("e"));

    // and the game sees the pointer, which hit nothing
    EXPECT_TRUE(GameInput().HasPointer());
  }

  TEST_F(UiInputTest, ThePointerIsInPixelsOfTheFrame)
  {
    _renderer.SetResolution(960, 540);
    ShowGrid();

    // `a` is at 100 of the file, which is 50 of this frame
    ClickAt(75, 60);
    EXPECT_TRUE(_ui->WasClicked("a"));

    // `e` reaches from 220 to 320 and from 170 to 220 of the file
    ClickAt(135, 97);
    EXPECT_FALSE(_ui->WasClicked("a"));
    EXPECT_TRUE(_ui->WasClicked("e"));

    ClickAt(270, 195);
    EXPECT_TRUE(_ui->GetEvents().empty());
  }

  // a click

  TEST_F(UiInputTest, AClickIsAPressAndAReleaseOnTheSameButton)
  {
    ShowGrid();

    PointAt(150, 125);
    FrameWith({Action::Pointer_Primary});
    EXPECT_FALSE(_ui->WasClicked("a")) << "while the button is held";

    FrameWith({Action::Pointer_Primary});
    EXPECT_FALSE(_ui->WasClicked("a"));

    FrameWith({});
    EXPECT_TRUE(_ui->WasClicked("a"));
  }

  TEST_F(UiInputTest, LettingGoSomewhereElseIsNoClick)
  {
    ShowGrid();

    PointAt(150, 125);
    FrameWith({Action::Pointer_Primary});

    PointAt(270, 125);
    FrameWith({Action::Pointer_Primary});
    FrameWith({});

    EXPECT_TRUE(_ui->GetEvents().empty());
  }

  TEST_F(UiInputTest, PressingSomewhereElseIsNoClick)
  {
    ShowGrid();

    PointAt(50, 50);
    FrameWith({Action::Pointer_Primary});

    PointAt(150, 125);
    FrameWith({Action::Pointer_Primary});
    FrameWith({});

    EXPECT_TRUE(_ui->GetEvents().empty());
  }

  TEST_F(UiInputTest, AButtonThatIsHeldIsInItsStateActive)
  {
    ShowGrid();

    PointAt(150, 125);
    FrameWith({Action::Pointer_Primary});
    EXPECT_TRUE(Element("a").GetStates().active);
    EXPECT_TRUE(Element("a").GetStates().hover);

    // it looks as it did before while the pointer is somewhere else, and
    // is still the one that is held
    PointAt(50, 50);
    FrameWith({Action::Pointer_Primary});
    EXPECT_FALSE(Element("a").GetStates().active);

    PointAt(150, 125);
    FrameWith({Action::Pointer_Primary});
    EXPECT_TRUE(Element("a").GetStates().active);

    FrameWith({});
    EXPECT_FALSE(Element("a").GetStates().active);
    EXPECT_TRUE(_ui->WasClicked("a"));
  }

  TEST_F(UiInputTest, AButtonThatCannotBeUsedTakesNoClick)
  {
    ShowGrid("", "enabled: false\n");

    ClickAt(270, 195);

    EXPECT_FALSE(_ui->WasClicked("e"));
    EXPECT_TRUE(Element("e").GetStates().disabled);
    EXPECT_FALSE(Element("e").GetStates().active);
  }

  TEST_F(UiInputTest, APressGivesTheButtonTheFocus)
  {
    ShowGrid();
    EXPECT_EQ(_ui->GetFocused(), "");

    PointAt(270, 195);
    FrameWith({Action::Pointer_Primary});

    EXPECT_EQ(_ui->GetFocused(), "e");
    EXPECT_TRUE(Element("e").GetStates().focus);
  }

  TEST_F(UiInputTest, ThePointerAloneLeavesTheFocusWhereItIs)
  {
    ShowGrid();
    ASSERT_TRUE(_ui->Focus("a"));

    PointAt(270, 195);
    Frame();

    EXPECT_EQ(_ui->GetFocused(), "a");
  }

  // the focus

  TEST_F(UiInputTest, WhatIsShownDuringPlayDoesNotTakeTheFocus)
  {
    ShowGrid();
    Frame();

    EXPECT_EQ(_ui->GetFocused(), "");
  }

  TEST_F(UiInputTest, AnElementThatAsksForTheFocusHasIt)
  {
    ShowGrid("", "autofocus: true\n");
    Frame();

    EXPECT_EQ(_ui->GetFocused(), "e");
    EXPECT_TRUE(Element("e").GetStates().focus);
    EXPECT_FALSE(Element("a").GetStates().focus);
  }

  TEST_F(UiInputTest, AFileThatIsModalGivesTheFocusToItsFirstButton)
  {
    ShowGrid("modal: true\n");
    Frame();

    EXPECT_EQ(_ui->GetFocused(), "a");
  }

  TEST_F(UiInputTest, TheGameMovesTheFocus)
  {
    ShowGrid();

    EXPECT_TRUE(_ui->Focus("f"));
    EXPECT_EQ(_ui->GetFocused(), "f");

    EXPECT_FALSE(_ui->Focus("root")) << "a panel cannot have the focus";
    EXPECT_FALSE(_ui->Focus("missing"));
    EXPECT_EQ(_ui->GetFocused(), "f");
  }

  TEST_F(UiInputTest, TheFirstKeyGivesTheFocusToTheFirstButton)
  {
    ShowGrid();

    Press(Action::Ui_Down);

    EXPECT_EQ(_ui->GetFocused(), "a");
  }

  struct Move
  {
    const char *from;
    Action action;
    const char *to;
  };

  void PrintTo(const Move &move, std::ostream *out)
  {
    *out << "from " << move.from << " by action " << static_cast<int>(move.action) << " to " << move.to;
  }

  class UiFocusTest : public UiInputTest, public ::testing::WithParamInterface<Move> {};

  TEST_P(UiFocusTest, MovesInTheDirectionOfTheKey)
  {
    ShowGrid();
    ASSERT_TRUE(_ui->Focus(GetParam().from));

    Press(GetParam().action);

    EXPECT_EQ(_ui->GetFocused(), GetParam().to);
  }

  INSTANTIATE_TEST_SUITE_P(EveryDirection, UiFocusTest, ::testing::Values(
    Move{"a", Action::Ui_Right, "b"},
    Move{"b", Action::Ui_Right, "c"},
    Move{"b", Action::Ui_Left, "a"},
    Move{"a", Action::Ui_Down, "d"},
    Move{"b", Action::Ui_Down, "e"},
    Move{"e", Action::Ui_Up, "b"},
    Move{"f", Action::Ui_Up, "c"},
    Move{"f", Action::Ui_Left, "e"},
    Move{"e", Action::Ui_Left, "d"},
    // at the edge the focus stays where it is
    Move{"c", Action::Ui_Right, "c"},
    Move{"a", Action::Ui_Left, "a"},
    Move{"a", Action::Ui_Up, "a"},
    Move{"e", Action::Ui_Down, "e"}));

  TEST_F(UiInputTest, MovesOnceForAKeyThatIsHeld)
  {
    ShowGrid();
    ASSERT_TRUE(_ui->Focus("a"));

    FrameWith({Action::Ui_Right});
    FrameWith({Action::Ui_Right});
    FrameWith({Action::Ui_Right});

    EXPECT_EQ(_ui->GetFocused(), "b");

    FrameWith({});
    FrameWith({Action::Ui_Right});

    EXPECT_EQ(_ui->GetFocused(), "c");
  }

  TEST_F(UiInputTest, SkipsWhatCannotBeUsed)
  {
    ShowGrid("", "enabled: false\n");
    ASSERT_TRUE(_ui->Focus("d"));

    Press(Action::Ui_Right);

    EXPECT_EQ(_ui->GetFocused(), "f");
  }

  TEST_F(UiInputTest, SkipsWhatIsHidden)
  {
    ShowGrid("", "hidden: true\n");
    ASSERT_TRUE(_ui->Focus("b"));

    Press(Action::Ui_Down);

    // nothing is straight below, and the focus goes to what is nearest
    EXPECT_NE(_ui->GetFocused(), "e");
    EXPECT_NE(_ui->GetFocused(), "b");
  }

  TEST_F(UiInputTest, PrefersWhatLiesStraightAhead)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: button\n  name: from\n  position: absolute\n  left: 0\n  top: 100\n  width: 50\n  height: 50\n"
      "- type: button\n  name: near\n  position: absolute\n  left: 120\n  top: 0\n  width: 50\n  height: 50\n"
      "- type: button\n  name: ahead\n  position: absolute\n  left: 200\n  top: 100\n  width: 50\n  height: 50\n"),
      0) << _logger->Messages(LogLevel::Error);

    ASSERT_TRUE(_ui->Focus("from"));
    Press(Action::Ui_Right);

    EXPECT_EQ(_ui->GetFocused(), "ahead");
  }

  TEST_F(UiInputTest, AFocusThatCanNoLongerBeUsedIsGivenUp)
  {
    ShowGrid("", "enabled: \"{can}\"\n");
    _ui->SetFlag("can", true);
    ASSERT_TRUE(_ui->Focus("e"));
    Frame();
    ASSERT_EQ(_ui->GetFocused(), "e");

    _ui->SetFlag("can", false);
    Frame();

    EXPECT_EQ(_ui->GetFocused(), "");
  }

  TEST_F(UiInputTest, AcceptChoosesWhatHasTheFocus)
  {
    ShowGrid();
    ASSERT_TRUE(_ui->Focus("e"));

    FrameWith({Action::Ui_Accept});

    EXPECT_TRUE(_ui->WasClicked("e"));
    EXPECT_TRUE(Element("e").GetStates().active);
  }

  TEST_F(UiInputTest, AcceptChoosesOnceWhileItIsHeld)
  {
    ShowGrid();
    ASSERT_TRUE(_ui->Focus("e"));

    int clicks = 0;
    _ui->OnClick("e", [&clicks] { clicks++; });

    FrameWith({Action::Ui_Accept});
    FrameWith({Action::Ui_Accept});
    FrameWith({Action::Ui_Accept});
    FrameWith({});

    EXPECT_EQ(clicks, 1);
    EXPECT_FALSE(Element("e").GetStates().active);
  }

  TEST_F(UiInputTest, AcceptChoosesNothingWithoutAFocus)
  {
    ShowGrid();

    Press(Action::Ui_Accept);

    EXPECT_TRUE(_ui->GetEvents().empty());
  }

  // what is left for the game

  TEST_F(UiInputTest, WithoutAUserInterfaceTheGameSeesEverything)
  {
    PointAt(150, 125);
    _input.state.SetAction(Action::Pointer_Primary);
    _input.state.SetKeyDown(neon::Key::W);
    _input.state.SetAction(Action::Ui_Accept);
    _input.state.SetAxisMotion(Axis::Mouse, 3, 4);
    Frame();

    EXPECT_TRUE(GameInput()[Action::Pointer_Primary]);
    EXPECT_TRUE(GameInput().IsKeyDown(neon::Key::W));
    EXPECT_TRUE(GameInput()[Action::Ui_Accept]);
    EXPECT_TRUE(GameInput()[Action::Mouse]);
    EXPECT_EQ(GameInput()[Axis::Mouse].x, 3);
    EXPECT_TRUE(GameInput().HasPointer());
    EXPECT_EQ(GameInput().GetPointer().x, 150);
  }

  TEST_F(UiInputTest, TheGameSeesAClickThatHitsNothing)
  {
    ShowGrid();

    PointAt(50, 50);
    FrameWith({Action::Pointer_Primary}, {neon::Key::W});

    EXPECT_TRUE(GameInput()[Action::Pointer_Primary]);
    EXPECT_TRUE(GameInput().HasPointer());
    EXPECT_TRUE(GameInput().IsKeyDown(neon::Key::W));
  }

  TEST_F(UiInputTest, TheGameDoesNotSeeAClickOnAButton)
  {
    ShowGrid();

    PointAt(150, 125);
    FrameWith({Action::Pointer_Primary}, {neon::Key::W});

    EXPECT_FALSE(GameInput()[Action::Pointer_Primary]);
    EXPECT_FALSE(GameInput().HasPointer());

    // what the user interface has no use for is left alone
    EXPECT_TRUE(GameInput().IsKeyDown(neon::Key::W));

    // and the input itself is as it was
    EXPECT_TRUE(_input.state[Action::Pointer_Primary]);
  }

  TEST_F(UiInputTest, TheGameDoesNotSeeTheEndOfAClickThatBeganOnAButton)
  {
    ShowGrid();

    PointAt(150, 125);
    FrameWith({Action::Pointer_Primary});

    PointAt(50, 50);
    FrameWith({Action::Pointer_Primary});

    EXPECT_FALSE(GameInput()[Action::Pointer_Primary]);
  }

  TEST_F(UiInputTest, TheGameDoesNotSeeThePointerOverAPanelThatTakesIt)
  {
    ASSERT_GE(ShowUnderRoot(
      "- type: panel\n"
      "  width: 100\n"
      "  height: 100\n"
      "  pointer_events: auto\n"), 0);

    PointAt(50, 50);
    FrameWith({Action::Pointer_Primary});
    EXPECT_FALSE(GameInput()[Action::Pointer_Primary]);

    PointAt(150, 50);
    FrameWith({Action::Pointer_Primary});
    EXPECT_TRUE(GameInput()[Action::Pointer_Primary]);
  }

  TEST_F(UiInputTest, TheGameSeesTheKeysOfTheUserInterfaceWhileNothingHasTheFocus)
  {
    ShowGrid();

    // the key that gives the focus is seen by the game as well
    FrameWith({});
    _input.state.SetAction(Action::Ui_Cancel);
    Frame();

    EXPECT_TRUE(GameInput()[Action::Ui_Cancel]);
  }

  TEST_F(UiInputTest, TheGameDoesNotSeeTheKeysOfTheUserInterfaceWhileSomethingHasTheFocus)
  {
    ShowGrid();
    ASSERT_TRUE(_ui->Focus("a"));

    FrameWith(
      {Action::Ui_Up, Action::Ui_Right, Action::Ui_Down, Action::Ui_Left, Action::Ui_Accept, Action::Ui_Cancel},
      {neon::Key::W, neon::Key::A});

    for (const Action action : {
           Action::Ui_Up, Action::Ui_Right, Action::Ui_Down, Action::Ui_Left, Action::Ui_Accept,
           Action::Ui_Cancel
         })
    {
      EXPECT_FALSE(GameInput()[action]) << static_cast<int>(action);
    }

    EXPECT_TRUE(GameInput().IsKeyDown(neon::Key::W));
    EXPECT_TRUE(GameInput().IsKeyDown(neon::Key::A));
  }

  TEST_F(UiInputTest, TheGameSeesNothingWhileAFileIsModal)
  {
    ShowGrid("modal: true\n");

    PointAt(50, 50);
    _input.state.SetAxisMotion(Axis::Mouse, 3, 4);
    for (std::size_t action = 0; action < neon::kAction_Size; action++)
    {
      _input.state.SetAction(static_cast<Action>(action));
    }
    Frame();

    for (std::size_t action = 0; action < neon::kAction_Size; action++)
    {
      EXPECT_FALSE(GameInput()[static_cast<Action>(action)]) << action;
    }
    EXPECT_FALSE(GameInput().HasPointer());
  }

  TEST_F(UiInputTest, TheGameSeesEverythingAgainOnceTheFileIsGone)
  {
    ASSERT_GE(Show("modal: true\nroot:\n  type: button\n  name: resume\n  text: a\n"), 0);
    const int document = 0;

    FrameWith({}, {neon::Key::W});
    ASSERT_FALSE(GameInput().IsKeyDown(neon::Key::W));

    _ui->Unload(document);
    FrameWith({Action::Ui_Accept}, {neon::Key::W});

    EXPECT_TRUE(GameInput().IsKeyDown(neon::Key::W));
    EXPECT_TRUE(GameInput()[Action::Ui_Accept]);
  }

  // files on top of each other

  TEST_F(UiInputTest, AFileThatIsModalTakesTheInputFromTheFilesBelowIt)
  {
    ShowGrid("", "autofocus: true\n");

    ASSERT_GE(Show(
      "ui: pause\n"
      "modal: true\n"
      "root:\n"
      "  type: button\n"
      "  name: resume\n"
      "  text: Resume\n"
      "  position: absolute\n"
      "  left: 800\n"
      "  top: 800\n"
      "  width: 100\n"
      "  height: 50\n", "ui/pause.ui.yml"), 0) << _logger->Messages(LogLevel::Error);

    EXPECT_EQ(_ui->GetFocused(), "resume");

    PointAt(150, 125);
    Frame();
    EXPECT_FALSE(Element("a").GetStates().hover);

    ClickAt(150, 125);
    EXPECT_FALSE(_ui->WasClicked("a"));

    // the focus does not leave the file that is modal
    Press(Action::Ui_Up);
    Press(Action::Ui_Left);
    EXPECT_EQ(_ui->GetFocused(), "resume");

    EXPECT_FALSE(_ui->Focus("missing"));
  }

  TEST_F(UiInputTest, WithoutAModalFileEveryFileTakesInput)
  {
    ShowGrid();
    ASSERT_GE(Show(
      "ui: over\n"
      "root:\n"
      "  type: button\n"
      "  name: over\n"
      "  text: a\n"
      "  position: absolute\n"
      "  left: 100\n"
      "  top: 300\n"
      "  width: 100\n"
      "  height: 50\n", "ui/over.ui.yml"), 0) << _logger->Messages(LogLevel::Error);

    ClickAt(150, 125);
    EXPECT_TRUE(_ui->WasClicked("a"));

    ClickAt(150, 325);
    EXPECT_TRUE(_ui->WasClicked("over"));

    // the focus moves from one file into the other
    ASSERT_TRUE(_ui->Focus("d"));
    Press(Action::Ui_Down);
    EXPECT_EQ(_ui->GetFocused(), "over");
  }

  TEST_F(UiInputTest, TheFileOnTopIsAskedFirst)
  {
    ShowGrid();
    ASSERT_GE(Show(
      "root:\n"
      "  type: button\n"
      "  name: cover\n"
      "  text: a\n"
      "  position: absolute\n"
      "  left: 100\n"
      "  top: 100\n"
      "  width: 100\n"
      "  height: 50\n", "ui/cover.ui.yml"), 0);

    ClickAt(150, 125);

    EXPECT_TRUE(_ui->WasClicked("cover"));
    EXPECT_FALSE(_ui->WasClicked("a"));
  }

  // the cursor

  TEST_F(UiInputTest, TheCursorIsHiddenWhenTheGameAsksForIt)
  {
    EXPECT_CALL(_input, CenterAndHideCursor()).Times(1);
    EXPECT_CALL(_input, ShowCursor()).Times(0);

    _ui->GetGameInput()->CenterAndHideCursor();
    _ui->GetGameInput()->CenterAndHideCursor();

    ShowGrid();
    Frame();
    Frame();
  }

  TEST_F(UiInputTest, TheCursorIsShownWhileAFileIsModal)
  {
    {
      ::testing::InSequence in_order;
      EXPECT_CALL(_input, CenterAndHideCursor());
      EXPECT_CALL(_input, ShowCursor());
      EXPECT_CALL(_input, CenterAndHideCursor());
    }

    // a game that turns the view with the mouse
    _ui->GetGameInput()->CenterAndHideCursor();
    Frame();

    const int menu = Show("modal: true\nroot:\n  type: button\n  name: resume\n  text: a\n");
    ASSERT_GE(menu, 0);
    Frame();
    Frame();

    _ui->Unload(menu);
    Frame();
    Frame();
  }

  TEST_F(UiInputTest, TheCursorStaysHiddenWhileAModalFileIsUsedWithAController)
  {
    EXPECT_CALL(_input, CenterAndHideCursor()).Times(1);
    EXPECT_CALL(_input, ShowCursor()).Times(0);

    _ui->GetGameInput()->CenterAndHideCursor();
    _input.state.SetDevice(neon::InputDevice::Gamepad);
    Frame();

    const int menu = Show("modal: true\nroot:\n  type: button\n  name: resume\n  text: a\n");
    ASSERT_GE(menu, 0);
    Frame();
    Frame();
  }

  TEST_F(UiInputTest, TheCursorComesOutWhenTheMouseIsTakenUpAgainInAModalFile)
  {
    {
      ::testing::InSequence in_order;
      EXPECT_CALL(_input, CenterAndHideCursor());
      EXPECT_CALL(_input, ShowCursor());
    }

    _ui->GetGameInput()->CenterAndHideCursor();
    _input.state.SetDevice(neon::InputDevice::Gamepad);
    const int menu = Show("modal: true\nroot:\n  type: button\n  name: resume\n  text: a\n");
    ASSERT_GE(menu, 0);
    Frame();

    _input.state.SetDevice(neon::InputDevice::KeyboardAndMouse);
    Frame();
  }

  TEST_F(UiInputTest, SaysWhichDeviceThePlayerUsedLast)
  {
    Frame();
    EXPECT_EQ(_ui->GetValue("input_device", nullptr), "keyboard");
    EXPECT_EQ(_ui->GetValue("gamepad", nullptr), "false");

    _input.state.SetDevice(neon::InputDevice::Gamepad);
    Frame();
    EXPECT_EQ(_ui->GetValue("input_device", nullptr), "gamepad");
    EXPECT_EQ(_ui->GetValue("gamepad", nullptr), "true");
  }

  TEST_F(UiInputTest, TheCursorStaysShownForAGameThatNeverHidIt)
  {
    EXPECT_CALL(_input, CenterAndHideCursor()).Times(0);
    EXPECT_CALL(_input, ShowCursor()).Times(0);

    const int menu = Show("modal: true\nroot:\n  type: button\n  name: resume\n  text: a\n");
    Frame();
    _ui->Unload(menu);
    Frame();
  }

  TEST_F(UiInputTest, WhatTheGameAsksForWhileAFileIsModalHoldsAfterwards)
  {
    {
      ::testing::InSequence in_order;
      EXPECT_CALL(_input, CenterAndHideCursor());
    }
    EXPECT_CALL(_input, ShowCursor()).Times(0);

    const int menu = Show("modal: true\nroot:\n  type: button\n  name: resume\n  text: a\n");
    Frame();

    _ui->GetGameInput()->CenterAndHideCursor();
    Frame();

    _ui->Unload(menu);
    Frame();
  }
}
