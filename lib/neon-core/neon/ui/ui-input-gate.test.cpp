#include "ui-input-gate.hpp"

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/mock-input-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::Action;
  using neon::Axis;
  using neon::UiConsumed;
  using neon::UiInputGate;
  using neon::testing::FakeInputContext;
  using neon::testing::RecordingLogger;
  using ::testing::InSequence;
  using ::testing::StrictMock;

  class UiInputGateTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    StrictMock<FakeInputContext> _input{_logger};
    UiInputGate _gate{&_input, _logger};

    void HoldEverything()
    {
      for (std::size_t action = 0; action < neon::kAction_Size; action++)
      {
        _input.state.SetAction(static_cast<Action>(action));
      }
      _input.state.SetAxisMotion(Axis::Mouse, 3, 4);
      _input.state.SetPointer(640, 360);
    }
  };

  TEST_F(UiInputGateTest, HasNothingPressedUntilItTakesOverAFrame)
  {
    HoldEverything();

    for (std::size_t action = 0; action < neon::kAction_Size; action++)
    {
      EXPECT_FALSE(_gate.GetInputState()[static_cast<Action>(action)]);
    }
    EXPECT_FALSE(_gate.GetInputState().HasPointer());
  }

  TEST_F(UiInputGateTest, HandsOnWhatWasNotUsed)
  {
    HoldEverything();
    _gate.Refresh({});

    for (std::size_t action = 0; action < neon::kAction_Size; action++)
    {
      EXPECT_TRUE(_gate.GetInputState()[static_cast<Action>(action)]) << action;
    }

    EXPECT_EQ(_gate.GetInputState()[Axis::Mouse].x, 3);
    EXPECT_EQ(_gate.GetInputState()[Axis::Mouse].y, 4);
    EXPECT_TRUE(_gate.GetInputState().HasPointer());
    EXPECT_EQ(_gate.GetInputState().GetPointer().x, 640);
  }

  TEST_F(UiInputGateTest, TakesThePointerAwayWhenItWasUsed)
  {
    HoldEverything();
    _gate.Refresh({.pointer = true});

    const auto &state = _gate.GetInputState();
    EXPECT_FALSE(state[Action::Pointer_Primary]);
    EXPECT_FALSE(state.HasPointer());

    // turning the view and everything else is left
    EXPECT_TRUE(state[Action::Mouse]);
    EXPECT_TRUE(state[Action::L_Up]);
    EXPECT_TRUE(state[Action::Ui_Accept]);
  }

  TEST_F(UiInputGateTest, TakesTheKeysOfTheUserInterfaceAwayWhenTheyWereUsed)
  {
    HoldEverything();
    _gate.Refresh({.navigation = true});

    const auto &state = _gate.GetInputState();
    for (const Action action : {
           Action::Ui_Up, Action::Ui_Right, Action::Ui_Down, Action::Ui_Left, Action::Ui_Accept,
           Action::Ui_Cancel
         })
    {
      EXPECT_FALSE(state[action]) << static_cast<int>(action);
    }

    for (const Action action : {
           Action::L_Up, Action::L_Right, Action::L_Down, Action::L_Left, Action::R_Up, Action::R_Right,
           Action::R_Down, Action::R_Left, Action::Mouse, Action::Pointer_Primary
         })
    {
      EXPECT_TRUE(state[action]) << static_cast<int>(action);
    }
    EXPECT_TRUE(state.HasPointer());
  }

  TEST_F(UiInputGateTest, TakesEverythingAway)
  {
    HoldEverything();
    _gate.Refresh({.everything = true});

    for (std::size_t action = 0; action < neon::kAction_Size; action++)
    {
      EXPECT_FALSE(_gate.GetInputState()[static_cast<Action>(action)]) << action;
    }
    EXPECT_FALSE(_gate.GetInputState().HasPointer());
  }

  TEST_F(UiInputGateTest, LeavesTheInputItselfAsItIs)
  {
    HoldEverything();
    _gate.Refresh({.everything = true});

    EXPECT_TRUE(_input.state[Action::Pointer_Primary]);
    EXPECT_TRUE(_input.state[Action::L_Up]);
    EXPECT_TRUE(_input.state.HasPointer());
  }

  TEST_F(UiInputGateTest, FollowsTheInputFromFrameToFrame)
  {
    HoldEverything();
    _gate.Refresh({.everything = true});

    _input.state.Reset();
    _input.state.SetAction(Action::L_Left);
    _gate.Refresh({});

    EXPECT_TRUE(_gate.GetInputState()[Action::L_Left]);
    EXPECT_FALSE(_gate.GetInputState()[Action::L_Up]);
    EXPECT_TRUE(_gate.GetInputState().HasPointer());
  }

  TEST_F(UiInputGateTest, HandsOutTheSameStateEveryTime)
  {
    EXPECT_EQ(&_gate.GetInputState(), &_gate.GetInputState());
  }

  // the cursor

  TEST_F(UiInputGateTest, HidesTheCursorForTheGame)
  {
    EXPECT_CALL(_input, CenterAndHideCursor()).Times(1);

    _gate.CenterAndHideCursor();
  }

  TEST_F(UiInputGateTest, ShowsTheCursorAgainForTheGame)
  {
    InSequence in_order;
    EXPECT_CALL(_input, CenterAndHideCursor());
    EXPECT_CALL(_input, ShowCursor());

    _gate.CenterAndHideCursor();
    _gate.ShowCursor();
  }

  TEST_F(UiInputGateTest, AsksOnceForWhatIsAskedTwice)
  {
    EXPECT_CALL(_input, CenterAndHideCursor()).Times(1);

    _gate.CenterAndHideCursor();
    _gate.CenterAndHideCursor();
    _gate.SetNeedsPointer(false);
    _gate.SetNeedsPointer(false);
  }

  TEST_F(UiInputGateTest, DoesNotShowACursorThatIsShown)
  {
    _gate.ShowCursor();
    _gate.SetNeedsPointer(true);
    _gate.SetNeedsPointer(false);
  }

  TEST_F(UiInputGateTest, ShowsTheCursorWhileAUserInterfaceNeedsThePointer)
  {
    InSequence in_order;
    EXPECT_CALL(_input, CenterAndHideCursor());
    EXPECT_CALL(_input, ShowCursor());
    EXPECT_CALL(_input, CenterAndHideCursor());

    _gate.CenterAndHideCursor();
    _gate.SetNeedsPointer(true);
    _gate.SetNeedsPointer(false);
  }

  TEST_F(UiInputGateTest, KeepsWhatTheGameAskedForWhileTheCursorWasShown)
  {
    // nothing is asked of the input until the pointer is no longer needed
    _gate.SetNeedsPointer(true);
    _gate.CenterAndHideCursor();
    ::testing::Mock::VerifyAndClearExpectations(&_input);

    EXPECT_CALL(_input, CenterAndHideCursor());
    _gate.SetNeedsPointer(false);
  }

  TEST_F(UiInputGateTest, LeavesTheCursorShownWhenTheGameChangedItsMind)
  {
    InSequence in_order;
    EXPECT_CALL(_input, CenterAndHideCursor());
    EXPECT_CALL(_input, ShowCursor());

    _gate.CenterAndHideCursor();
    _gate.SetNeedsPointer(true);
    _gate.ShowCursor();
    _gate.SetNeedsPointer(false);
  }
}

