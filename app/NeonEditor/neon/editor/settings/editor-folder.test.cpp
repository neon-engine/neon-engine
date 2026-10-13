#include "editor-folder.hpp"

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/ryml-document-format.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::EditorFolder;
  using neon::RYML_DocumentFormat;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;

  class EditorFolderTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    RYML_DocumentFormat _yaml;
    EditorFolder _folder{&_files, &_yaml, _logger};

    void SetUp() override
    {
      _files.Initialize();
    }
  };

  TEST(EditorFolder, IsPlacedByTheEditorsOwnNamesAndHoldsTwoFiles)
  {
    EXPECT_EQ(EditorFolder::organization, "neon-engine");
    EXPECT_EQ(EditorFolder::application, "neon-editor");
    EXPECT_EQ(EditorFolder::settings_path, "editor://editor-settings.yml");
    EXPECT_EQ(EditorFolder::layout_path, "editor://layout.ini");
  }

  TEST_F(EditorFolderTest, StartsWithTheDefaultsWhenThereIsNoFileYetAndSaysSo)
  {
    EXPECT_TRUE(_folder.ReadSettings());

    EXPECT_EQ(_folder.GetSettings().window_width, 1600);
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "There is no editor://editor-settings.yml yet"));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);
  }

  TEST_F(EditorFolderTest, ReadsTheSettingsAndWritesThemBack)
  {
    _files.AddNativeFile("/editor/editor-settings.yml", "version: 1\nwindow:\n  width: 1280\n  height: 720\n");

    EXPECT_TRUE(_folder.ReadSettings());
    EXPECT_EQ(_folder.GetSettings().window_width, 1280);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Info,
      "The editor's settings were read from editor://editor-settings.yml: a window of 1280 by 720 points, windowed"));

    _folder.GetSettings().open_windows = {"Log"};
    EXPECT_TRUE(_folder.WriteSettings());
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "The editor's settings were written to editor://editor-settings.yml"));

    std::string written;
    ASSERT_TRUE(_files.ReadText("editor://editor-settings.yml", written));
    EXPECT_THAT(written, ::testing::HasSubstr("open_windows:\n  - Log"));
    EXPECT_THAT(written, ::testing::HasSubstr("width: 1280"));
  }

  TEST_F(EditorFolderTest, RefusesAFileWithAMistakeWithItsLinesAndLeavesItAlone)
  {
    const std::string mistaken = "version: 1\nwindow:\n  width: 1280\n  colour: red\n";
    _files.AddNativeFile("/editor/editor-settings.yml", mistaken);

    EXPECT_FALSE(_folder.ReadSettings());

    EXPECT_EQ(_folder.GetSettings().window_width, 1600);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "editor://editor-settings.yml:4: ")) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "editor://editor-settings.yml is refused, the editor starts with its defaults"));

    // and is not written back, so that the mistake can be corrected
    EXPECT_FALSE(_folder.WriteSettings());
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "was refused when it was read, and is not written back"));
    std::string kept;
    ASSERT_TRUE(_files.ReadText("editor://editor-settings.yml", kept));
    EXPECT_EQ(kept, mistaken);
  }

  TEST_F(EditorFolderTest, ReadsAndWritesTheLayoutOfTheWindows)
  {
    std::string layout = "untouched";
    EXPECT_FALSE(_folder.ReadLayout(layout));
    EXPECT_EQ(layout, "untouched");
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "There is no editor://layout.ini yet"));

    EXPECT_TRUE(_folder.WriteLayout("[Window][Log]\nPos=0,0\n"));
    EXPECT_TRUE(_folder.ReadLayout(layout));
    EXPECT_EQ(layout, "[Window][Log]\nPos=0,0\n");
    EXPECT_TRUE(_files.HasNativeFile("/editor/layout.ini"));
  }
}
