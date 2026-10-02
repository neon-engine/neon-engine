#include "input-state.hpp"

#include <memory>

#include <gtest/gtest.h>

#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::Action;
  using neon::Axis;
  using neon::InputState;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;

  constexpr Action every_action[] = {
    Action::Mouse,
    Action::Ui_Up,
    Action::Ui_Right,
    Action::Ui_Down,
    Action::Ui_Left,
    Action::Ui_Accept,
    Action::Ui_Cancel,
    Action::Pointer_Primary
  };

  class InputStateTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    InputState _state{_logger};
  };

  TEST_F(InputStateTest, StartsWithNothingPressedOrMoved)
  {
    for (const Action action : every_action) { EXPECT_FALSE(_state[action]); }

    EXPECT_EQ(_state[Axis::Mouse].x, 0.0);
    EXPECT_EQ(_state[Axis::Mouse].y, 0.0);
  }

  TEST_F(InputStateTest, StartsWithTheKeyboardAndTheMouseAsTheDevice)
  {
    EXPECT_EQ(_state.GetDevice(), neon::InputDevice::KeyboardAndMouse);
  }

  TEST_F(InputStateTest, KeepsTheDeviceThatWasUsedLastAcrossAReset)
  {
    _state.SetDevice(neon::InputDevice::Gamepad);
    _state.Reset();

    EXPECT_EQ(_state.GetDevice(), neon::InputDevice::Gamepad);
  }

  TEST_F(InputStateTest, KnowsEveryAction)
  {
    EXPECT_EQ(neon::kAction_Size, std::size(every_action));
  }

  TEST_F(InputStateTest, SetsTheActionThatWasGivenAndNoOther)
  {
    for (const Action set : every_action)
    {
      InputState state(_logger);
      state.SetAction(set);

      for (const Action action : every_action) { EXPECT_EQ(state[action], action == set); }
    }
  }

  TEST_F(InputStateTest, KeepsSeveralActionsAtOnce)
  {
    _state.SetAction(Action::Ui_Up);
    _state.SetAction(Action::Ui_Left);

    EXPECT_TRUE(_state[Action::Ui_Up]);
    EXPECT_TRUE(_state[Action::Ui_Left]);
    EXPECT_FALSE(_state[Action::Ui_Down]);
    EXPECT_FALSE(_state[Action::Ui_Right]);
  }

  TEST_F(InputStateTest, KeepsAnActionThatIsSetTwice)
  {
    _state.SetAction(Action::Ui_Up);
    _state.SetAction(Action::Ui_Up);

    EXPECT_TRUE(_state[Action::Ui_Up]);
  }

  TEST_F(InputStateTest, ResetReleasesEveryAction)
  {
    for (const Action action : every_action) { _state.SetAction(action); }

    _state.Reset();

    for (const Action action : every_action) { EXPECT_FALSE(_state[action]); }
  }

  TEST_F(InputStateTest, MouseMotionIsKeptAndSetsTheMouseAction)
  {
    _state.SetAxisMotion(Axis::Mouse, 12.5, -3.0);

    EXPECT_TRUE(_state[Action::Mouse]);
    EXPECT_EQ(_state[Axis::Mouse].x, 12.5);
    EXPECT_EQ(_state[Axis::Mouse].y, -3.0);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);
  }

  TEST_F(InputStateTest, MouseMotionReplacesTheOneBefore)
  {
    _state.SetAxisMotion(Axis::Mouse, 12.5, -3.0);
    _state.SetAxisMotion(Axis::Mouse, 1.0, 2.0);

    EXPECT_EQ(_state[Axis::Mouse].x, 1.0);
    EXPECT_EQ(_state[Axis::Mouse].y, 2.0);
  }

  TEST_F(InputStateTest, ResetReleasesTheMouseActionAndKeepsTheLastMotion)
  {
    _state.SetAxisMotion(Axis::Mouse, 12.5, -3.0);

    _state.Reset();

    // the motion is only read while the action is set
    EXPECT_FALSE(_state[Action::Mouse]);
    EXPECT_EQ(_state[Axis::Mouse].x, 12.5);
    EXPECT_EQ(_state[Axis::Mouse].y, -3.0);
  }

  TEST_F(InputStateTest, ReportsAnAxisItDoesNotKnow)
  {
    _state.SetAxisMotion(Axis::COUNT, 1.0, 2.0);

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "unsupported axis passed in"));
    EXPECT_FALSE(_state[Action::Mouse]);
    EXPECT_EQ(_state[Axis::Mouse].x, 0.0);
    EXPECT_EQ(_state[Axis::Mouse].y, 0.0);
  }

  TEST_F(InputStateTest, IsCopiedWithWhatItHolds)
  {
    _state.SetAction(Action::Ui_Accept);
    _state.SetAxisMotion(Axis::Mouse, 4.0, 5.0);
    _state.SetPointer(640.0, 360.0);

    const InputState copy = _state;
    _state.Reset();
    _state.ClearPointer();

    EXPECT_TRUE(copy[Action::Ui_Accept]);
    EXPECT_TRUE(copy[Action::Mouse]);
    EXPECT_EQ(copy[Axis::Mouse].x, 4.0);
    EXPECT_TRUE(copy.HasPointer());
    EXPECT_EQ(copy.GetPointer().x, 640.0);
  }

  TEST_F(InputStateTest, ClearActionReleasesThatActionAndNoOther)
  {
    for (const Action cleared : every_action)
    {
      InputState state(_logger);
      for (const Action action : every_action) { state.SetAction(action); }

      state.ClearAction(cleared);

      for (const Action action : every_action) { EXPECT_EQ(state[action], action != cleared); }
    }
  }

  TEST_F(InputStateTest, ClearActionLeavesAnActionThatIsNotSetAlone)
  {
    _state.ClearAction(Action::Ui_Accept);

    for (const Action action : every_action) { EXPECT_FALSE(_state[action]); }
  }

  TEST_F(InputStateTest, StartsWithoutAPointer)
  {
    EXPECT_FALSE(_state.HasPointer());
    EXPECT_EQ(_state.GetPointer().x, 0.0);
    EXPECT_EQ(_state.GetPointer().y, 0.0);
  }

  TEST_F(InputStateTest, KeepsWhereThePointerIs)
  {
    _state.SetPointer(640.5, 360.25);

    EXPECT_TRUE(_state.HasPointer());
    EXPECT_EQ(_state.GetPointer().x, 640.5);
    EXPECT_EQ(_state.GetPointer().y, 360.25);

    // the pointer is no action, and the motion of the mouse is not its place
    for (const Action action : every_action) { EXPECT_FALSE(_state[action]); }
    EXPECT_EQ(_state[Axis::Mouse].x, 0.0);
  }

  TEST_F(InputStateTest, ResetKeepsThePointerWhereItIs)
  {
    _state.SetPointer(640.0, 360.0);
    _state.SetAction(Action::Pointer_Primary);

    _state.Reset();

    EXPECT_FALSE(_state[Action::Pointer_Primary]);
    EXPECT_TRUE(_state.HasPointer());
    EXPECT_EQ(_state.GetPointer().x, 640.0);
    EXPECT_EQ(_state.GetPointer().y, 360.0);
  }

  TEST_F(InputStateTest, ClearPointerTakesThePointerAway)
  {
    _state.SetPointer(640.0, 360.0);

    _state.ClearPointer();

    EXPECT_FALSE(_state.HasPointer());
  }
}

