#include "deferred-file-sink.hpp"

#include <chrono>
#include <string>
#include <string_view>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/temporary-directory.hpp>

namespace
{
  using neon::DeferredFileSink;
  using neon::testing::TemporaryDirectory;

  /// Each test writes its file into a folder of its own. The sink is told
  /// to write only the text of a message, so that lines can be compared
  /// whole.
  class DeferredFileSinkTest : public ::testing::Test
  {
  protected:
    TemporaryDirectory _directory;
    DeferredFileSink _sink{1048576, 1};

    void SetUp() override
    {
      _sink.set_pattern("%v");
    }

    void Log(const std::string &text)
    {
      _sink.log(spdlog::details::log_msg(std::string_view("Test"), spdlog::level::info, text));
    }

    void Open()
    {
      _sink.Open(_directory.Native("logs/engine.log"));
    }

    /// What the file holds so far.
    [[nodiscard]] std::string LogFile()
    {
      _sink.flush();
      return _directory.Read("logs/engine.log");
    }
  };

  TEST_F(DeferredFileSinkTest, WritesNoFileBeforeItIsOpened)
  {
    Log("held back");

    EXPECT_FALSE(_sink.IsOpen());
    EXPECT_FALSE(_directory.HasDirectory("logs"));
  }

  TEST_F(DeferredFileSinkTest, WritesWhatItHeldWhenItIsOpenedAndThenWhatFollows)
  {
    Log("first");
    Log("second");
    Open();
    Log("third");

    EXPECT_TRUE(_sink.IsOpen());
    EXPECT_EQ(LogFile(), "first\nsecond\nthird\n");
  }

  TEST_F(DeferredFileSinkTest, KeepsTheTimeAMessageWasLoggedAt)
  {
    _sink.set_pattern("%Y-%m-%d %v");

    // noon, so that the date is the same in every time zone of a test machine
    using namespace std::chrono;
    spdlog::details::log_msg message(std::string_view("Test"), spdlog::level::info, "long ago");
    message.time = sys_days{year{2001} / 2 / 3} + hours{12};
    _sink.log(message);

    Open();

    EXPECT_EQ(LogFile(), "2001-02-03 long ago\n");
  }

  TEST_F(DeferredFileSinkTest, WritesHeldMessagesInThePatternItWasGivenLater)
  {
    Log("held back");
    _sink.set_pattern("[%n] %v");
    Open();
    Log("written at once");

    EXPECT_EQ(LogFile(), "[Test] held back\n[Test] written at once\n");
  }

  TEST_F(DeferredFileSinkTest, HoldsNoMoreThanItIsToldAndSaysHowManyAreMissing)
  {
    DeferredFileSink sink(1048576, 1, 2);
    sink.set_pattern("%v");
    for (const auto *text : {"one", "two", "three", "four", "five"})
    {
      sink.log(spdlog::details::log_msg(std::string_view("Test"), spdlog::level::info, text));
    }

    sink.Open(_directory.Native("logs/engine.log"));
    sink.flush();

    EXPECT_EQ(
      _directory.Read("logs/engine.log"),
      "one\ntwo\n"
      "3 messages that were logged before the log file was opened are missing here. They went to the console only\n");
  }

  TEST_F(DeferredFileSinkTest, AddsToAFileThatExists)
  {
    _directory.Write("logs/engine.log", "what an earlier run wrote\n");

    Log("this run");
    Open();

    EXPECT_EQ(LogFile(), "what an earlier run wrote\nthis run\n");
  }

  TEST_F(DeferredFileSinkTest, LetsGoOfWhatItHeldWhenItGoesWithout)
  {
    Log("let go");
    _sink.GoWithout();
    Log("not held either");

    // a file that turns up after all only gets what follows
    Open();
    Log("written");

    EXPECT_EQ(LogFile(), "written\n");
  }

  TEST_F(DeferredFileSinkTest, ThrowsAndLetsGoOfWhatItHeldWhenTheFileCannotBeOpened)
  {
    // a file is where the folder of the log file would have to be
    _directory.Write("logs", "in the way");

    Log("let go");

    EXPECT_THROW(Open(), spdlog::spdlog_ex);
    EXPECT_FALSE(_sink.IsOpen());

    Log("not held either");
    _sink.Open(_directory.Native("elsewhere.log"));
    Log("written");
    _sink.flush();

    EXPECT_EQ(_directory.Read("elsewhere.log"), "written\n");
  }

  TEST_F(DeferredFileSinkTest, MovesToAnotherFileWithoutRepeatingWhatItHeld)
  {
    Log("first");
    Open();
    _sink.Open(_directory.Native("elsewhere.log"));
    Log("second");
    _sink.flush();

    EXPECT_EQ(_directory.Read("logs/engine.log"), "first\n");
    EXPECT_EQ(_directory.Read("elsewhere.log"), "second\n");
  }
}
