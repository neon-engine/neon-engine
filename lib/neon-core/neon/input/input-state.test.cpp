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
    Action::Mouse
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

    const InputState copy = _state;
    _state.Reset();

    EXPECT_TRUE(copy[Action::R_Up]);
    EXPECT_TRUE(copy[Action::Mouse]);
    EXPECT_EQ(copy[Axis::Mouse].x, 4.0);
  }
}
