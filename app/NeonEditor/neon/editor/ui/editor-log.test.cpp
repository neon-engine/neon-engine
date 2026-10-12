#include "editor-log.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/recording-tool-ui.hpp>

namespace
{
  using neon::EditorLog;
  using neon::LoggingSystem;
  using neon::testing::RecordingToolUi;
  using ::testing::Contains;
  using ::testing::HasSubstr;
  using ::testing::Not;

  /// The logging system prints to the console, which is why this test
  /// prints. It never gets a log file.
  class EditorLogTest : public ::testing::Test
  {
  protected:
    SettingsConfig _settings;
    LoggingSystem _logging{_settings};
    RecordingToolUi _ui;

    void SetUp() override
    {
      _logging.Initialize();
      _logging.GoWithoutLogFile();
    }
  };

  TEST_F(EditorLogTest, ShowsTheLinesThatWereLoggedAndScrollsToTheLast)
  {
    EditorLog log(&_logging);
    _logging.CreateLogger("Test")->Info("a line of the test");

    log.Draw(_ui);

    EXPECT_THAT(_ui.calls, Contains("begin-window Log"));
    EXPECT_THAT(_ui.calls, Contains(HasSubstr("a line of the test")));
    EXPECT_THAT(_ui.calls, Contains("scroll-to-bottom"));

    // nothing new: the lines are shown again, without scrolling
    _ui.calls.clear();
    log.Draw(_ui);
    EXPECT_THAT(_ui.calls, Contains(HasSubstr("a line of the test")));
    EXPECT_THAT(_ui.calls, Not(Contains("scroll-to-bottom")));
  }

  TEST_F(EditorLogTest, ACollapsedWindowShowsNothingAndIsEnded)
  {
    EditorLog log(&_logging);
    _ui.windows_open = false;

    log.Draw(_ui);

    EXPECT_THAT(_ui.calls, Contains("begin-window Log"));
    EXPECT_THAT(_ui.calls, Contains("end-window"));
    EXPECT_THAT(_ui.calls, Not(Contains(HasSubstr("text"))));
  }
}
