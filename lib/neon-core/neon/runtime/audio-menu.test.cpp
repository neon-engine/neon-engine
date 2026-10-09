#include "audio-menu.hpp"

#include <format>
#include <map>
#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/mock-audio-context.hpp>
#include <neon/testing/mock-ui-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::AudioMenu;
  using neon::DataValue;
  using neon::DocumentFormat;
  using neon::PlayerSettings;
  using neon::SoundGroupSetting;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::MockAudioContext;
  using neon::testing::MockUiContext;
  using neon::testing::RecordingLogger;
  using ::testing::_;
  using ::testing::NiceMock;

  // A format that reads nothing and writes a line, for a test that looks at
  // what would be written rather than at the text.
  class WrittenFormat final : public DocumentFormat
  {
  public:
    bool Read(const std::string &, const std::string &, DataValue &, std::string &error) override
    {
      error = "not read in this test";
      return false;
    }

    std::string Write(const DataValue &) override { return "written\n"; }
  };

  class AudioMenuTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    NiceMock<MockUiContext> _ui;
    NiceMock<MockAudioContext> _audio;
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    WrittenFormat _format;
    PlayerSettings _player{&_files, &_format, _logger};

    // the groups of the engine, and one of the project
    std::vector<SoundGroupSetting> _groups = [] {
      auto groups = neon::built_in_sound_groups();
      groups.push_back({.name = "radio"});
      return groups;
    }();

    // what the user interface holds, as the sliders of the menu would
    std::map<std::string, double> _values;

    // the volumes of the groups as the audio holds them
    std::map<std::string, float> _volumes = {
      {"music", 0.6f}, {"effects", 0.8f}, {"voices", 1.0f}, {"ambience", 0.5f}, {"radio", 0.25f}
    };

    void SetUp() override
    {
      _files.Initialize();

      ON_CALL(_ui, SetNumber(_, _)).WillByDefault([this](const std::string &name, const double number)
      {
        _values[name] = number;
      });
      ON_CALL(_ui, GetNumber(_, _)).WillByDefault([this](const std::string &name, double &number)
      {
        const auto found = _values.find(name);
        if (found == _values.end()) { return false; }
        number = found->second;
        return true;
      });

      ON_CALL(_audio, GetGroupVolume(_)).WillByDefault([this](const std::string &group)
      {
        const auto found = _volumes.find(group);
        return found == _volumes.end() ? 0.0f : found->second;
      });
      ON_CALL(_audio, SetGroupVolume(_, _)).WillByDefault([this](const std::string &group, const float volume)
      {
        _volumes[group] = volume;
      });
    }

    AudioMenu Menu(PlayerSettings *player = nullptr)
    {
      return AudioMenu(&_ui, &_audio, _groups, player, _logger);
    }

    /// The volume of a group the file of the player would hold, or null.
    const DataValue *Kept(const std::string &group)
    {
      const DataValue *audio = _player.Document().Find("audio");
      const DataValue *volumes = audio != nullptr ? audio->Find("volumes") : nullptr;
      return volumes != nullptr ? volumes->Find(group) : nullptr;
    }
  };

  TEST_F(AudioMenuTest, ShowsTheVolumesAsTheyAreWhenItIsOpened)
  {
    AudioMenu menu = Menu();

    // what the menu's file starts them with is no choice of the player's
    _values["music"] = 60.0;
    _values["effects"] = 80.0;
    _values["ambience"] = 50.0;
    _volumes["music"] = 0.3f;

    menu.Open();

    EXPECT_DOUBLE_EQ(_values["music"], 30.0);
    EXPECT_DOUBLE_EQ(_values["effects"], 80.0);
    EXPECT_DOUBLE_EQ(_values["voices"], 100.0);
    EXPECT_DOUBLE_EQ(_values["ambience"], 50.0);
    EXPECT_DOUBLE_EQ(_values["radio"], 25.0) << "a group of the project has a value too";
    EXPECT_TRUE(menu.IsOpen());
  }

  TEST_F(AudioMenuTest, HasAValueForEveryGroupNamedAfterIt)
  {
    EXPECT_THAT(Menu().GetValueNames(), ::testing::ElementsAre("music", "effects", "voices", "ambience", "radio"));
  }

  TEST_F(AudioMenuTest, ShowsAVolumeToAHundredth)
  {
    _volumes["music"] = 1.0f / 3.0f;

    AudioMenu menu = Menu();
    menu.Open();

    EXPECT_DOUBLE_EQ(_values["music"], 33.33);
  }

  TEST_F(AudioMenuTest, ChangesAVolumeAtOnce)
  {
    AudioMenu menu = Menu();
    menu.Open();

    _values["music"] = 25.0;
    _values["radio"] = 100.0;
    menu.Update();

    EXPECT_FLOAT_EQ(_volumes["music"], 0.25f);
    EXPECT_FLOAT_EQ(_volumes["radio"], 1.0f);
    EXPECT_FLOAT_EQ(_volumes["effects"], 0.8f);
  }

  TEST_F(AudioMenuTest, TakesAValueBelowZeroAsSilence)
  {
    AudioMenu menu = Menu();
    menu.Open();

    _values["effects"] = -10.0;
    menu.Update();

    EXPECT_FLOAT_EQ(_volumes["effects"], 0.0f);
  }

  TEST_F(AudioMenuTest, ChangesNothingWhenNothingChanged)
  {
    _volumes["music"] = 1.0f / 3.0f;

    AudioMenu menu = Menu();
    menu.Open();

    EXPECT_CALL(_audio, SetGroupVolume(_, _)).Times(0);
    menu.Update();
    menu.Update();
    menu.Close(false);
  }

  TEST_F(AudioMenuTest, HandsAVolumeOverOnlyWhenItChanges)
  {
    AudioMenu menu = Menu();
    menu.Open();

    EXPECT_CALL(_audio, SetGroupVolume("music", _)).Times(1);
    _values["music"] = 40.0;
    menu.Update();
    menu.Update();
  }

  TEST_F(AudioMenuTest, KeepsWhatChangedWhenTheMenuIsClosedWithApply)
  {
    AudioMenu menu = Menu(&_player);
    menu.Open();
    _values["music"] = 45.0;
    _values["radio"] = 0.0;
    menu.Update();

    menu.Close(true);

    ASSERT_NE(Kept("music"), nullptr);
    double music = 0.0;
    ASSERT_TRUE(Kept("music")->GetNumber(music));
    EXPECT_NEAR(music, 0.45, 1e-6);
    ASSERT_NE(Kept("radio"), nullptr);
    EXPECT_EQ(Kept("effects"), nullptr) << "what the player never changed stays the project's";
    EXPECT_FLOAT_EQ(_volumes["music"], 0.45f) << "and it is still heard";
    EXPECT_TRUE(_files.Exists("user://settings.yml"));
    EXPECT_FALSE(menu.IsOpen());
  }

  TEST_F(AudioMenuTest, TakesWhatChangedInTheFrameTheMenuIsClosed)
  {
    AudioMenu menu = Menu(&_player);
    menu.Open();
    _values["ambience"] = 10.0;

    menu.Close(true);

    EXPECT_FLOAT_EQ(_volumes["ambience"], 0.1f);
    EXPECT_NE(Kept("ambience"), nullptr);
  }

  TEST_F(AudioMenuTest, WritesNothingWhenNothingChanged)
  {
    AudioMenu menu = Menu(&_player);
    menu.Open();

    menu.Close(true);

    EXPECT_FALSE(_files.Exists("user://settings.yml"));
  }

  TEST_F(AudioMenuTest, PutsBackWhatWasNotKept)
  {
    _volumes["music"] = 1.0f / 3.0f;

    AudioMenu menu = Menu(&_player);
    menu.Open();
    _values["music"] = 90.0;
    _values["effects"] = 20.0;
    menu.Update();

    menu.Close(false);

    EXPECT_FLOAT_EQ(_volumes["music"], 1.0f / 3.0f) << "as it was, not as the menu showed it";
    EXPECT_FLOAT_EQ(_volumes["effects"], 0.8f);
    EXPECT_FALSE(_files.Exists("user://settings.yml"));
  }

  TEST_F(AudioMenuTest, ShowsWhatWasKeptWhenItIsOpenedAgain)
  {
    AudioMenu menu = Menu(&_player);
    menu.Open();
    _values["music"] = 15.0;
    menu.Close(true);

    _values["music"] = 60.0;
    menu.Open();

    EXPECT_DOUBLE_EQ(_values["music"], 15.0);
  }

  TEST_F(AudioMenuTest, DoesNothingUntilItIsOpened)
  {
    AudioMenu menu = Menu();
    _values["music"] = 10.0;

    EXPECT_CALL(_audio, SetGroupVolume(_, _)).Times(0);
    menu.Update();
    menu.Close(true);

    EXPECT_FALSE(menu.IsOpen());
  }

  TEST_F(AudioMenuTest, KeepsNothingWithoutTheSettingsOfThePlayer)
  {
    AudioMenu menu = Menu();
    menu.Open();
    _values["music"] = 10.0;

    menu.Close(true);

    EXPECT_FLOAT_EQ(_volumes["music"], 0.1f);
    EXPECT_FALSE(_files.Exists("user://settings.yml"));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);
  }
} // namespace
