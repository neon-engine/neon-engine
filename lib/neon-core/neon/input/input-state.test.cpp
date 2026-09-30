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
    Action::L_Up,
    Action::L_Right,
    Action::L_Down,
    Action::L_Left,
    Action::R_Up,
    Action::R_Right,
    Action::R_Down,
    Action::R_Left,
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
    _state.SetAction(Action::L_Up);
    _state.SetAction(Action::L_Left);

    EXPECT_TRUE(_state[Action::L_Up]);
    EXPECT_TRUE(_state[Action::L_Left]);
    EXPECT_FALSE(_state[Action::L_Down]);
    EXPECT_FALSE(_state[Action::L_Right]);
  }

  TEST_F(InputStateTest, KeepsAnActionThatIsSetTwice)
  {
    _state.SetAction(Action::L_Up);
    _state.SetAction(Action::L_Up);

    EXPECT_TRUE(_state[Action::L_Up]);
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
    _state.SetAction(Action::R_Up);
    _state.SetAxisMotion(Axis::Mouse, 4.0, 5.0);
    _state.SetPointer(640.0, 360.0);

    const InputState copy = _state;
    _state.Reset();
    _state.ClearPointer();

    EXPECT_TRUE(copy[Action::R_Up]);
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
