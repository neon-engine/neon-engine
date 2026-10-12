#include "project-reading.hpp"

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
  using neon::Project;
  using neon::ProjectReading;
  using neon::RYML_DocumentFormat;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using neon::testing::RecordingLoggingContext;
  using ::testing::HasSubstr;
  using ::testing::IsEmpty;

  const std::string project_text =
    "version: 1\n"
    "name: under-development\n"
    "organization: maker\n"
    "scenes:\n"
    "  - assets://scenes/level.scene.yml\n"
    "entry_scene: assets://scenes/level.scene.yml\n";

  const std::string game_text =
    "version: 1\n"
    "name: built-game\n"
    "organization: maker\n"
    "scenes:\n"
    "  - assets://scenes/level.scene.yml\n"
    "entry_scene: assets://scenes/level.scene.yml\n";

  class ProjectReadingTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    RYML_DocumentFormat _yaml;
    RecordingLoggingContext _logging;
    ProjectReading _reading{&_files, &_yaml, &_logging, "NeonRuntime"};

    Project _project;
    std::string _file;
    bool _none = false;
    std::vector<std::string> _errors;

    void SetUp() override
    {
      _files.Initialize();
    }

    bool Read()
    {
      return _reading.Read(_project, _file, _none, _errors);
    }
  };

  TEST_F(ProjectReadingTest, ReadsTheProjectOfAProjectUnderDevelopment)
  {
    _files.AddNativeFile("/assets/project.yml", project_text);

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_project.name, "under-development");
    EXPECT_EQ(_file, "assets://project.yml");
    EXPECT_FALSE(_none);
  }

  TEST_F(ProjectReadingTest, ReadsTheGameOfABuiltGameFirst)
  {
    _files.AddNativeFile("/assets/project.yml", project_text);
    _files.AddNativeFile("/assets/game.yml", game_text);

    ASSERT_TRUE(Read()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_project.name, "built-game");
    EXPECT_EQ(_file, "assets://game.yml");
    EXPECT_TRUE(_logging.Of("NeonRuntime")->Contains(LogLevel::Info, "the game is what runs"));
  }

  TEST_F(ProjectReadingTest, SaysNothingAboutTheGameWhenThereIsOnlyAProject)
  {
    _files.AddNativeFile("/assets/project.yml", project_text);

    ASSERT_TRUE(Read());
    EXPECT_THAT(_logging.Of("NeonRuntime")->Entries(), IsEmpty());
  }

  TEST_F(ProjectReadingTest, FailsAndSaysThatThereIsNoneWhereNeitherFileIs)
  {
    EXPECT_FALSE(Read());
    EXPECT_TRUE(_none);
    EXPECT_EQ(_file, "assets://project.yml");
    ASSERT_EQ(_errors.size(), 1u);
    EXPECT_THAT(_errors[0], HasSubstr("assets://project.yml: there is no project here, the file cannot be read"));
  }

  TEST_F(ProjectReadingTest, FailsWithTheErrorsOfAProjectThatIsThereAndWrong)
  {
    _files.AddNativeFile("/assets/project.yml", "version: 1\n");

    EXPECT_FALSE(Read());
    EXPECT_FALSE(_none);
    EXPECT_THAT(_errors, ::testing::Not(IsEmpty()));
  }
}