// Keys, text, the wheel, and the right stick.

namespace
{
  using neon::Key;
  using neon::KeyEvent;
  using neon::TextComposition;

  TEST_F(InputStateTest, StartsWithoutKeysTextAndWheel)
  {
    EXPECT_TRUE(_state.GetKeyEvents().empty());
    EXPECT_TRUE(_state.GetText().empty());
    EXPECT_TRUE(_state.GetComposition().text.empty());
    EXPECT_EQ(_state.GetWheel().x, 0.0);
    EXPECT_EQ(_state.GetWheel().y, 0.0);
    EXPECT_FALSE(_state.IsWheelPrecise());
    EXPECT_EQ(_state.GetRightStick().x, 0.0);
  }

  TEST_F(InputStateTest, KeepsTheKeysInTheOrderTheyWentDown)
  {
    _state.AddKeyEvent({Key::Left, true, false, {.shift = true}});
    _state.AddKeyEvent({Key::Left, false, false, {}});
    _state.AddKeyEvent({Key::A, true, true, {.shortcut = true}});

    ASSERT_EQ(_state.GetKeyEvents().size(), 3u);
    EXPECT_EQ(_state.GetKeyEvents()[0].key, Key::Left);
    EXPECT_TRUE(_state.GetKeyEvents()[0].modifiers.shift);
    EXPECT_FALSE(_state.GetKeyEvents()[1].is_down);
    EXPECT_TRUE(_state.GetKeyEvents()[2].is_repeat);
  }

