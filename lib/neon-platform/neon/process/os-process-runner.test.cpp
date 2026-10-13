// The processes of the operating system, as the editor starts a build: a
// program is found, started, read, and its exit code reported; and as it
// opens the code in another program, which is started without waiting. The
// programs are those every machine has: on macOS and Linux the shell, on
// Windows the command interpreter.

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/recording-logger.hpp>
#include <neon/testing/temporary-directory.hpp>

#include "os-process-runner.hpp"

namespace
{
  using neon::OS_ProcessRunner;
  using neon::ProcessCommand;
  using neon::testing::RecordingLogger;
  using ::testing::ElementsAre;
  using ::testing::EndsWith;
  using ::testing::HasSubstr;
  using ::testing::IsEmpty;

  class OSProcessRunnerTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    OS_ProcessRunner _runner{_logger};
    std::vector<std::string> _lines;
    int _exit_code = -1;
    std::string _error;

    /// A command that runs `script` in the shell of the platform.
    static ProcessCommand Shell(const std::string &script)
    {
#if defined(_WIN32)
      return {.program = "cmd", .arguments = {"/c", script}};
#else
      return {.program = "sh", .arguments = {"-c", script}};
#endif
    }

    bool Run(const ProcessCommand &command)
    {
      return _runner.Run(command, [this](const std::string &line) { _lines.push_back(line); }, _exit_code, _error);
    }
  };

  TEST_F(OSProcessRunnerTest, FindsTheShellOnPath)
  {
    std::string path;
#if defined(_WIN32)
    ASSERT_TRUE(_runner.Find("cmd", path));
    EXPECT_THAT(path, EndsWith("cmd.exe"));
#else
    ASSERT_TRUE(_runner.Find("sh", path));
    EXPECT_THAT(path, EndsWith("/sh"));
#endif
  }

  TEST_F(OSProcessRunnerTest, FindsAProgramByItsPath)
  {
    std::string on_path;
    ASSERT_TRUE(_runner.Find(Shell("").program, on_path));

    std::string path;
    ASSERT_TRUE(_runner.Find(on_path, path));
    EXPECT_EQ(path, on_path);
  }

  TEST_F(OSProcessRunnerTest, FindsNothingForAProgramThatIsNotThere)
  {
    std::string path = "unchanged";
    EXPECT_FALSE(_runner.Find("neon-engine-no-such-program", path));
    EXPECT_FALSE(_runner.Find("/neon-engine/no/such/program", path));
    EXPECT_EQ(path, "unchanged");
  }

  TEST_F(OSProcessRunnerTest, ReadsTheLinesAProgramPrintsInOrder)
  {
#if defined(_WIN32)
    ASSERT_TRUE(Run(Shell("echo one & echo two 1>&2 & echo three"))) << _error;
    EXPECT_THAT(_lines, ElementsAre("one", "two ", "three"));
#else
    ASSERT_TRUE(Run(Shell("echo one; echo two >&2; echo three"))) << _error;
    EXPECT_THAT(_lines, ElementsAre("one", "two", "three"));
#endif
    EXPECT_EQ(_exit_code, 0);
  }

  TEST_F(OSProcessRunnerTest, ReportsTheExitCode)
  {
    ASSERT_TRUE(Run(Shell("exit 3"))) << _error;
    EXPECT_EQ(_exit_code, 3);
    EXPECT_THAT(_lines, IsEmpty());
  }

  TEST_F(OSProcessRunnerTest, HandsAnArgumentOverAsItIs)
  {
#if defined(_WIN32)
    ASSERT_TRUE(Run({.program = "cmd", .arguments = {"/c", "echo", "two words"}})) << _error;
#else
    ASSERT_TRUE(Run({.program = "echo", .arguments = {"two words"}})) << _error;
#endif
    EXPECT_THAT(_lines, ElementsAre(HasSubstr("two words")));
  }

  TEST_F(OSProcessRunnerTest, KeepsALastLineWithoutALineBreak)
  {
#if defined(_WIN32)
    ASSERT_TRUE(Run(Shell("<nul set /p =last"))) << _error;
#else
    ASSERT_TRUE(Run(Shell("printf last"))) << _error;
#endif
    EXPECT_THAT(_lines, ElementsAre("last"));
  }

  TEST_F(OSProcessRunnerTest, StartsInTheFolderItIsGiven)
  {
#if defined(_WIN32)
    ProcessCommand command = Shell("cd");
    command.directory = "C:\\";
    ASSERT_TRUE(Run(command)) << _error;
    EXPECT_THAT(_lines, ElementsAre("C:\\"));
#else
    ProcessCommand command = Shell("pwd");
    command.directory = "/";
    ASSERT_TRUE(Run(command)) << _error;
    EXPECT_THAT(_lines, ElementsAre("/"));
#endif
  }

  TEST_F(OSProcessRunnerTest, RefusesAProgramThatIsNotThere)
  {
    EXPECT_FALSE(Run({.program = "neon-engine-no-such-program"}));
    EXPECT_THAT(_error, HasSubstr("neon-engine-no-such-program is not installed"));
    EXPECT_THAT(_lines, IsEmpty());
  }

  // starting without waiting

  TEST_F(OSProcessRunnerTest, StartsAProgramAndReturnsBeforeItEnds)
  {
    const auto before = std::chrono::steady_clock::now();
#if defined(_WIN32)
    ASSERT_TRUE(_runner.Start(Shell("ping -n 4 127.0.0.1 > nul"), _error)) << _error;
#else
    ASSERT_TRUE(_runner.Start(Shell("sleep 3"), _error)) << _error;
#endif
    const auto taken = std::chrono::steady_clock::now() - before;
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(taken).count(), 2000);
  }

  TEST_F(OSProcessRunnerTest, AStartedProgramRunsInTheFolderItIsGiven)
  {
    const neon::testing::TemporaryDirectory directory;
    ProcessCommand command = Shell("echo started > started.txt");
    command.directory = directory.Native();

    ASSERT_TRUE(_runner.Start(command, _error)) << _error;

    // the program runs by itself, so the file turns up a moment later
    const std::filesystem::path file = directory.Path() / "started.txt";
    for (int waited = 0; waited < 500 && !std::filesystem::exists(file); waited++)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(std::filesystem::exists(file));
  }

  TEST_F(OSProcessRunnerTest, RefusesToStartAProgramThatIsNotThere)
  {
    EXPECT_FALSE(_runner.Start({.program = "neon-engine-no-such-program"}, _error));
    EXPECT_THAT(_error, HasSubstr("neon-engine-no-such-program is not installed"));
  }
} // namespace
