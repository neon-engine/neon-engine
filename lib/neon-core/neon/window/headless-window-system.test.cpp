#include "headless-window-system.hpp"

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::Headless_WindowSystem;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;
  using ::testing::IsEmpty;

  class HeadlessWindowSystemTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    SettingsConfig _settings{.width = 640, .height = 360};
  };

  TEST_F(HeadlessWindowSystemTest, RunsFromTheStart)
  {
    const Headless_WindowSystem window_system(_settings, _logger);

    EXPECT_TRUE(window_system.IsRunning());
  }

  TEST_F(HeadlessWindowSystemTest, KeepsRunningThroughInitializeAndUpdate)
  {
    Headless_WindowSystem window_system(_settings, _logger);

    window_system.Initialize();
    for (int frame = 0; frame < 100; frame++) { window_system.Update(); }

    EXPECT_TRUE(window_system.IsRunning());
  }

  TEST_F(HeadlessWindowSystemTest, StopsRunningWhenToldToClose)
  {
    Headless_WindowSystem window_system(_settings, _logger);
    window_system.Initialize();

    window_system.SignalToClose();

    EXPECT_FALSE(window_system.IsRunning());
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Headless window system signaled to close"));
  }

  TEST_F(HeadlessWindowSystemTest, StaysClosedWhenToldToCloseTwice)
  {
    Headless_WindowSystem window_system(_settings, _logger);

    window_system.SignalToClose();
    window_system.SignalToClose();
    window_system.Update();

    EXPECT_FALSE(window_system.IsRunning());
  }

  TEST_F(HeadlessWindowSystemTest, DrawsAtTheSizeOfTheSettings)
  {
    Headless_WindowSystem window_system(_settings, _logger);

    const auto [width, height] = window_system.GetDrawableSize();

    EXPECT_EQ(width, 640);
    EXPECT_EQ(height, 360);
  }

  TEST_F(HeadlessWindowSystemTest, DrawsAtTheSizeOfTheSettingsInEveryWindowMode)
  {
    for (const WindowMode mode : {WindowMode::Windowed, WindowMode::Borderless, WindowMode::Fullscreen})
    {
      _settings.window_mode = mode;
      Headless_WindowSystem window_system(_settings, _logger);

      const auto [width, height] = window_system.GetDrawableSize();

      EXPECT_EQ(width, 640);
      EXPECT_EQ(height, 360);
    }
  }

  TEST_F(HeadlessWindowSystemTest, SaysTheSizeItRendersAt)
  {
    Headless_WindowSystem window_system(_settings, _logger);

    window_system.Initialize();

    EXPECT_TRUE(
      _logger->Contains(LogLevel::Info, "Initializing headless window system, rendering at 640x360"));
  }

  TEST_F(HeadlessWindowSystemTest, AdvancesByASixtiethOfASecondWithoutATimeStep)
  {
    Headless_WindowSystem window_system(_settings, _logger);

    EXPECT_DOUBLE_EQ(Headless_WindowSystem::frame_time, 1.0 / 60.0);
    EXPECT_DOUBLE_EQ(window_system.GetDeltaTime(), 1.0 / 60.0);
  }

  TEST_F(HeadlessWindowSystemTest, AdvancesByTheTimeStepOfTheSettings)
  {
    _settings.time_step = 0.05;
    Headless_WindowSystem window_system(_settings, _logger);

    EXPECT_DOUBLE_EQ(window_system.GetDeltaTime(), 0.05);
  }

  TEST_F(HeadlessWindowSystemTest, AdvancesByTheSameAmountInEveryFrame)
  {
    Headless_WindowSystem window_system(_settings, _logger);
    window_system.Initialize();

    const double first = window_system.GetDeltaTime();
    for (int frame = 0; frame < 10; frame++)
    {
      window_system.Update();
      EXPECT_EQ(window_system.GetDeltaTime(), first);
    }
  }

  TEST_F(HeadlessWindowSystemTest, FallsBackToASixtiethOfASecondForATimeStepBelowZero)
  {
    _settings.time_step = -1.0;
    Headless_WindowSystem window_system(_settings, _logger);

    EXPECT_DOUBLE_EQ(window_system.GetDeltaTime(), 1.0 / 60.0);
  }

  TEST_F(HeadlessWindowSystemTest, NeedsNoVulkanExtensions)
  {
    Headless_WindowSystem window_system(_settings, _logger);

    EXPECT_THAT(window_system.GetVulkanInstanceExtensions(), IsEmpty());
  }

  TEST_F(HeadlessWindowSystemTest, CreatesNoVulkanSurface)
  {
    Headless_WindowSystem window_system(_settings, _logger);
    int instance = 0;
    void *surface = &instance;

    EXPECT_FALSE(window_system.CreateVulkanSurface(&instance, &surface));
    EXPECT_EQ(surface, &instance);
    EXPECT_FALSE(window_system.CreateVulkanSurface(nullptr, nullptr));
  }

  TEST_F(HeadlessWindowSystemTest, AcceptsTheCallsForTheCursorAndTheFocus)
  {
    Headless_WindowSystem window_system(_settings, _logger);
    window_system.Initialize();

    window_system.CenterCursor();
    window_system.SetWindowFocus(true);
    window_system.SetWindowFocus(false);

    EXPECT_TRUE(window_system.IsRunning());
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 0u);
  }

  TEST_F(HeadlessWindowSystemTest, CleanUpLeavesItAsItIs)
  {
    Headless_WindowSystem window_system(_settings, _logger);
    window_system.Initialize();

    window_system.CleanUp();
    window_system.CleanUp();

    EXPECT_TRUE(window_system.IsRunning());
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Cleaning up headless window system"));
  }

  TEST_F(HeadlessWindowSystemTest, KeepsTheSettingsItWasCreatedWith)
  {
    Headless_WindowSystem window_system(_settings, _logger);

    _settings.width = 1;
    _settings.time_step = 9.0;

    EXPECT_EQ(window_system.GetDrawableSize().width, 640);
    EXPECT_DOUBLE_EQ(window_system.GetDeltaTime(), 1.0 / 60.0);
  }
}
