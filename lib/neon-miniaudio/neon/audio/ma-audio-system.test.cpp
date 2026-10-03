#include "ma-audio-system.hpp"

#include <cmath>
#include <cstdint>
#include <memory>
#include <numbers>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::ListenerInfo;
  using neon::MA_AudioSystem;
  using neon::SoundInfo;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;

  constexpr double frame = 1.0 / 60.0;

  /// A WAV file of a sine tone: one channel of 16-bit samples, at the rate
  /// the audio system mixes at, so that nothing is resampled.
  std::string Tone(const double seconds, const float amplitude = 0.5f)
  {
    constexpr std::uint32_t rate = MA_AudioSystem::sample_rate;
    constexpr double frequency = 440.0;
    const auto count = static_cast<std::uint32_t>(seconds * rate);
    const std::uint32_t data_size = count * 2;

    std::string wav;
    const auto write = [&wav](const std::uint32_t value, const int bytes)
    {
      for (int i = 0; i < bytes; i++) { wav.push_back(static_cast<char>(value >> (8 * i) & 0xff)); }
    };

    wav += "RIFF";
    write(36 + data_size, 4);
    wav += "WAVEfmt ";
    write(16, 4); // size of the format
    write(1, 2); // PCM
    write(1, 2); // channels
    write(rate, 4);
    write(rate * 2, 4); // bytes per second
    write(2, 2); // bytes per frame
    write(16, 2); // bits per sample
    wav += "data";
    write(data_size, 4);

    for (std::uint32_t i = 0; i < count; i++)
    {
      const double sample = amplitude * std::sin(2.0 * std::numbers::pi * frequency * i / rate);
      write(static_cast<std::uint16_t>(static_cast<std::int16_t>(std::lround(sample * 32767.0))), 2);
    }

    return wav;
  }

  /// Settings that mix in Advance() instead of through a sound card.
  SettingsConfig WithoutASoundCard()
  {
    SettingsConfig settings;
    settings.audio_output = AudioOutput::None;
    return settings;
  }

  class MaAudioSystemTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    MA_AudioSystem _audio{WithoutASoundCard(), &_files, _logger};

    void SetUp() override
    {
      _files.Initialize();
      _files.AddNativeFile("/assets/sounds/tone.wav", Tone(1.0));
      _files.AddNativeFile("/assets/sounds/short.wav", Tone(0.2));
      _files.AddNativeFile("/assets/sounds/quiet.wav", Tone(1.0, 0.25f));
      _files.AddNativeFile("/assets/sounds/broken.wav", "this is not a sound");
      _audio.Initialize();
    }

    void TearDown() override
    {
      _audio.CleanUp();
    }

    /// How loud it is after time has passed.
    float LevelAfter(const double seconds)
    {
      _audio.Advance(seconds);
      return _audio.GetOutputLevel();
    }

    int Playing(const SoundInfo &sound_info)
    {
      const int sound = _audio.CreateSound(sound_info);
      _audio.Play(sound);
      return sound;
    }

    /// The level of every frame, for as many frames as fit in the time.
    std::vector<float> LevelsOver(const double seconds)
    {
      std::vector<float> levels;
      for (int i = 0; i < static_cast<int>(std::lround(seconds / frame)); i++) { levels.push_back(LevelAfter(frame)); }
      return levels;
    }

    /// Whether every level is at most a little above the one before.
    static bool Falls(const std::vector<float> &levels)
    {
      for (std::size_t i = 1; i < levels.size(); i++)
      {
        if (levels[i] > levels[i - 1] + 0.01f) { return false; }
      }
      return true;
    }

    /// Whether every level is at least a little below the one before.
    static bool Rises(const std::vector<float> &levels)
    {
      for (std::size_t i = 1; i < levels.size(); i++)
      {
        if (levels[i] < levels[i - 1] - 0.01f) { return false; }
      }
      return true;
    }
  };

  // Initialize

  TEST_F(MaAudioSystemTest, SaysItMixesWithoutASoundCard)
  {
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Audio is mixed without a sound card"));
  }

  TEST_F(MaAudioSystemTest, CannotCreateASoundBeforeItIsInitialized)
  {
    MA_AudioSystem audio{WithoutASoundCard(), &_files, _logger};

    EXPECT_EQ(audio.CreateSound({.path = "assets://sounds/tone.wav"}), -1);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "before the audio system was initialized"));
  }

  // Creating sounds

  TEST_F(MaAudioSystemTest, CreatesASoundOfAFile)
  {
    EXPECT_GE(_audio.CreateSound({.path = "assets://sounds/tone.wav"}), 0);
  }

  TEST_F(MaAudioSystemTest, GivesTwoSoundsOfOneFileIdsOfTheirOwn)
  {
    const int first = _audio.CreateSound({.path = "assets://sounds/tone.wav"});
    const int second = _audio.CreateSound({.path = "assets://sounds/tone.wav"});

    EXPECT_GE(second, 0);
    EXPECT_NE(first, second);
  }

  TEST_F(MaAudioSystemTest, CannotCreateASoundOfAFileThatIsNotThere)
  {
    EXPECT_EQ(_audio.CreateSound({.path = "assets://sounds/missing.wav"}), -1);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "assets://sounds/missing.wav cannot be read"));
  }

  TEST_F(MaAudioSystemTest, CannotCreateASoundOfAFileThatIsNotASound)
  {
    EXPECT_EQ(_audio.CreateSound({.path = "assets://sounds/broken.wav"}), -1);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "assets://sounds/broken.wav is not a sound that can be played"));
  }

  // Loudness

  TEST_F(MaAudioSystemTest, IsSilentWithNothingPlaying)
  {
    _audio.CreateSound({.path = "assets://sounds/tone.wav"});

    EXPECT_EQ(LevelAfter(0.1), 0.0f);
  }

  TEST_F(MaAudioSystemTest, IsAsLoudAsTheFileWhenASoundPlays)
  {
    Playing({.path = "assets://sounds/tone.wav"});

    EXPECT_NEAR(LevelAfter(0.1), 0.5f, 0.02f);
  }

  TEST_F(MaAudioSystemTest, PlaysASoundAtItsVolume)
  {
    Playing({.path = "assets://sounds/tone.wav", .volume = 0.5f});

    EXPECT_NEAR(LevelAfter(0.1), 0.25f, 0.02f);
  }

  TEST_F(MaAudioSystemTest, FollowsAChangeOfVolume)
  {
    const int sound = Playing({.path = "assets://sounds/tone.wav"});
    LevelAfter(0.1);

    _audio.SetVolume(sound, 0.0f);

    EXPECT_EQ(LevelAfter(0.1), 0.0f);
  }

  TEST_F(MaAudioSystemTest, TakesNoVolumeBelowSilence)
  {
    const int sound = Playing({.path = "assets://sounds/tone.wav"});

    _audio.SetVolume(sound, -1.0f);

    EXPECT_EQ(LevelAfter(0.1), 0.0f);
  }

  TEST_F(MaAudioSystemTest, FollowsTheVolumeOfEverything)
  {
    Playing({.path = "assets://sounds/tone.wav"});

    _audio.SetMasterVolume(0.5f);
    EXPECT_NEAR(LevelAfter(0.1), 0.25f, 0.02f);

    _audio.SetMasterVolume(0.0f);
    EXPECT_EQ(LevelAfter(0.1), 0.0f);
  }

  TEST_F(MaAudioSystemTest, AddsUpTwoSoundsOfOneFile)
  {
    Playing({.path = "assets://sounds/quiet.wav"});
    const float one = LevelAfter(0.1);

    // started together, so that they are in step
    _audio.CleanUp();
    _audio.Initialize();
    Playing({.path = "assets://sounds/quiet.wav"});
    Playing({.path = "assets://sounds/quiet.wav"});
    const float two = LevelAfter(0.1);

    EXPECT_NEAR(one, 0.25f, 0.02f);
    EXPECT_NEAR(two, 0.5f, 0.04f);
  }

  // Fading

  TEST_F(MaAudioSystemTest, FadesASoundToAVolumeOverTime)
  {
    const int sound = Playing({.path = "assets://sounds/tone.wav", .looping = true});
    LevelAfter(0.1);

    _audio.FadeTo(sound, 0.5f, 0.5);
    const auto levels = LevelsOver(0.5);

    // from 0.5 to half of that, which it reaches as the fade ends
    EXPECT_TRUE(Falls(levels));
    EXPECT_GT(levels.front(), 0.45f);
    EXPECT_NEAR(levels[levels.size() / 2], 0.375f, 0.03f);
    EXPECT_NEAR(levels.back(), 0.25f, 0.02f);

    // and where it stays
    EXPECT_NEAR(LevelAfter(0.5), 0.25f, 0.02f);
    EXPECT_TRUE(_audio.IsPlaying(sound));
  }

  TEST_F(MaAudioSystemTest, FadesASoundOnTopOfItsVolume)
  {
    const int sound = Playing({.path = "assets://sounds/tone.wav", .looping = true});

    _audio.FadeTo(sound, 0.5f, 0.0);
    _audio.SetVolume(sound, 0.5f);

    EXPECT_NEAR(LevelAfter(0.1), 0.125f, 0.01f);
  }

  TEST_F(MaAudioSystemTest, KeepsPlayingASoundThatFadedToSilence)
  {
    const int sound = Playing({.path = "assets://sounds/tone.wav", .looping = true});

    _audio.FadeTo(sound, 0.0f, 0.25);
    LevelAfter(0.5);

    EXPECT_EQ(LevelAfter(0.1), 0.0f);
    EXPECT_TRUE(_audio.IsPlaying(sound));
  }

  TEST_F(MaAudioSystemTest, PlaysASoundThatFadedToSilenceAtItsVolumeAgain)
  {
    const int sound = Playing({.path = "assets://sounds/tone.wav", .looping = true});
    _audio.FadeTo(sound, 0.0f, 0.0);
    LevelAfter(0.1);

    _audio.Play(sound);

    EXPECT_NEAR(LevelAfter(0.1), 0.5f, 0.02f);
  }

  TEST_F(MaAudioSystemTest, FadesASoundOutAndStopsIt)
  {
    const int sound = Playing({.path = "assets://sounds/tone.wav", .looping = true});
    LevelAfter(0.1);

    _audio.FadeOut(sound, 0.5);
    const auto levels = LevelsOver(0.45);

    EXPECT_TRUE(Falls(levels));
    EXPECT_GT(levels.front(), 0.45f);
    EXPECT_LT(levels.back(), 0.1f);
    EXPECT_TRUE(_audio.IsPlaying(sound));

    // the time the fade takes is that which Advance() is told
    LevelAfter(0.06);
    EXPECT_FALSE(_audio.IsPlaying(sound));
    EXPECT_EQ(LevelAfter(0.1), 0.0f);
  }

  TEST_F(MaAudioSystemTest, PlaysASoundThatFadedOutAgain)
  {
    const int sound = Playing({.path = "assets://sounds/tone.wav", .looping = true});
    _audio.FadeOut(sound, 0.1);
    LevelAfter(0.2);

    _audio.Play(sound);

    EXPECT_NEAR(LevelAfter(0.1), 0.5f, 0.02f);
    EXPECT_TRUE(_audio.IsPlaying(sound));
  }

  TEST_F(MaAudioSystemTest, PutsOffAFadeOutWhenTheSoundIsFadedAgain)
  {
    const int sound = Playing({.path = "assets://sounds/tone.wav", .looping = true});
    _audio.FadeOut(sound, 0.5);
    LevelAfter(0.25);

    _audio.FadeTo(sound, 1.0f, 0.25);
    LevelAfter(0.5);

    EXPECT_TRUE(_audio.IsPlaying(sound));
    EXPECT_NEAR(LevelAfter(0.1), 0.5f, 0.02f);
  }

  TEST_F(MaAudioSystemTest, FadesASoundInFromSilence)
  {
    const int sound = _audio.CreateSound({.path = "assets://sounds/tone.wav", .looping = true});

    _audio.FadeIn(sound, 0.5);
    const auto levels = LevelsOver(0.5);

    EXPECT_TRUE(Rises(levels));
    EXPECT_LT(levels.front(), 0.05f);
    EXPECT_NEAR(levels[levels.size() / 2], 0.25f, 0.03f);
    EXPECT_NEAR(LevelAfter(0.1), 0.5f, 0.02f);
  }

  TEST_F(MaAudioSystemTest, CrossfadesFromOnePieceOfMusicToAnother)
  {
    // the two are mixed into one level, so each is measured on its own with
    // the group of the other silent. Runs mix the same, which makes the two
    // runs one crossfade heard twice
    const auto run = [this](const std::string &silent_group)
    {
      _audio.CleanUp();
      _audio.Initialize();
      _audio.AddGroup("first");
      _audio.AddGroup("second");
      _audio.SetGroupVolume("first", 1.0f);
      _audio.SetGroupVolume("second", 1.0f);
      _audio.SetGroupVolume(silent_group, 0.0f);

      const int first = Playing({.path = "assets://sounds/tone.wav", .looping = true, .group = "first"});
      const int second = _audio.CreateSound({.path = "assets://sounds/tone.wav", .looping = true, .group = "second"});
      LevelAfter(0.1);

      _audio.Crossfade(first, second, 1.0);
      auto levels = LevelsOver(1.0);
      LevelAfter(0.05);

      EXPECT_FALSE(_audio.IsPlaying(first));
      EXPECT_TRUE(_audio.IsPlaying(second));
      return levels;
    };

    const auto first = run("second");
    const auto second = run("first");

    EXPECT_TRUE(Falls(first));
    EXPECT_GT(first.front(), 0.45f);
    EXPECT_LT(first.back(), 0.05f);

    EXPECT_TRUE(Rises(second));
    EXPECT_LT(second.front(), 0.05f);
    EXPECT_GT(second.back(), 0.45f);

    // halfway, both are heard at half
    EXPECT_NEAR(first[first.size() / 2], 0.25f, 0.03f);
    EXPECT_NEAR(second[second.size() / 2], 0.25f, 0.03f);
  }

  TEST_F(MaAudioSystemTest, GivesTheSameLevelsOfAFadeInEveryRun)
  {
    const auto run = [this]
    {
      _audio.CleanUp();
      _audio.Initialize();

      const int sound = Playing({.path = "assets://sounds/tone.wav", .looping = true});
      _audio.FadeOut(sound, 0.4);
      return LevelsOver(0.5);
    };

    EXPECT_EQ(run(), run());
  }

  // Groups

  TEST_F(MaAudioSystemTest, PlaysASoundAtTheVolumeOfItsGroup)
  {
    Playing({.path = "assets://sounds/tone.wav", .group = neon::sound_group::music});

    _audio.SetGroupVolume(neon::sound_group::music, 0.5f);

    EXPECT_NEAR(LevelAfter(0.1), 0.25f, 0.02f);
  }

  TEST_F(MaAudioSystemTest, PutsASoundAmongTheEffectsUnlessItSaysOtherwise)
  {
    Playing({.path = "assets://sounds/tone.wav"});

    _audio.SetGroupVolume(neon::sound_group::effects, 0.0f);

    EXPECT_EQ(LevelAfter(0.1), 0.0f);
  }

  TEST_F(MaAudioSystemTest, LeavesTheSoundsOfTheOtherGroupsAsTheyAre)
  {
    Playing({.path = "assets://sounds/tone.wav", .group = neon::sound_group::voices});

    _audio.SetGroupVolume(neon::sound_group::music, 0.0f);
    _audio.SetGroupVolume(neon::sound_group::effects, 0.0f);

    EXPECT_NEAR(LevelAfter(0.1), 0.5f, 0.02f);
  }

  TEST_F(MaAudioSystemTest, PutsTheVolumeOfEverythingOnTopOfThatOfTheGroup)
  {
    Playing({.path = "assets://sounds/tone.wav", .group = neon::sound_group::music});

    _audio.SetGroupVolume(neon::sound_group::music, 0.5f);
    _audio.SetMasterVolume(0.5f);

    EXPECT_NEAR(LevelAfter(0.1), 0.125f, 0.01f);
  }

  TEST_F(MaAudioSystemTest, PlaysASoundOfAGroupOfAGame)
  {
    _audio.AddGroup("ambience");
    Playing({.path = "assets://sounds/tone.wav", .group = "ambience"});

    _audio.SetGroupVolume("ambience", 0.5f);

    EXPECT_NEAR(LevelAfter(0.1), 0.25f, 0.02f);
  }

  TEST_F(MaAudioSystemTest, PutsASoundOfAGroupThatIsNotThereAmongTheEffects)
  {
    Playing({.path = "assets://sounds/tone.wav", .group = "musik"});

    _audio.SetGroupVolume(neon::sound_group::effects, 0.5f);

    EXPECT_NEAR(LevelAfter(0.1), 0.25f, 0.02f);
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "Sound assets://sounds/tone.wav is of group musik, which is not there"));
  }

  TEST_F(MaAudioSystemTest, KeepsTheVolumeOfAGroupWhenInitializedAgain)
  {
    _audio.SetGroupVolume(neon::sound_group::music, 0.5f);

    _audio.CleanUp();
    _audio.Initialize();
    Playing({.path = "assets://sounds/tone.wav", .group = neon::sound_group::music});

    EXPECT_EQ(_audio.GetGroupVolume(neon::sound_group::music), 0.5f);
    EXPECT_NEAR(LevelAfter(0.1), 0.25f, 0.02f);
  }

  TEST_F(MaAudioSystemTest, HasAmbienceFromTheStart)
  {
    Playing({.path = "assets://sounds/tone.wav", .group = neon::sound_group::ambience});

    _audio.SetGroupVolume(neon::sound_group::ambience, 0.5f);

    EXPECT_NEAR(LevelAfter(0.1), 0.25f, 0.02f);
    EXPECT_FALSE(_logger->Contains(LogLevel::Warn, "is of group ambience"));
  }

  TEST_F(MaAudioSystemTest, StartsWithTheGroupsOfTheSettingsAtTheirVolumes)
  {
    auto settings = WithoutASoundCard();
    settings.sound_groups.front().volume = 0.25f;
    settings.sound_groups.push_back({.name = "radio", .volume = 0.5f});

    MA_AudioSystem audio{settings, &_files, _logger};
    audio.Initialize();

    EXPECT_EQ(audio.GetGroupVolume(neon::sound_group::music), 0.25f);
    EXPECT_EQ(audio.GetGroupVolume("radio"), 0.5f);

    const int sound = audio.CreateSound({.path = "assets://sounds/tone.wav", .group = "radio"});
    audio.Play(sound);
    audio.Advance(0.1);
    EXPECT_NEAR(audio.GetOutputLevel(), 0.25f, 0.02f);
    EXPECT_FALSE(_logger->Contains(LogLevel::Warn, "is of group radio"));

    audio.CleanUp();
  }

  // Pausing and stopping groups

  TEST_F(MaAudioSystemTest, HoldsTheAmbienceWhilePausedAndLetsItGoOnAfter)
  {
    const int hum = Playing({.path = "assets://sounds/tone.wav", .looping = true, .group = neon::sound_group::ambience});

    _audio.SetPaused(true);

    EXPECT_EQ(LevelAfter(0.1), 0.0f);
    EXPECT_TRUE(_audio.IsPlaying(hum));

    _audio.SetPaused(false);

    EXPECT_NEAR(LevelAfter(0.1), 0.5f, 0.02f);
  }

  TEST_F(MaAudioSystemTest, HoldsTheEffectsAndTheVoicesAndTheGroupsOfAGameWhilePaused)
  {
    _audio.AddGroup("radio");
    Playing({.path = "assets://sounds/tone.wav", .looping = true, .group = neon::sound_group::effects});
    Playing({.path = "assets://sounds/tone.wav", .looping = true, .group = neon::sound_group::voices});
    Playing({.path = "assets://sounds/tone.wav", .looping = true, .group = "radio"});

    _audio.SetPaused(true);

    EXPECT_EQ(LevelAfter(0.1), 0.0f);
  }

  TEST_F(MaAudioSystemTest, LetsTheMusicPlayOnWhilePaused)
  {
    Playing({.path = "assets://sounds/tone.wav", .looping = true, .group = neon::sound_group::music});

    _audio.SetPaused(true);

    EXPECT_NEAR(LevelAfter(0.1), 0.5f, 0.02f);
  }

  TEST_F(MaAudioSystemTest, GoesOnFromWhereASoundWasHeld)
  {
    // 0.2 seconds long: held after 0.1, it would have ended while paused
    const int sound = Playing({.path = "assets://sounds/short.wav"});
    LevelAfter(0.1);

    _audio.SetPaused(true);
    LevelAfter(0.5);
    EXPECT_TRUE(_audio.IsPlaying(sound));

    _audio.SetPaused(false);

    EXPECT_NEAR(LevelAfter(0.05), 0.5f, 0.02f);
    EXPECT_TRUE(_audio.IsPlaying(sound));
    LevelAfter(0.2);
    EXPECT_FALSE(_audio.IsPlaying(sound));
  }

  TEST_F(MaAudioSystemTest, DoesNotHoldASoundThatIsPlayedWhilePaused)
  {
    const int sound = _audio.CreateSound({.path = "assets://sounds/tone.wav", .looping = true});
    _audio.SetPaused(true);

    _audio.Play(sound);

    EXPECT_NEAR(LevelAfter(0.1), 0.5f, 0.02f);
  }

  TEST_F(MaAudioSystemTest, StopsTheMusicAndLeavesTheAmbience)
  {
    const int music = Playing({.path = "assets://sounds/tone.wav", .looping = true, .group = neon::sound_group::music});
    const int hum = Playing({.path = "assets://sounds/quiet.wav", .looping = true, .group = neon::sound_group::ambience});
    EXPECT_NEAR(LevelAfter(0.1), 0.75f, 0.03f);

    _audio.StopGroup(neon::sound_group::music);

    EXPECT_NEAR(LevelAfter(0.1), 0.25f, 0.02f);
    EXPECT_FALSE(_audio.IsPlaying(music));
    EXPECT_TRUE(_audio.IsPlaying(hum));
  }

  TEST_F(MaAudioSystemTest, StopsTheEffectsAndLeavesTheAmbience)
  {
    const int step = Playing({.path = "assets://sounds/tone.wav", .looping = true});
    const int hum = Playing({.path = "assets://sounds/quiet.wav", .looping = true, .group = neon::sound_group::ambience});

    _audio.StopGroup(neon::sound_group::effects);

    EXPECT_NEAR(LevelAfter(0.1), 0.25f, 0.02f);
    EXPECT_FALSE(_audio.IsPlaying(step));
    EXPECT_TRUE(_audio.IsPlaying(hum));
  }

  TEST_F(MaAudioSystemTest, StopsASoundOfAGroupThatIsNotThereWithTheEffects)
  {
    const int sound = Playing({.path = "assets://sounds/tone.wav", .looping = true, .group = "musik"});

    _audio.StopGroup(neon::sound_group::effects);

    EXPECT_FALSE(_audio.IsPlaying(sound));
  }

  TEST_F(MaAudioSystemTest, ReportsStoppingAGroupThatIsNotThere)
  {
    const int sound = Playing({.path = "assets://sounds/tone.wav", .looping = true});

    _audio.StopGroup("musik");

    EXPECT_TRUE(_audio.IsPlaying(sound));
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "The sounds of group musik cannot be stopped, as there is no such group"));
  }

  // Playing and ending

  TEST_F(MaAudioSystemTest, DoesNotPlayASoundThatWasOnlyCreated)
  {
    const int sound = _audio.CreateSound({.path = "assets://sounds/tone.wav"});

    EXPECT_FALSE(_audio.IsPlaying(sound));
  }

  TEST_F(MaAudioSystemTest, PlaysASoundUntilItHasEnded)
  {
    const int sound = Playing({.path = "assets://sounds/short.wav"});

    LevelAfter(0.1);
    EXPECT_TRUE(_audio.IsPlaying(sound));

    LevelAfter(0.2);
    EXPECT_FALSE(_audio.IsPlaying(sound));
    EXPECT_EQ(LevelAfter(0.1), 0.0f);
  }

  TEST_F(MaAudioSystemTest, PlaysASoundThatLoopsOnAndOn)
  {
    const int sound = Playing({.path = "assets://sounds/short.wav", .looping = true});

    LevelAfter(0.5);
    LevelAfter(0.5);

    EXPECT_TRUE(_audio.IsPlaying(sound));
    EXPECT_GT(LevelAfter(0.1), 0.4f);
  }

  TEST_F(MaAudioSystemTest, LoopsASoundThatIsToldToLoopWhileItPlays)
  {
    const int sound = Playing({.path = "assets://sounds/short.wav"});

    _audio.SetLooping(sound, true);
    LevelAfter(0.5);

    EXPECT_TRUE(_audio.IsPlaying(sound));
  }

  TEST_F(MaAudioSystemTest, StopsASound)
  {
    const int sound = Playing({.path = "assets://sounds/tone.wav"});
    LevelAfter(0.1);

    _audio.Stop(sound);

    EXPECT_FALSE(_audio.IsPlaying(sound));
    EXPECT_EQ(LevelAfter(0.1), 0.0f);
  }

  TEST_F(MaAudioSystemTest, PlaysASoundAgainFromItsStart)
  {
    const int sound = Playing({.path = "assets://sounds/short.wav"});
    LevelAfter(0.15);

    // 0.05 seconds were left. From the start it plays for 0.2 again
    _audio.Play(sound);
    LevelAfter(0.1);

    EXPECT_TRUE(_audio.IsPlaying(sound));
  }

  TEST_F(MaAudioSystemTest, PlaysASoundFasterWithAHigherPitch)
  {
    const int fast = Playing({.path = "assets://sounds/short.wav", .pitch = 2.0f});
    const int normal = Playing({.path = "assets://sounds/short.wav"});

    LevelAfter(0.15);

    // twice as fast, 0.2 seconds take 0.1
    EXPECT_FALSE(_audio.IsPlaying(fast));
    EXPECT_TRUE(_audio.IsPlaying(normal));
  }

  TEST_F(MaAudioSystemTest, KeepsThePitchWhenToldAPitchOfZero)
  {
    const int sound = Playing({.path = "assets://sounds/short.wav", .pitch = 2.0f});

    _audio.SetPitch(sound, 0.0f);
    LevelAfter(0.15);

    EXPECT_FALSE(_audio.IsPlaying(sound));
  }

  // Distance

  TEST_F(MaAudioSystemTest, PlaysASoundWithAPlaceQuieterFurtherAway)
  {
    _audio.SetListener(ListenerInfo{});
    const SoundInfo hum{.path = "assets://sounds/tone.wav", .looping = true, .spatial = true, .max_distance = 100.0f};

    const int sound = Playing(hum);
    _audio.SetPosition(sound, {0.0f, 0.0f, -1.0f}, glm::vec3{0.0f});
    const float near = LevelAfter(0.2);

    _audio.SetPosition(sound, {0.0f, 0.0f, -20.0f}, glm::vec3{0.0f});
    const float far = LevelAfter(0.2);

    EXPECT_GT(near, 0.1f);
    EXPECT_LT(far, near / 4.0f);
  }

  TEST_F(MaAudioSystemTest, PlaysASoundWithoutAPlaceTheSameEverywhere)
  {
    _audio.SetListener(ListenerInfo{});
    const int sound = Playing({.path = "assets://sounds/tone.wav", .looping = true});

    _audio.SetPosition(sound, {0.0f, 0.0f, -1.0f}, glm::vec3{0.0f});
    const float near = LevelAfter(0.2);

    _audio.SetPosition(sound, {0.0f, 0.0f, -20.0f}, glm::vec3{0.0f});
    const float far = LevelAfter(0.2);

    EXPECT_NEAR(far, near, 0.01f);
  }

  TEST_F(MaAudioSystemTest, PlaysASoundWithAPlaceAsLoudAsItGetsWithinItsLeastDistance)
  {
    _audio.SetListener(ListenerInfo{});
    const int sound = Playing({
      .path = "assets://sounds/tone.wav", .looping = true, .spatial = true, .min_distance = 30.0f, .max_distance = 100.0f
    });

    _audio.SetPosition(sound, {0.0f, 0.0f, -1.0f}, glm::vec3{0.0f});
    const float near = LevelAfter(0.2);

    _audio.SetPosition(sound, {0.0f, 0.0f, -20.0f}, glm::vec3{0.0f});
    const float far = LevelAfter(0.2);

    EXPECT_NEAR(far, near, 0.02f);
  }

  TEST_F(MaAudioSystemTest, ASoundWithAnInverseFalloffIsStillHeardPastItsMostDistance)
  {
    _audio.SetListener(ListenerInfo{});
    const int sound = Playing({
      .path = "assets://sounds/tone.wav", .looping = true, .spatial = true, .min_distance = 1.0f, .max_distance = 10.0f
    });

    _audio.SetPosition(sound, {0.0f, 0.0f, -10.0f}, glm::vec3{0.0f});
    const float at_the_most = LevelAfter(0.2);

    _audio.SetPosition(sound, {0.0f, 0.0f, -40.0f}, glm::vec3{0.0f});
    const float far = LevelAfter(0.2);

    EXPECT_GT(far, 0.0f);
    EXPECT_NEAR(far, at_the_most, 0.01f);
  }

  TEST_F(MaAudioSystemTest, ASoundWithALinearFalloffIsSilentFromItsMostDistanceOn)
  {
    _audio.SetListener(ListenerInfo{});
    const int sound = Playing({
      .path = "assets://sounds/tone.wav",
      .looping = true,
      .spatial = true,
      .min_distance = 1.0f,
      .max_distance = 10.0f,
      .falloff = neon::SoundFalloff::Linear
    });

    _audio.SetPosition(sound, {0.0f, 0.0f, -1.0f}, glm::vec3{0.0f});
    const float near = LevelAfter(0.2);

    _audio.SetPosition(sound, {0.0f, 0.0f, -5.5f}, glm::vec3{0.0f});
    const float halfway = LevelAfter(0.2);

    _audio.SetPosition(sound, {0.0f, 0.0f, -40.0f}, glm::vec3{0.0f});
    LevelAfter(0.2);
    const float far = LevelAfter(0.2);

    EXPECT_GT(near, 0.1f);
    EXPECT_LT(halfway, near);
    EXPECT_GT(halfway, 0.0f);
    EXPECT_NEAR(far, 0.0f, 0.001f);
  }

  // Time

  TEST_F(MaAudioSystemTest, MixesNothingWhenNoTimePasses)
  {
    const int sound = Playing({.path = "assets://sounds/short.wav"});

    _audio.Advance(0.0);
    _audio.Advance(-1.0);

    // nothing of the sound was used up
    LevelAfter(0.15);
    EXPECT_TRUE(_audio.IsPlaying(sound));
  }

  TEST_F(MaAudioSystemTest, GivesTheSameLevelsInEveryRun)
  {
    const auto run = [this]
    {
      _audio.CleanUp();
      _audio.Initialize();
      _audio.SetListener(ListenerInfo{});

      const int sound = Playing({.path = "assets://sounds/tone.wav", .looping = true, .spatial = true});
      std::vector<float> levels;
      for (int i = 0; i < 30; i++)
      {
        _audio.SetPosition(sound, {0.0f, 0.0f, -1.0f - 0.5f * static_cast<float>(i)}, {0.0f, 0.0f, -30.0f});
        levels.push_back(LevelAfter(frame));
      }
      return levels;
    };

    EXPECT_EQ(run(), run());
  }

  // Cleaning up

  TEST_F(MaAudioSystemTest, ForgetsASoundThatIsDestroyed)
  {
    const int sound = Playing({.path = "assets://sounds/tone.wav"});

    _audio.DestroySound(sound);

    EXPECT_FALSE(_audio.IsPlaying(sound));
    EXPECT_EQ(LevelAfter(0.1), 0.0f);
  }

  TEST_F(MaAudioSystemTest, PlaysTheOtherSoundOfAFileWhenOneIsDestroyed)
  {
    const int first = Playing({.path = "assets://sounds/tone.wav"});
    const int second = Playing({.path = "assets://sounds/tone.wav"});

    _audio.DestroySound(first);

    EXPECT_TRUE(_audio.IsPlaying(second));
    EXPECT_GT(LevelAfter(0.1), 0.4f);
  }

  TEST_F(MaAudioSystemTest, IgnoresAnIdItDoesNotKnow)
  {
    constexpr int unknown = 42;

    EXPECT_NO_THROW({
      _audio.Play(unknown);
      _audio.Stop(unknown);
      _audio.SetVolume(unknown, 0.5f);
      _audio.SetPitch(unknown, 2.0f);
      _audio.SetLooping(unknown, true);
      _audio.SetPosition(unknown, glm::vec3{1.0f}, glm::vec3{0.0f});
      _audio.FadeIn(unknown, 1.0);
      _audio.FadeTo(unknown, 0.5f, 1.0);
      _audio.FadeOut(unknown, 1.0);
      _audio.DestroySound(unknown);
    });
    EXPECT_FALSE(_audio.IsPlaying(unknown));
  }

  TEST_F(MaAudioSystemTest, ForgetsEverySoundWhenCleanedUp)
  {
    const int sound = Playing({.path = "assets://sounds/tone.wav"});

    _audio.CleanUp();

    EXPECT_FALSE(_audio.IsPlaying(sound));
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Cleaning up miniaudio"));
  }

  TEST_F(MaAudioSystemTest, CanBeInitializedAgainAfterItWasCleanedUp)
  {
    _audio.CleanUp();
    _audio.Initialize();

    Playing({.path = "assets://sounds/tone.wav"});

    EXPECT_GT(LevelAfter(0.1), 0.4f);
  }
} // namespace