// The keyboard while a text is typed, the wheel, and the right stick.

namespace
{
  using neon::Key;

  TEST_F(UiInputGateTest, TakesTheKeyboardAwayWhileATextIsTyped)
  {
    _input.state.SetKeyboardAction(Action::L_Up);
    _input.state.SetKeyboardAction(Action::Ui_Cancel);
    _input.state.SetAction(Action::L_Left);
    _input.state.AddKeyEvent({Key::Backspace, true, false, {}});
    _input.state.AddText("w");

    _gate.Refresh({.keyboard = true});

    const auto &state = _gate.GetInputState();
    EXPECT_FALSE(state[Action::L_Up]);
    EXPECT_FALSE(state[Action::Ui_Cancel]);
    EXPECT_TRUE(state.GetKeyEvents().empty());
    EXPECT_TRUE(state.GetText().empty());

    // a controller still moves
    EXPECT_TRUE(state[Action::L_Left]);
  }

  TEST_F(UiInputGateTest, HandsTheKeyboardOnWhenNoTextIsTyped)
  {
    _input.state.SetKeyboardAction(Action::L_Up);
    _input.state.AddKeyEvent({Key::A, true, false, {}});
    _input.state.AddText("a");

    _gate.Refresh({});

    EXPECT_TRUE(_gate.GetInputState()[Action::L_Up]);
    EXPECT_EQ(_gate.GetInputState().GetKeyEvents().size(), 1u);
    EXPECT_EQ(_gate.GetInputState().GetText(), "a");
  }

  TEST_F(UiInputGateTest, TakesTheWheelAwayWhenItWasUsed)
  {
    _input.state.AddWheel(0.0, 2.0, false);

    _gate.Refresh({});
    EXPECT_EQ(_gate.GetInputState().GetWheel().y, 2.0);

    _gate.Refresh({.wheel = true});
    EXPECT_EQ(_gate.GetInputState().GetWheel().y, 0.0);
  }

  TEST_F(UiInputGateTest, TakesTheRightStickAwayWhenItWasUsed)
  {
    _input.state.SetRightStick(0.0, 1.0);

    _gate.Refresh({});
    EXPECT_EQ(_gate.GetInputState().GetRightStick().y, 1.0);

    _gate.Refresh({.right_stick = true});
    EXPECT_EQ(_gate.GetInputState().GetRightStick().y, 0.0);
  }

  TEST_F(UiInputGateTest, TakesEverythingOfTheKeyboardAndTheWheelAwayAsWell)
  {
    _input.state.AddKeyEvent({Key::A, true, false, {}});
    _input.state.AddText("a");
    _input.state.AddWheel(1.0, 1.0, false);
    _input.state.SetComposition({"ni", 0, 0});

    _gate.Refresh({.everything = true});

    EXPECT_TRUE(_gate.GetInputState().GetKeyEvents().empty());
    EXPECT_TRUE(_gate.GetInputState().GetText().empty());
    EXPECT_EQ(_gate.GetInputState().GetWheel().y, 0.0);
    EXPECT_TRUE(_gate.GetInputState().GetComposition().text.empty());
  }
} // namespace
