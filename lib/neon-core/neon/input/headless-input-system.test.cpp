#include "headless-input-system.hpp"

#include <memory>

#include <gtest/gtest.h>

#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::Action;
  using neon::Axis;
  using neon::Headless_InputSystem;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;

  class HeadlessInputSystemTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    Headless_InputSystem _input_system{SettingsConfig{}, _logger};

    void ExpectNothingPressedOrMoved()
    {
      const auto &state = _input_system.GetInputState();

      for (std::size_t action = 0; action < neon::kAction_Size; action++)
      {
        EXPECT_FALSE(state[static_cast<Action>(action)]) << "action " << action;
      }
      EXPECT_EQ(state[Axis::Mouse].x, 0.0);
      EXPECT_EQ(state[Axis::Mouse].y, 0.0);
    }
  };

  TEST_F(HeadlessInputSystemTest, HasNothingPressedBeforeItIsInitialized)
  {
    ExpectNothingPressedOrMoved();
  }

  TEST_F(HeadlessInputSystemTest, HasNothingPressedInAnyFrame)
  {
    _input_system.Initialize();

    for (int frame = 0; frame < 3; frame++)
    {
      _input_system.ProcessInput();
      ExpectNothingPressedOrMoved();
    }

    _input_system.CleanUp();
  }

  TEST_F(HeadlessInputSystemTest, HandsOutTheSameStateEveryTime)
  {
    EXPECT_EQ(&_input_system.GetInputState(), &_input_system.GetInputState());
  }

  TEST_F(HeadlessInputSystemTest, AcceptsTheCallsForTheCursor)
  {
    _input_system.Initialize();

    _input_system.CenterAndHideCursor();
    _input_system.ShowCursor();

    ExpectNothingPressedOrMoved();
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 0u);
  }

  TEST_F(HeadlessInputSystemTest, SaysWhenItStartsAndStops)
  {
    _input_system.Initialize();
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Initializing headless input system"));

    _input_system.CleanUp();
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Cleaning up headless input system"));
  }

  TEST_F(HeadlessInputSystemTest, CanBeCleanedUpTwiceAndStartedAgain)
  {
    _input_system.Initialize();
    _input_system.CleanUp();
    _input_system.CleanUp();
    _input_system.Initialize();
    _input_system.ProcessInput();

    ExpectNothingPressedOrMoved();
  }

  TEST_F(HeadlessInputSystemTest, IsAnInputSystemLikeAnyOther)
  {
    neon::InputSystem &input_system = _input_system;
    input_system.Initialize();
    input_system.ProcessInput();

    neon::InputContext &input_context = _input_system;
    EXPECT_FALSE(input_context.GetInputState()[Action::L_Up]);
  }
}
