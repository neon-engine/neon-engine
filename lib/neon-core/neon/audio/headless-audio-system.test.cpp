#include "headless-audio-system.hpp"

#include <memory>

#include <gtest/gtest.h>

#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::Headless_AudioSystem;
  using neon::SoundInfo;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;

  class HeadlessAudioSystemTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();

    // it reads no files, so it has no file system
    Headless_AudioSystem _audio{SettingsConfig{}, nullptr, _logger};

    void SetUp() override
    {
      _audio.Initialize();
    }

    void TearDown() override
    {
      _audio.CleanUp();
    }
  };

  TEST_F(HeadlessAudioSystemTest, SaysWhenItStartsAndStops)
  {
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Initializing headless audio system"));

    _audio.CleanUp();
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Cleaning up headless audio system"));
  }

  TEST_F(HeadlessAudioSystemTest, CreatesASoundOfAFileThatIsNotThere)
  {
    // nothing is read, so nothing can be missing
    EXPECT_GE(_audio.CreateSound({.path = "assets://sounds/missing.wav"}), 0);
  }

  TEST_F(HeadlessAudioSystemTest, GivesEverySoundAnIdOfItsOwn)
  {
    const int first = _audio.CreateSound({.path = "assets://sounds/step.wav"});
    const int second = _audio.CreateSound({.path = "assets://sounds/step.wav"});

    EXPECT_NE(first, second);
  }

  TEST_F(HeadlessAudioSystemTest, DoesNotPlayASoundThatWasOnlyCreated)
  {
    const int sound = _audio.CreateSound({.path = "assets://sounds/step.wav"});

    EXPECT_FALSE(_audio.IsPlaying(sound));
  }

  TEST_F(HeadlessAudioSystemTest, PlaysASoundUntilTheNextFrame)
  {
    const int sound = _audio.CreateSound({.path = "assets://sounds/step.wav"});

    _audio.Play(sound);
    EXPECT_TRUE(_audio.IsPlaying(sound));

    // a sound has no length here, and has ended once time has passed
    _audio.Advance(0.016);
    EXPECT_FALSE(_audio.IsPlaying(sound));
  }

  TEST_F(HeadlessAudioSystemTest, PlaysASoundThatLoopsUntilItIsStopped)
  {
    const int sound = _audio.CreateSound({.path = "assets://sounds/hum.wav", .looping = true});

    _audio.Play(sound);
    _audio.Advance(0.016);
    _audio.Advance(0.016);
    EXPECT_TRUE(_audio.IsPlaying(sound));

    _audio.Stop(sound);
    EXPECT_FALSE(_audio.IsPlaying(sound));
  }

  TEST_F(HeadlessAudioSystemTest, LoopsASoundThatIsToldToLoopLater)
  {
    const int sound = _audio.CreateSound({.path = "assets://sounds/hum.wav"});

    _audio.SetLooping(sound, true);
    _audio.Play(sound);
    _audio.Advance(0.016);

    EXPECT_TRUE(_audio.IsPlaying(sound));
  }

  TEST_F(HeadlessAudioSystemTest, ForgetsASoundThatIsDestroyed)
  {
    const int sound = _audio.CreateSound({.path = "assets://sounds/hum.wav", .looping = true});
    _audio.Play(sound);

    _audio.DestroySound(sound);

    EXPECT_FALSE(_audio.IsPlaying(sound));
  }

  TEST_F(HeadlessAudioSystemTest, IgnoresAnIdItDoesNotKnow)
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

  TEST_F(HeadlessAudioSystemTest, IsAlwaysSilent)
  {
    const int sound = _audio.CreateSound({.path = "assets://sounds/hum.wav", .looping = true});
    _audio.Play(sound);
    _audio.Advance(0.5);

    EXPECT_EQ(_audio.GetOutputLevel(), 0.0f);
  }

  TEST_F(HeadlessAudioSystemTest, ForgetsEverySoundWhenCleanedUp)
  {
    const int sound = _audio.CreateSound({.path = "assets://sounds/hum.wav", .looping = true});
    _audio.Play(sound);

    _audio.CleanUp();

    EXPECT_FALSE(_audio.IsPlaying(sound));
  }
} // namespace
