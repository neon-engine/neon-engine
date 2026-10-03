// The settings files, as an application reads them: SettingsFile with the
// document format for YAML, in layers on top of each other. Files are kept
// in memory, except for the settings of the runtime, which are read as they
// are in the repository.

#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/ryml-document-format.hpp>
#include <neon/settings/settings-file.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::ApiVersion;
  using neon::RYML_DocumentFormat;
  using neon::SettingsFile;
  using neon::SoundGroupSetting;
  using neon::Tonemapper;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using ::testing::HasSubstr;
  using ::testing::IsEmpty;

  const std::string complete =
    "version: 1\n"
    "\n"
    "window:\n"
    "  title: A Game\n"
    "  width: 1600\n"
    "  height: 900\n"
    "  mode: fullscreen\n"
    "\n"
    "ui:\n"
    "  scale: 1.5\n"
    "  start: assets://ui/hud.ui.yml\n"
    "  pause_menu: assets://ui/pause.ui.yml\n"
    "  settings_menu: assets://ui/settings.ui.yml\n"
    "\n"
    "world:\n"
    "  steps_per_second: 120\n"
    "  most_steps_per_frame: 4\n"
    "\n"
    "input:\n"
    "  gyro: true\n"
    "\n"
    "scripting:\n"
    "  jit: false\n"
    "\n"
    "rendering:\n"
    "  vulkan_version: \"1.2\"\n"
    "  max_light_sources: 64\n"
    "  max_render_objects: 2048\n"
    "  shadow_distance: 80\n"
    "  shadow_cascades: 2\n"
    "  tonemapper: aces\n"
    "  exposure: 1.5\n"
    "\n"
    "audio:\n"
    "  groups:\n"
    "    - radio\n"
    "    - name: crowd\n"
    "      volume: 0.5\n"
    "  volumes:\n"
    "    music: 0.6\n"
    "    ambience: 0.25\n"
    "    radio: 0.75\n";

  class SettingsFilesTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    RYML_DocumentFormat _yaml;
    SettingsFile _file{&_files, &_yaml, _logger};
    SettingsConfig _settings;
    bool _found = false;
    std::vector<std::string> _errors;

    void SetUp() override
    {
      _files.Initialize();
    }

    void WriteOfTheProject(const std::string &text)
    {
      _files.AddNativeFile("/assets/settings.yml", text);
    }

    void WriteOfThePlayer(const std::string &text)
    {
      _files.AddNativeFile("/user/settings.yml", text);
    }

    bool ReadOfTheProject()
    {
      return _file.Read(std::string(SettingsFile::of_the_project), _settings, _found, _errors);
    }

    bool ReadOfThePlayer()
    {
      return _file.Read(std::string(SettingsFile::of_the_player), _settings, _found, _errors);
    }

    /// Reads the project's file, and expects it to fail with one error that
    /// holds the text.
    void ExpectRefused(const std::string &text)
    {
      EXPECT_FALSE(ReadOfTheProject());
      EXPECT_TRUE(_found);
      ASSERT_EQ(_errors.size(), 1u) << ::testing::PrintToString(_errors);
      EXPECT_THAT(_errors.front(), HasSubstr(text));
    }

    /// The volume of a group of the settings, or -1 for a group that is
    /// not there.
    float VolumeOf(const std::string &group) const
    {
      for (const auto &setting : _settings.sound_groups)
      {
        if (setting.name == group) { return setting.volume; }
      }
      return -1.0f;
    }

    static std::string FileOfTheRuntime()
    {
      const std::ifstream file(NEON_RUNTIME_SETTINGS);
      EXPECT_TRUE(file.good()) << NEON_RUNTIME_SETTINGS;

      std::stringstream text;
      text << file.rdbuf();
      return text.str();
    }
  };

  // reading

  TEST_F(SettingsFilesTest, ReadsEverything)
  {
    WriteOfTheProject(complete);

    ASSERT_TRUE(ReadOfTheProject()) << ::testing::PrintToString(_errors);
    EXPECT_TRUE(_found);
    EXPECT_EQ(_settings.title, "A Game");
    EXPECT_EQ(_settings.width, 1600);
    EXPECT_EQ(_settings.height, 900);
    EXPECT_EQ(_settings.window_mode, WindowMode::Fullscreen);
    EXPECT_DOUBLE_EQ(_settings.ui_scale, 1.5);
    EXPECT_EQ(_settings.ui_path, "assets://ui/hud.ui.yml");
    EXPECT_EQ(_settings.pause_menu, "assets://ui/pause.ui.yml");
    EXPECT_EQ(_settings.settings_menu, "assets://ui/settings.ui.yml");
    EXPECT_DOUBLE_EQ(_settings.steps_per_second, 120.0);
    EXPECT_EQ(_settings.gyro_enabled, true);
    EXPECT_FALSE(_settings.script_jit);
    EXPECT_EQ(_settings.most_steps_per_frame, 4u);
    EXPECT_EQ(_settings.vulkan_version, (ApiVersion{1, 2}));
    EXPECT_EQ(_settings.max_light_sources, 64u);
    EXPECT_EQ(_settings.max_render_objects, 2048u);
    EXPECT_DOUBLE_EQ(_settings.shadow_distance, 80.0);
    EXPECT_EQ(_settings.shadow_cascades, 2u);
    EXPECT_EQ(_settings.tonemapper, Tonemapper::Aces);
    EXPECT_DOUBLE_EQ(_settings.exposure, 1.5);
    EXPECT_EQ(VolumeOf("music"), 0.6f);
    EXPECT_EQ(VolumeOf("effects"), 1.0f);
    EXPECT_EQ(VolumeOf("voices"), 1.0f);
    EXPECT_EQ(VolumeOf("ambience"), 0.25f);
    EXPECT_EQ(VolumeOf("radio"), 0.75f);
    EXPECT_EQ(VolumeOf("crowd"), 0.5f);
    EXPECT_EQ(_settings.sound_groups.size(), 6u);
    EXPECT_THAT(_errors, IsEmpty());
  }

  TEST_F(SettingsFilesTest, LeavesWhatIsNotWrittenAsItWas)
  {
    _settings.width = 640;
    _settings.pause_menu = "assets://ui/mine.ui.yml";
    WriteOfTheProject("version: 1\nwindow:\n  height: 360\n");

    ASSERT_TRUE(ReadOfTheProject()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_settings.width, 640);
    EXPECT_EQ(_settings.height, 360);
    EXPECT_EQ(_settings.pause_menu, "assets://ui/mine.ui.yml");
  }

  TEST_F(SettingsFilesTest, AFileThatIsNotThereChangesNothingAndIsNotAnError)
  {
    _settings.width = 640;

    EXPECT_TRUE(ReadOfTheProject());
    EXPECT_FALSE(_found);
    EXPECT_EQ(_settings.width, 640);
    EXPECT_THAT(_errors, IsEmpty());
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "There are no settings at assets://settings.yml"));
  }

  TEST_F(SettingsFilesTest, ThePlayersFileReadsOnTopOfTheProjects)
  {
    WriteOfTheProject("version: 1\nwindow:\n  width: 1920\n  height: 1080\n  mode: borderless\n");
    WriteOfThePlayer("version: 1\nwindow:\n  width: 1280\n  height: 720\n  mode: windowed\nui:\n  scale: 2\n");

    ASSERT_TRUE(ReadOfTheProject()) << ::testing::PrintToString(_errors);
    ASSERT_TRUE(ReadOfThePlayer()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_settings.width, 1280);
    EXPECT_EQ(_settings.height, 720);
    EXPECT_EQ(_settings.window_mode, WindowMode::Windowed);
    EXPECT_DOUBLE_EQ(_settings.ui_scale, 2.0);
  }

  TEST_F(SettingsFilesTest, AFileWithAMistakeChangesNothing)
  {
    _settings.width = 640;
    WriteOfTheProject("version: 1\nwindow:\n  width: 1920\n  height: 0\n");

    EXPECT_FALSE(ReadOfTheProject());
    EXPECT_EQ(_settings.width, 640);
  }

  // what is refused

  TEST_F(SettingsFilesTest, LeavesTheGyroToTheInputMapWhenTheFileDoesNotSay)
  {
    WriteOfTheProject("version: 1\nworld:\n  steps_per_second: 30\n");

    ASSERT_TRUE(ReadOfTheProject()) << ::testing::PrintToString(_errors);
    EXPECT_FALSE(_settings.gyro_enabled.has_value());

    // the player's file says, over the project's
    WriteOfThePlayer("version: 1\ninput:\n  gyro: false\n");

    ASSERT_TRUE(ReadOfThePlayer()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_settings.gyro_enabled, false);
  }

  TEST_F(SettingsFilesTest, RefusesAMissingVersion)
  {
    WriteOfTheProject("window:\n  width: 1920\n");

    ExpectRefused("'version' is missing. It holds the version of the layout, which is 1");
  }

  TEST_F(SettingsFilesTest, RefusesAVersionAboveItsOwn)
  {
    WriteOfTheProject("version: 2\n");

    ExpectRefused("assets://settings.yml:1: the settings have version 2, and this engine reads up to version 1");
  }

  TEST_F(SettingsFilesTest, RefusesASizeThatIsNotAboveZero)
  {
    WriteOfTheProject("version: 1\nwindow:\n  width: -5\n");

    ExpectRefused("assets://settings.yml:3: 'width' of 'window' of the settings is -5, "
                  "where a whole number above zero was expected");
  }

  TEST_F(SettingsFilesTest, RefusesAScaleThatIsNotAboveZero)
  {
    WriteOfTheProject("version: 1\nui:\n  scale: 0\n");

    ExpectRefused("'scale' of 'ui' of the settings is 0, where a number above zero was expected");
  }

  TEST_F(SettingsFilesTest, RefusesAModeItDoesNotKnow)
  {
    WriteOfTheProject("version: 1\nwindow:\n  mode: maximized\n");

    EXPECT_FALSE(ReadOfTheProject());
    ASSERT_EQ(_errors.size(), 1u) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors.front(), HasSubstr("'mode'"));
    EXPECT_THAT(_errors.front(), HasSubstr("windowed"));
  }

  TEST_F(SettingsFilesTest, RefusesAVulkanVersionThatIsNotInQuotes)
  {
    WriteOfTheProject("version: 1\nrendering:\n  vulkan_version: 1.3\n");

    ExpectRefused("assets://settings.yml:3: 'vulkan_version' of 'rendering' of the settings is not a version "
                  "of Vulkan 1 in quotes, such as \"1.3\"");
  }

  TEST_F(SettingsFilesTest, RefusesAVulkanVersionThatIsNotOfVulkan1)
  {
    WriteOfTheProject("version: 1\nrendering:\n  vulkan_version: \"2.0\"\n");

    ExpectRefused("is not a version of Vulkan 1 in quotes");
  }

  TEST_F(SettingsFilesTest, ClampsLightAboveWhiteAtExposureOneWithoutAFile)
  {
    // the defaults keep every frame of a scene that never goes above white
    EXPECT_EQ(_settings.tonemapper, Tonemapper::None);
    EXPECT_DOUBLE_EQ(_settings.exposure, 1.0);
  }

  TEST_F(SettingsFilesTest, ReadsEveryTonemapperByName)
  {
    WriteOfTheProject("version: 1\nrendering:\n  tonemapper: agx\n");
    ASSERT_TRUE(ReadOfTheProject()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_settings.tonemapper, Tonemapper::Agx);

    WriteOfTheProject("version: 1\nrendering:\n  tonemapper: none\n");
    ASSERT_TRUE(ReadOfTheProject()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_settings.tonemapper, Tonemapper::None);
  }

  TEST_F(SettingsFilesTest, RefusesATonemapperItDoesNotKnow)
  {
    WriteOfTheProject("version: 1\nrendering:\n  tonemapper: reinhard\n");

    EXPECT_FALSE(ReadOfTheProject());
    ASSERT_EQ(_errors.size(), 1u) << ::testing::PrintToString(_errors);
    EXPECT_THAT(_errors.front(), HasSubstr("'tonemapper'"));
    EXPECT_THAT(_errors.front(), HasSubstr("aces"));
    EXPECT_THAT(_errors.front(), HasSubstr("agx"));
  }

  TEST_F(SettingsFilesTest, RefusesAnExposureThatIsNotAboveZero)
  {
    WriteOfTheProject("version: 1\nrendering:\n  exposure: 0\n");

    ExpectRefused("assets://settings.yml:3: 'exposure' of 'rendering' of the settings is 0, "
                  "where a number above zero was expected");
  }

  TEST_F(SettingsFilesTest, RefusesANameItDoesNotKnow)
  {
    WriteOfTheProject("version: 1\nwindow:\n  wide: 1920\n");

    ExpectRefused("assets://settings.yml:3: 'wide' is not known to 'window' of the settings. Known are: ");
  }

  TEST_F(SettingsFilesTest, RefusesAPartItDoesNotKnow)
  {
    WriteOfTheProject("version: 1\nnetwork:\n  port: 1\n");

    ExpectRefused("assets://settings.yml:2: 'network' is not known to the settings. Known are: ");
  }

  // the groups of sounds

  TEST_F(SettingsFilesTest, HasTheGroupsOfTheEngineWithoutAFile)
  {
    EXPECT_EQ(VolumeOf("music"), 1.0f);
    EXPECT_EQ(VolumeOf("effects"), 1.0f);
    EXPECT_EQ(VolumeOf("voices"), 1.0f);
    EXPECT_EQ(VolumeOf("ambience"), 1.0f);
    EXPECT_EQ(_settings.sound_groups.size(), 4u);
  }

  TEST_F(SettingsFilesTest, AGroupOfTheProjectPausesWithTheGameAsTheEffectsDo)
  {
    WriteOfTheProject("version: 1\naudio:\n  groups: [radio]\n");

    ASSERT_TRUE(ReadOfTheProject()) << ::testing::PrintToString(_errors);
    ASSERT_EQ(_settings.sound_groups.size(), 5u);
    EXPECT_EQ(_settings.sound_groups.back().name, "radio");
    EXPECT_TRUE(_settings.sound_groups.back().pauses);
    EXPECT_FALSE(_settings.sound_groups.front().pauses);
  }

  TEST_F(SettingsFilesTest, ThePlayersFileSetsTheVolumeOfAGroupOfTheProject)
  {
    WriteOfTheProject("version: 1\naudio:\n  groups:\n    - radio\n  volumes:\n    radio: 0.5\n");
    WriteOfThePlayer("version: 1\naudio:\n  volumes:\n    radio: 0.25\n    ambience: 0\n");

    ASSERT_TRUE(ReadOfTheProject()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(VolumeOf("radio"), 0.5f);

    ASSERT_TRUE(ReadOfThePlayer()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(VolumeOf("radio"), 0.25f);
    EXPECT_EQ(VolumeOf("ambience"), 0.0f);
    EXPECT_EQ(VolumeOf("music"), 1.0f);
  }

  TEST_F(SettingsFilesTest, RefusesAGroupThatIsDeclaredTwice)
  {
    WriteOfTheProject("version: 1\naudio:\n  groups:\n    - radio\n    - name: radio\n      volume: 0.5\n");

    ExpectRefused("assets://settings.yml:5: 'groups' of 'audio' of the settings declares 'radio', which is a "
                  "group already. There are: music, effects, voices, ambience, radio");
  }

  TEST_F(SettingsFilesTest, RefusesAGroupOfTheEngineDeclaredAgain)
  {
    WriteOfTheProject("version: 1\naudio:\n  groups:\n    - ambience\n");

    ExpectRefused("assets://settings.yml:4: 'groups' of 'audio' of the settings declares 'ambience', which is a "
                  "group already. There are: music, effects, voices, ambience");
  }

  TEST_F(SettingsFilesTest, RefusesAVolumeOfAGroupThatIsNotThere)
  {
    WriteOfTheProject("version: 1\naudio:\n  volumes:\n    musik: 0.5\n");

    ExpectRefused("assets://settings.yml:4: 'musik' of 'volumes' of 'audio' of the settings is not a group. "
                  "There are: music, effects, voices, ambience");
  }

  TEST_F(SettingsFilesTest, RefusesANameAGroupDoesNotKnow)
  {
    WriteOfTheProject("version: 1\naudio:\n  groups:\n    - name: radio\n      loudness: 0.5\n");

    ExpectRefused("assets://settings.yml:5: 'loudness' is not known to group 1 of 'groups' of 'audio' of the "
                  "settings. Known are: name, volume");
  }

  TEST_F(SettingsFilesTest, RefusesAGroupWithoutAName)
  {
    WriteOfTheProject("version: 1\naudio:\n  groups:\n    - volume: 0.5\n");

    ExpectRefused("assets://settings.yml:4: 'name' is missing. It holds the name of the group");
  }

  TEST_F(SettingsFilesTest, RefusesAGroupThatIsNeitherANameNorAMap)
  {
    WriteOfTheProject("version: 1\naudio:\n  groups:\n    - [radio]\n");

    ExpectRefused("assets://settings.yml:4: 'groups' of 'audio' of the settings holds a list, where the name of "
                  "a group was expected, or a map with its name and volume");
  }

  TEST_F(SettingsFilesTest, RefusesGroupsThatAreNotAList)
  {
    WriteOfTheProject("version: 1\naudio:\n  groups: radio\n");

    ExpectRefused("assets://settings.yml:3: 'groups' of 'audio' of the settings is text, where a list of names "
                  "was expected");
  }

  TEST_F(SettingsFilesTest, RefusesAVolumeBelowSilence)
  {
    WriteOfTheProject("version: 1\naudio:\n  volumes:\n    music: -1\n");

    ExpectRefused("assets://settings.yml:4: 'music' of 'volumes' of 'audio' of the settings is -1, where a number "
                  "of at least 0 was expected, 1 being the loudness of the sounds");
  }

  TEST_F(SettingsFilesTest, AFileWithAGroupThatIsRefusedDeclaresNone)
  {
    WriteOfTheProject("version: 1\naudio:\n  groups:\n    - radio\n    - music\n");

    EXPECT_FALSE(ReadOfTheProject());
    EXPECT_EQ(_settings.sound_groups.size(), 4u);
  }

  TEST_F(SettingsFilesTest, ReportsEveryProblemNotOnlyTheFirst)
  {
    WriteOfTheProject("version: 1\nwindow:\n  width: 0\n  height: 0\nui:\n  scale: -1\n");

    EXPECT_FALSE(ReadOfTheProject());
    EXPECT_EQ(_errors.size(), 3u) << ::testing::PrintToString(_errors);
  }

  // the settings of the runtime

  TEST_F(SettingsFilesTest, ReadsTheSettingsOfTheRuntime)
  {
    WriteOfTheProject(FileOfTheRuntime());

    ASSERT_TRUE(ReadOfTheProject()) << ::testing::PrintToString(_errors);
    EXPECT_EQ(_settings.width, 1920);
    EXPECT_EQ(_settings.height, 1080);
    EXPECT_EQ(_settings.window_mode, WindowMode::Borderless);
    EXPECT_EQ(_settings.pause_menu, "assets://ui/pause.ui.yml");
    EXPECT_EQ(_settings.settings_menu, "assets://ui/settings.ui.yml");
  }
} // namespace
