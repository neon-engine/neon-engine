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
