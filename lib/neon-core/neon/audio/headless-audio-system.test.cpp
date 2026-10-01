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
      _audio.FadeIn(unknown, 1.0);
      _audio.FadeTo(unknown, 0.5f, 1.0);
      _audio.FadeOut(unknown, 1.0);
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

  // Fading

  TEST_F(HeadlessAudioSystemTest, PlaysASoundThatFadesIn)
  {
    const int sound = _audio.CreateSound({.path = "assets://sounds/theme.ogg", .looping = true});

    _audio.FadeIn(sound, 2.0);

    EXPECT_TRUE(_audio.IsPlaying(sound));
  }

  TEST_F(HeadlessAudioSystemTest, StopsASoundWhenItHasFadedOut)
  {
    const int sound = _audio.CreateSound({.path = "assets://sounds/theme.ogg", .looping = true});
    _audio.Play(sound);

    _audio.FadeOut(sound, 0.5);
    _audio.Advance(0.25);
    EXPECT_TRUE(_audio.IsPlaying(sound));

    _audio.Advance(0.25);
    EXPECT_FALSE(_audio.IsPlaying(sound));
  }

  TEST_F(HeadlessAudioSystemTest, StopsASoundThatFadesOutInNoTimeAtOnce)
  {
    const int sound = _audio.CreateSound({.path = "assets://sounds/theme.ogg", .looping = true});
    _audio.Play(sound);

    _audio.FadeOut(sound, 0.0);

    EXPECT_FALSE(_audio.IsPlaying(sound));
  }

  TEST_F(HeadlessAudioSystemTest, KeepsPlayingASoundThatFadesToSilence)
  {
    const int sound = _audio.CreateSound({.path = "assets://sounds/theme.ogg", .looping = true});
    _audio.Play(sound);

    _audio.FadeTo(sound, 0.0f, 0.5);
    _audio.Advance(1.0);

    EXPECT_TRUE(_audio.IsPlaying(sound));
  }

  TEST_F(HeadlessAudioSystemTest, PutsOffAFadeOutWhenTheSoundIsPlayedOrFadedAgain)
  {
    const int played = _audio.CreateSound({.path = "assets://sounds/theme.ogg", .looping = true});
    const int faded = _audio.CreateSound({.path = "assets://sounds/theme.ogg", .looping = true});
    _audio.Play(played);
    _audio.Play(faded);
    _audio.FadeOut(played, 0.5);
    _audio.FadeOut(faded, 0.5);

    _audio.Play(played);
    _audio.FadeTo(faded, 1.0f, 0.5);
    _audio.Advance(1.0);

    EXPECT_TRUE(_audio.IsPlaying(played));
    EXPECT_TRUE(_audio.IsPlaying(faded));
  }

  TEST_F(HeadlessAudioSystemTest, CrossfadesFromOneSoundToAnother)
  {
    const int first = _audio.CreateSound({.path = "assets://sounds/first.ogg", .looping = true});
    const int second = _audio.CreateSound({.path = "assets://sounds/second.ogg", .looping = true});
    _audio.Play(first);

    _audio.Crossfade(first, second, 1.0);
    EXPECT_TRUE(_audio.IsPlaying(first));
    EXPECT_TRUE(_audio.IsPlaying(second));

    _audio.Advance(1.0);
    EXPECT_FALSE(_audio.IsPlaying(first));
    EXPECT_TRUE(_audio.IsPlaying(second));
  }

  // Groups

  TEST_F(HeadlessAudioSystemTest, HasMusicEffectsAndVoicesFromTheStart)
  {
    EXPECT_EQ(_audio.GetGroupVolume(neon::sound_group::music), 1.0f);
    EXPECT_EQ(_audio.GetGroupVolume(neon::sound_group::effects), 1.0f);
    EXPECT_EQ(_audio.GetGroupVolume(neon::sound_group::voices), 1.0f);
  }

  TEST_F(HeadlessAudioSystemTest, KeepsTheVolumeOfAGroup)
  {
    _audio.SetGroupVolume(neon::sound_group::music, 0.25f);

    EXPECT_EQ(_audio.GetGroupVolume(neon::sound_group::music), 0.25f);
    EXPECT_EQ(_audio.GetGroupVolume(neon::sound_group::effects), 1.0f);
  }

  TEST_F(HeadlessAudioSystemTest, TakesNoVolumeOfAGroupBelowSilence)
  {
    _audio.SetGroupVolume(neon::sound_group::voices, -1.0f);

    EXPECT_EQ(_audio.GetGroupVolume(neon::sound_group::voices), 0.0f);
  }

  TEST_F(HeadlessAudioSystemTest, AddsAGroupOfAGame)
  {
    _audio.AddGroup("ambience");
    EXPECT_EQ(_audio.GetGroupVolume("ambience"), 1.0f);

    _audio.SetGroupVolume("ambience", 0.5f);
    _audio.AddGroup("ambience");
    EXPECT_EQ(_audio.GetGroupVolume("ambience"), 0.5f);
  }

  TEST_F(HeadlessAudioSystemTest, ReportsTheVolumeOfAGroupThatIsNotThere)
  {
    _audio.SetGroupVolume("musik", 0.5f);

    EXPECT_EQ(_audio.GetGroupVolume("musik"), 0.0f);
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "The volume of group musik cannot be set"));
  }

  TEST_F(HeadlessAudioSystemTest, ReportsASoundOfAGroupThatIsNotThere)
  {
    EXPECT_GE(_audio.CreateSound({.path = "assets://sounds/theme.ogg", .group = "musik"}), 0);
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "Sound assets://sounds/theme.ogg is of group musik, which is not there"));
  }

  TEST_F(HeadlessAudioSystemTest, KeepsTheVolumesOfTheGroupsWhenCleanedUp)
  {
    _audio.AddGroup("ambience");
    _audio.SetGroupVolume(neon::sound_group::music, 0.25f);

    _audio.CleanUp();
    _audio.Initialize();

    EXPECT_EQ(_audio.GetGroupVolume(neon::sound_group::music), 0.25f);
    EXPECT_EQ(_audio.GetGroupVolume("ambience"), 1.0f);
  }

  TEST_F(HeadlessAudioSystemTest, ForgetsEverySoundWhenCleanedUp)
  {
    const int sound = _audio.CreateSound({.path = "assets://sounds/hum.wav", .looping = true});
    _audio.Play(sound);

    _audio.CleanUp();

    EXPECT_FALSE(_audio.IsPlaying(sound));
  }
} // namespace
