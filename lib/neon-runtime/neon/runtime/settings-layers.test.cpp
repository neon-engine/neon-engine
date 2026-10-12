#include "settings-layers.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/ryml-document-format.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/testing/recording-logging-context.hpp>

namespace
{
  using neon::RYML_DocumentFormat;
  using neon::SettingsLayers;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using neon::testing::RecordingLoggingContext;
  using ::testing::IsEmpty;
  using ::testing::Not;

  class SettingsLayersTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    RYML_DocumentFormat _yaml;
    RecordingLoggingContext _logging;
    SettingsLayers _layers{&_files, &_yaml, &_logging, "NeonEditor"};

    SettingsConfig _settings;
    std::vector<std::string> _errors;

    void SetUp() override
    {
      _files.Initialize();
      _files.PlaceUserDirectory("maker", "game");
    }

    bool ReadFiles()
    {
      return _layers.ReadFiles(_settings, _errors);
    }
  };

  TEST_F(SettingsLayersTest, ReadsNothingWhenThereAreNoFiles)
  {
    EXPECT_TRUE(ReadFiles());
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_settings.width, 1280);
  }

  TEST_F(SettingsLayersTest, ReadsThePlayersFileOverTheProjects)
  {
    _files.AddNativeFile("/assets/settings.yml", "version: 1\nwindow:\n  width: 1920\n  height: 1080\n");
    _files.AddNativeFile("/user/settings.yml", "version: 1\nwindow:\n  width: 800\n");

    ASSERT_TRUE(ReadFiles()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_settings.width, 800);
    EXPECT_EQ(_settings.height, 1080);
  }

  TEST_F(SettingsLayersTest, StopsOnAMistakeInTheProjectsFile)
  {
    _files.AddNativeFile("/assets/settings.yml", "version: 1\nwindow:\n  width: wide\n");

    EXPECT_FALSE(ReadFiles());
    EXPECT_THAT(_errors, Not(IsEmpty()));
  }

  TEST_F(SettingsLayersTest, LeavesOutThePlayersFileWithAMistakeAndWarns)
  {
    _files.AddNativeFile("/assets/settings.yml", "version: 1\nwindow:\n  width: 1920\n");
    _files.AddNativeFile("/user/settings.yml", "version: 1\nwindow:\n  width: wide\n");

    EXPECT_TRUE(ReadFiles());
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_EQ(_settings.width, 1920);
    const auto logger = _logging.Of("NeonEditor");
    EXPECT_TRUE(logger->Contains(LogLevel::Warn, "The settings of the player are left out until they are corrected"));
    EXPECT_GE(logger->Count(LogLevel::Warn), 2u);
  }
}
