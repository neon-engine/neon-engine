#include "toolchain-files.hpp"

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/ryml-document-format.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::RYML_DocumentFormat;
  using neon::ToolchainFile;
  using neon::ToolchainFiles;
  using neon::Toolchains;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using ::testing::SizeIs;

  class ToolchainFilesTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    RYML_DocumentFormat _yaml;
    ToolchainFile _file{&_files, &_yaml, _logger};
    ToolchainFiles _reading{&_file, &_files, _logger};
    Toolchains _toolchains;

    void SetUp() override
    {
      _files.Initialize();
    }

    /// Writes `editor.yml` of this machine.
    void Write(const std::string &text)
    {
      ASSERT_TRUE(_files.WriteText(std::string(ToolchainFiles::shared_path), text));
    }

    /// Writes `user-editor.yml` of this machine.
    void WriteUser(const std::string &text)
    {
      ASSERT_TRUE(_files.WriteText(std::string(ToolchainFiles::user_path), text));
    }
  };

  TEST_F(ToolchainFilesTest, TheFilesAreTheMachinesNotAProjects)
  {
    EXPECT_EQ(ToolchainFiles::shared_path, "editor://editor.yml");
    EXPECT_EQ(ToolchainFiles::user_path, "editor://user-editor.yml");
  }

  TEST_F(ToolchainFilesTest, WithoutAFileTheToolchainsAreTheEnginesDefaults)
  {
    EXPECT_TRUE(_reading.Read(_toolchains));

    EXPECT_THAT(_toolchains.All(), SizeIs(3));
    EXPECT_NE(_toolchains.Find("macos-arm64"), nullptr);
    EXPECT_NE(_toolchains.Find("linux-x64"), nullptr);
    EXPECT_NE(_toolchains.Find("windows-x64"), nullptr);
    EXPECT_EQ(_toolchains.Editor("vscode"), "");
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(ToolchainFilesTest, TheFileIsLaidOverTheDefaults)
  {
    Write(
      "version: 1\n"
      "toolchains:\n"
      "  windows-x64: off\n"
      "  mine:\n"
      "    kind: native\n"
      "    platform: darwin-arm64\n"
      "    configure: cmake -S {project} -B {build}\n"
      "    build: cmake --build {build}\n"
      "editors:\n"
      "  vscode: /opt/code/bin/code\n");

    EXPECT_TRUE(_reading.Read(_toolchains));

    EXPECT_THAT(_toolchains.All(), SizeIs(3));
    EXPECT_NE(_toolchains.Find("macos-arm64"), nullptr);
    EXPECT_EQ(_toolchains.Find("windows-x64"), nullptr);
    EXPECT_NE(_toolchains.Find("mine"), nullptr);
    EXPECT_EQ(_toolchains.Editor("vscode"), "/opt/code/bin/code");
  }

  TEST_F(ToolchainFilesTest, TheUsersFileIsLaidOverTheShared)
  {
    Write("version: 1\neditors:\n  vscode: shared\n");
    WriteUser("version: 1\neditors:\n  vscode: of-the-user\n");

    EXPECT_TRUE(_reading.Read(_toolchains));

    EXPECT_EQ(_toolchains.Editor("vscode"), "of-the-user");
  }

  TEST_F(ToolchainFilesTest, ReadingAgainStartsOverFromTheDefaults)
  {
    Write("version: 1\neditors:\n  vscode: code\n");
    ASSERT_TRUE(_reading.Read(_toolchains));
    ASSERT_TRUE(_files.WriteText(std::string(ToolchainFiles::shared_path), "version: 1\n"));

    EXPECT_TRUE(_reading.Read(_toolchains));

    EXPECT_EQ(_toolchains.Editor("vscode"), "");
  }

  TEST_F(ToolchainFilesTest, AFileWithAMistakeAddsNothingAndIsSaidWithItsLine)
  {
    Write("version: 1\neditors:\n  vscode: code\n  clion: [a]\n");
    WriteUser("version: 1\neditors:\n  vscode: of-the-user\n");

    EXPECT_FALSE(_reading.Read(_toolchains));

    // the good file is read all the same
    EXPECT_THAT(_toolchains.All(), SizeIs(3));
    EXPECT_EQ(_toolchains.Editor("vscode"), "of-the-user");
    EXPECT_EQ(_toolchains.Editor("clion"), "");
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "editor://editor.yml:4")) << _logger->Messages(LogLevel::Error);
  }
} // namespace