  TEST_F(InputStateTest, JoinsTheTextOfAFrame)
  {
    _state.AddText("Zo");
    _state.AddText("\xC3\xAB");
    EXPECT_EQ(_state.GetText(), "Zo\xC3\xAB");
  }

  TEST_F(InputStateTest, AddsUpTheWheelOfAFrame)
  {
    _state.AddWheel(0.0, 1.0, false);
    _state.AddWheel(0.5, 2.0, true);

    EXPECT_EQ(_state.GetWheel().x, 0.5);
    EXPECT_EQ(_state.GetWheel().y, 3.0);
    EXPECT_TRUE(_state.IsWheelPrecise());
  }

  TEST_F(InputStateTest, ForgetsWhatHappenedInTheFrameWhenItIsReset)
  {
    _state.AddKeyEvent({Key::Enter, true, false, {}});
    _state.AddText("a");
    _state.AddWheel(1.0, 1.0, true);
    _state.SetRightStick(0.5, -0.5);
    _state.SetComposition({"ni", 2, 0});

    _state.Reset();

    EXPECT_TRUE(_state.GetKeyEvents().empty());
    EXPECT_TRUE(_state.GetText().empty());
    EXPECT_EQ(_state.GetWheel().y, 0.0);
    EXPECT_FALSE(_state.IsWheelPrecise());
    EXPECT_EQ(_state.GetRightStick().x, 0.0);

    // an input method goes on from frame to frame
    EXPECT_EQ(_state.GetComposition().text, "ni");
  }

  TEST_F(InputStateTest, TakesTheKeyboardAwayAndLeavesWhatElseHoldsAnAction)
  {
    _state.SetKeyboardAction(Action::Ui_Up);
    _state.SetKeyboardAction(Action::Ui_Accept);
    _state.SetAction(Action::Ui_Accept);
    _state.SetAction(Action::Pointer_Primary);
    _state.AddKeyEvent({Key::A, true, false, {}});
    _state.AddText("w");

    EXPECT_TRUE(_state[Action::Ui_Up]);

    _state.ClearKeyboard();

    EXPECT_FALSE(_state[Action::Ui_Up]);
    EXPECT_TRUE(_state[Action::Ui_Accept]);
    EXPECT_TRUE(_state[Action::Pointer_Primary]);
    EXPECT_TRUE(_state.GetKeyEvents().empty());
    EXPECT_TRUE(_state.GetText().empty());
  }

  TEST_F(InputStateTest, ReleasesAnActionWhateverHoldsIt)
  {
    _state.SetKeyboardAction(Action::Ui_Up);
    _state.SetAction(Action::Ui_Up);
    _state.ClearAction(Action::Ui_Up);
    EXPECT_FALSE(_state[Action::Ui_Up]);

    _state.ClearKeyboard();
    EXPECT_FALSE(_state[Action::Ui_Up]);
  }

  TEST(KeyTest, HasANameForEveryKeyAndFindsItAgain)
  {
    for (int key = 1; key < static_cast<int>(Key::COUNT); key++)
    {
      const std::string name = neon::NameOf(static_cast<Key>(key));
      EXPECT_NE(name, "unknown") << key;
      EXPECT_EQ(neon::KeyOf(name), static_cast<Key>(key)) << name;
    }

    EXPECT_EQ(neon::KeyOf("f13"), Key::Unknown);
    EXPECT_EQ(neon::KeyOf(""), Key::Unknown);
  }
} // namespace
