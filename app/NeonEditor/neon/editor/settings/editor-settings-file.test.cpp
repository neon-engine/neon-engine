#include "editor-settings-file.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/ryml-document-format.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::EditorSettings;
  using neon::EditorSettingsFile;
  using neon::RYML_DocumentFormat;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using ::testing::ElementsAre;
  using ::testing::HasSubstr;
  using ::testing::IsEmpty;
  using ::testing::Not;
  using ::testing::SizeIs;

  const std::string complete =
    "version: 1\n"
    "window:\n"
    "  width: 1920\n"
    "  height: 1080\n"
    "  mode: borderless\n"
    "last_project: /home/someone/projects/museum\n"
    "projects_folder: /home/someone/projects\n"
    "open_windows: [Log, Inspector]\n";

  class EditorSettingsFileTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    RYML_DocumentFormat _yaml;
    EditorSettingsFile _file{&_files, &_yaml, _logger};
    EditorSettings _settings;
    std::vector<std::string> _errors;

    void SetUp() override
    {
      _files.Initialize();
    }

    bool Read(const std::string &text)
    {
      return _file.ReadText("editor-settings.yml", text, _settings, _errors);
    }
  };

  TEST_F(EditorSettingsFileTest, StartsWithTheDefaultsOfAWindowedEditor)
  {
    EXPECT_EQ(_settings.window_width, 1600);
    EXPECT_EQ(_settings.window_height, 1000);
    EXPECT_EQ(_settings.window_mode, WindowMode::Windowed);
    EXPECT_EQ(_settings.last_project, "");
    EXPECT_EQ(_settings.projects_folder, "");
    EXPECT_THAT(_settings.open_windows, IsEmpty());
  }

  TEST_F(EditorSettingsFileTest, ReadsEverything)
  {
    ASSERT_TRUE(Read(complete)) << ::testing::PrintToString(_errors);

    EXPECT_EQ(_settings.window_width, 1920);
    EXPECT_EQ(_settings.window_height, 1080);
    EXPECT_EQ(_settings.window_mode, WindowMode::Borderless);
    EXPECT_EQ(_settings.last_project, "/home/someone/projects/museum");
    EXPECT_EQ(_settings.projects_folder, "/home/someone/projects");
    EXPECT_THAT(_settings.open_windows, ElementsAre("Log", "Inspector"));
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(EditorSettingsFileTest, WhatIsLeftOutKeepsItsDefault)
  {
    ASSERT_TRUE(Read("version: 1\nwindow:\n  width: 800\n")) << ::testing::PrintToString(_errors);

    EXPECT_EQ(_settings.window_width, 800);
    EXPECT_EQ(_settings.window_height, 1000);
    EXPECT_EQ(_settings.window_mode, WindowMode::Windowed);
    EXPECT_THAT(_settings.open_windows, IsEmpty());
  }

  TEST_F(EditorSettingsFileTest, WritesWhatItReadsAndReadsWhatItWrote)
  {
    ASSERT_TRUE(Read(complete)) << ::testing::PrintToString(_errors);

    const std::string written = _file.WriteText(_settings);
    EXPECT_THAT(written, HasSubstr("version: 1"));
    EXPECT_THAT(written, HasSubstr("mode: borderless"));
    EXPECT_THAT(written, HasSubstr("last_project: /home/someone/projects/museum"));

    EditorSettings again;
    ASSERT_TRUE(_file.ReadText("again", written, again, _errors)) << ::testing::PrintToString(_errors) << written;
    EXPECT_EQ(again.window_width, 1920);
    EXPECT_EQ(again.window_height, 1080);
    EXPECT_EQ(again.window_mode, WindowMode::Borderless);
    EXPECT_EQ(again.last_project, "/home/someone/projects/museum");
    EXPECT_EQ(again.projects_folder, "/home/someone/projects");
    EXPECT_THAT(again.open_windows, ElementsAre("Log", "Inspector"));
  }

  TEST_F(EditorSettingsFileTest, LeavesOutAFolderThatWasNeverChosen)
  {
    const std::string written = _file.WriteText(EditorSettings{});

    EXPECT_THAT(written, Not(HasSubstr("last_project")));
    EXPECT_THAT(written, Not(HasSubstr("projects_folder")));
    EXPECT_THAT(written, HasSubstr("width: 1600"));
    EXPECT_THAT(written, HasSubstr("open_windows: []"));
    EXPECT_THAT(written, Not(HasSubstr("- ")));
  }

  TEST_F(EditorSettingsFileTest, ReadsAndWritesThroughTheFileSystem)
  {
    EditorSettings settings;
    settings.window_width = 1234;
    settings.open_windows = {"Log"};
    ASSERT_TRUE(_file.Write("editor://editor-settings.yml", settings));

    ASSERT_TRUE(_file.Read("editor://editor-settings.yml", _settings, _errors)) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_settings.window_width, 1234);
    EXPECT_THAT(_settings.open_windows, ElementsAre("Log"));
  }

  TEST_F(EditorSettingsFileTest, SaysWhenTheFileCannotBeRead)
  {
    EXPECT_FALSE(_file.Read("editor://editor-settings.yml", _settings, _errors));
    ASSERT_THAT(_errors, SizeIs(1));
    EXPECT_THAT(_errors.front(), HasSubstr("editor://editor-settings.yml: the file cannot be read"));
  }

  // what is refused

  TEST_F(EditorSettingsFileTest, RefusesANameThatIsNotKnownWithItsLineAndChangesNothing)
  {
    EXPECT_FALSE(Read("version: 1\nwindow:\n  width: 800\n  colour: red\nlast_projekt: x\n"));

    ASSERT_THAT(_errors, SizeIs(2));
    EXPECT_THAT(_errors[0], HasSubstr("editor-settings.yml:4: "));
    EXPECT_THAT(_errors[0], HasSubstr("colour"));
    EXPECT_THAT(_errors[1], HasSubstr("editor-settings.yml:5: "));
    EXPECT_THAT(_errors[1], HasSubstr("last_projekt"));
    EXPECT_EQ(_settings.window_width, 1600);
  }

  TEST_F(EditorSettingsFileTest, RefusesASizeThatIsNotAboveZero)
  {
    EXPECT_FALSE(Read("version: 1\nwindow:\n  width: 0\n  height: -5\n"));

    ASSERT_THAT(_errors, SizeIs(2));
    EXPECT_THAT(_errors[0], HasSubstr(
                  "editor-settings.yml:3: 'width' of 'window' of the editor's settings is 0, where a whole number of "
                  "points above zero was expected"));
    EXPECT_THAT(_errors[1], HasSubstr("editor-settings.yml:4: 'height' of 'window' of the editor's settings is -5"));
  }

  TEST_F(EditorSettingsFileTest, RefusesAModeItDoesNotKnow)
  {
    EXPECT_FALSE(Read("version: 1\nwindow:\n  mode: maximised\n"));

    ASSERT_THAT(_errors, SizeIs(1));
    EXPECT_THAT(_errors[0], HasSubstr("editor-settings.yml:3: "));
    EXPECT_THAT(_errors[0], HasSubstr("maximised"));
    EXPECT_EQ(_settings.window_mode, WindowMode::Windowed);
  }

  TEST_F(EditorSettingsFileTest, RefusesAMissingVersionAndALaterOne)
  {
    EXPECT_FALSE(Read("window:\n  width: 800\n"));
    ASSERT_THAT(_errors, SizeIs(1));
    EXPECT_THAT(_errors[0], HasSubstr("'version' is missing"));

    _errors.clear();
    EXPECT_FALSE(Read("version: 2\n"));
    ASSERT_THAT(_errors, SizeIs(1));
    EXPECT_THAT(_errors[0], HasSubstr("editor-settings.yml:1: the file has version 2, and this editor reads up to version 1"));
  }

  TEST_F(EditorSettingsFileTest, RefusesTextThatIsNoDocument)
  {
    EXPECT_FALSE(Read("version: [1\n"));
    ASSERT_THAT(_errors, SizeIs(1));
    EXPECT_THAT(_errors[0], HasSubstr("editor-settings.yml"));
  }

  TEST_F(EditorSettingsFileTest, RefusesAnEmptyTitleAmongTheOpenWindows)
  {
    EXPECT_FALSE(Read("version: 1\nopen_windows: [Log, '']\n"));
    ASSERT_THAT(_errors, SizeIs(1));
    EXPECT_THAT(_errors[0], HasSubstr("editor-settings.yml:2: 'open_windows' holds an empty title"));
  }
}
