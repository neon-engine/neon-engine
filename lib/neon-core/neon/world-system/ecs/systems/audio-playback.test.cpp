#include "audio-playback.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/mock-audio-context.hpp>
#include <neon/world-system/ecs/components/sound-listener.hpp>
#include <neon/world-system/ecs/components/sound-source.hpp>

namespace
{
  using neon::AudioPlayback;
  using neon::Entity;
  using neon::ListenerInfo;
  using neon::SoundInfo;
  using neon::SoundListener;
  using neon::SoundSource;
  using neon::Transform;
  using neon::testing::FakeEntityStore;
  using neon::testing::MockAudioContext;
  using ::testing::_;
  using ::testing::Field;
  using ::testing::NiceMock;
  using ::testing::Return;
  using ::testing::SaveArg;

  constexpr int sound_id = 7;

  /// A transform that was placed in the world at a position.
  Transform PlacedAt(const glm::vec3 &position)
  {
    Transform transform;
    transform.position = position;
    transform.world_coordinates[3] = glm::vec4(position, 1.0f);
    return transform;
  }

  class AudioPlaybackTest : public ::testing::Test
  {
  protected:
    // the store goes before the audio, as it releases sounds when it does
    NiceMock<MockAudioContext> _audio;
    FakeEntityStore _store;
    AudioPlayback _system{&_audio};

    void SetUp() override
    {
      ON_CALL(_audio, CreateSound(_)).WillByDefault(Return(sound_id));
      ON_CALL(_audio, IsPlaying(_)).WillByDefault(Return(true));

      _store.Initialize();
      _store.Register<Transform>("Transform");
      _system.Initialize(_store);
    }

    Entity CreateSource(const SoundSource &source, const Transform &transform = {})
    {
      const Entity entity = _store.CreateEntity("");
      _store.Set(entity, transform);
      _store.Set(entity, source);
      return entity;
    }

    static SoundSource Source(const std::string &path = "assets://sounds/hum.wav")
    {
      SoundSource source;
      source.sound.path = path;
      return source;
    }

    SoundSource &SourceOf(const Entity entity)
    {
      return *_store.Get<SoundSource>(entity);
    }
  };

  // Initialize

  TEST_F(AudioPlaybackTest, RegistersTheComponentsOfSound)
  {
    EXPECT_NO_THROW((void) (_store.Query<Transform, SoundSource>()));
    EXPECT_NO_THROW((void) (_store.Query<Transform, SoundListener>()));
  }

  // Sources

  TEST_F(AudioPlaybackTest, CreatesTheSoundOfASourceFromWhatItSays)
  {
    auto source = Source("assets://sounds/step.wav");
    source.sound.looping = true;
    CreateSource(source);

    EXPECT_CALL(_audio, CreateSound(Field(&SoundInfo::path, "assets://sounds/step.wav"))).WillOnce(Return(sound_id));

    _system.Update(_store, 0.016);
  }

  TEST_F(AudioPlaybackTest, CreatesTheSoundOfASourceInItsGroup)
  {
    auto source = Source("assets://sounds/theme.ogg");
    source.sound.group = neon::sound_group::music;
    CreateSource(source);

    EXPECT_CALL(_audio, CreateSound(Field(&SoundInfo::group, "music"))).WillOnce(Return(sound_id));

    _system.Update(_store, 0.016);
  }

  TEST_F(AudioPlaybackTest, CreatesTheSoundOfASourceOnce)
  {
    CreateSource(Source());

    EXPECT_CALL(_audio, CreateSound(_)).Times(1).WillOnce(Return(sound_id));

    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);
  }

  TEST_F(AudioPlaybackTest, PlaysASourceOnceWhenItJoinsTheWorld)
  {
    const Entity entity = CreateSource(Source());

    EXPECT_CALL(_audio, Play(sound_id)).Times(1);

    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);

    EXPECT_EQ(SourceOf(entity).sound_id, sound_id);
  }

  TEST_F(AudioPlaybackTest, DoesNotPlayASourceThatIsNotToPlay)
  {
    auto source = Source();
    source.playing = false;
    CreateSource(source);

    EXPECT_CALL(_audio, Play(_)).Times(0);

    _system.Update(_store, 0.016);
  }

  TEST_F(AudioPlaybackTest, StopsASourceWhenTheGameSaysSo)
  {
    const Entity entity = CreateSource(Source());
    _system.Update(_store, 0.016);

    EXPECT_CALL(_audio, Stop(sound_id)).Times(1);

    SourceOf(entity).playing = false;
    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);
  }

  TEST_F(AudioPlaybackTest, PlaysASourceAgainWhenTheGameSaysSo)
  {
    const Entity entity = CreateSource(Source());
    _system.Update(_store, 0.016);
    SourceOf(entity).playing = false;
    _system.Update(_store, 0.016);

    EXPECT_CALL(_audio, Play(sound_id)).Times(1);

    SourceOf(entity).playing = true;
    _system.Update(_store, 0.016);
  }

  TEST_F(AudioPlaybackTest, SaysASourceHasStoppedWhenItsSoundHasEnded)
  {
    const Entity entity = CreateSource(Source());
    _system.Update(_store, 0.016);

    ON_CALL(_audio, IsPlaying(sound_id)).WillByDefault(Return(false));
    _system.Update(_store, 0.016);

    EXPECT_FALSE(SourceOf(entity).playing);
  }

  TEST_F(AudioPlaybackTest, DoesNotStopASoundThatHasEnded)
  {
    CreateSource(Source());
    _system.Update(_store, 0.016);
    ON_CALL(_audio, IsPlaying(sound_id)).WillByDefault(Return(false));

    // it has stopped by itself
    EXPECT_CALL(_audio, Stop(_)).Times(0);

    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);
  }

  TEST_F(AudioPlaybackTest, TriesASoundThatCannotBeCreatedOnlyOnce)
  {
    const Entity entity = CreateSource(Source("assets://sounds/missing.wav"));

    EXPECT_CALL(_audio, CreateSound(_)).Times(1).WillOnce(Return(-1));
    EXPECT_CALL(_audio, Play(_)).Times(0);

    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);

    EXPECT_EQ(SourceOf(entity).sound_id, AudioPlayback::failed_sound);
    EXPECT_FALSE(SourceOf(entity).playing);
  }

  TEST_F(AudioPlaybackTest, HandsTheVolumePitchAndLoopingOfASourceToTheAudioEveryFrame)
  {
    const Entity entity = CreateSource(Source());
    _system.Update(_store, 0.016);

    EXPECT_CALL(_audio, SetVolume(sound_id, 0.25f));
    EXPECT_CALL(_audio, SetPitch(sound_id, 1.5f));
    EXPECT_CALL(_audio, SetLooping(sound_id, true));

    auto &source = SourceOf(entity);
    source.sound.volume = 0.25f;
    source.sound.pitch = 1.5f;
    source.sound.looping = true;
    _system.Update(_store, 0.016);
  }

  TEST_F(AudioPlaybackTest, PlacesASourceWithAPlaceWhereItsEntityIs)
  {
    auto source = Source();
    source.sound.spatial = true;
    CreateSource(source, PlacedAt({1.0f, 2.0f, 3.0f}));

    EXPECT_CALL(_audio, SetPosition(sound_id, glm::vec3(1.0f, 2.0f, 3.0f), glm::vec3(0.0f)));

    _system.Update(_store, 0.016);
  }

  TEST_F(AudioPlaybackTest, DoesNotPlaceASourceWithoutAPlace)
  {
    CreateSource(Source(), PlacedAt({1.0f, 2.0f, 3.0f}));

    EXPECT_CALL(_audio, SetPosition(_, _, _)).Times(0);

    _system.Update(_store, 0.016);
  }

  TEST_F(AudioPlaybackTest, GivesASourceThatMovedTheSpeedItMovedAt)
  {
    auto source = Source();
    source.sound.spatial = true;
    const Entity entity = CreateSource(source, PlacedAt({0.0f, 0.0f, 0.0f}));
    _system.Update(_store, 0.5);

    // 2 units in half a second
    EXPECT_CALL(_audio, SetPosition(sound_id, glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(4.0f, 0.0f, 0.0f)));

    *_store.Get<Transform>(entity) = PlacedAt({2.0f, 0.0f, 0.0f});
    _system.Update(_store, 0.5);
  }

  TEST_F(AudioPlaybackTest, GivesASourceThatMovedNoSpeedWhenNoTimePassed)
  {
    auto source = Source();
    source.sound.spatial = true;
    const Entity entity = CreateSource(source, PlacedAt({0.0f, 0.0f, 0.0f}));
    _system.Update(_store, 0.5);

    EXPECT_CALL(_audio, SetPosition(sound_id, glm::vec3(2.0f, 0.0f, 0.0f), glm::vec3(0.0f)));

    *_store.Get<Transform>(entity) = PlacedAt({2.0f, 0.0f, 0.0f});
    _system.Update(_store, 0.0);
  }

  TEST_F(AudioPlaybackTest, ReleasesTheSoundOfAnEntityThatStopsBeingASource)
  {
    const Entity entity = CreateSource(Source());
    _system.Update(_store, 0.016);

    EXPECT_CALL(_audio, DestroySound(sound_id)).Times(1);

    _store.Remove<SoundSource>(entity);
  }

  TEST_F(AudioPlaybackTest, ReleasesTheSoundOfAnEntityThatIsDestroyed)
  {
    const Entity entity = CreateSource(Source());
    _system.Update(_store, 0.016);

    EXPECT_CALL(_audio, DestroySound(sound_id)).Times(1);

    _store.DestroyEntity(entity);
  }

  TEST_F(AudioPlaybackTest, ReleasesNothingForASourceThatHadNoSoundYet)
  {
    const Entity entity = CreateSource(Source());

    EXPECT_CALL(_audio, DestroySound(_)).Times(0);

    _store.DestroyEntity(entity);
  }

  TEST_F(AudioPlaybackTest, ReleasesNothingForASoundThatCouldNotBeCreated)
  {
    ON_CALL(_audio, CreateSound(_)).WillByDefault(Return(-1));
    const Entity entity = CreateSource(Source("assets://sounds/missing.wav"));
    _system.Update(_store, 0.016);

    EXPECT_CALL(_audio, DestroySound(_)).Times(0);

    _store.DestroyEntity(entity);
  }

  // Fading

  TEST_F(AudioPlaybackTest, FadesInASourceInPlaceOfPlayingIt)
  {
    auto source = Source();
    source.playing = false;
    const Entity entity = CreateSource(source);
    _system.Update(_store, 0.016);

    EXPECT_CALL(_audio, FadeIn(sound_id, 2.0)).Times(1);
    EXPECT_CALL(_audio, Play(_)).Times(0);

    SourceOf(entity).FadeIn(2.0);
    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);

    EXPECT_TRUE(SourceOf(entity).playing);
    EXPECT_FALSE(SourceOf(entity).fade.has_value());
  }

  TEST_F(AudioPlaybackTest, FadesInASourceThatJoinsTheWorld)
  {
    auto source = Source();
    source.FadeIn(2.0);
    CreateSource(source);

    EXPECT_CALL(_audio, FadeIn(sound_id, 2.0)).Times(1);
    EXPECT_CALL(_audio, Play(_)).Times(0);

    _system.Update(_store, 0.016);
  }

  TEST_F(AudioPlaybackTest, FadesASourceThatPlaysToAVolume)
  {
    const Entity entity = CreateSource(Source());
    _system.Update(_store, 0.016);

    EXPECT_CALL(_audio, FadeTo(sound_id, 0.25f, 1.5)).Times(1);

    SourceOf(entity).FadeTo(0.25f, 1.5);
    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);
  }

  TEST_F(AudioPlaybackTest, FadesASourceAfterPlayingItInTheSameFrame)
  {
    auto source = Source();
    source.FadeTo(0.5f, 1.0);
    CreateSource(source);

    // playing starts a sound at the volume it has, which would undo a fade
    // that came first
    ::testing::InSequence in_order;
    EXPECT_CALL(_audio, Play(sound_id));
    EXPECT_CALL(_audio, FadeTo(sound_id, 0.5f, 1.0));

    _system.Update(_store, 0.016);
  }

  TEST_F(AudioPlaybackTest, FadesASourceOutAndSaysItStoppedOnceItHas)
  {
    const Entity entity = CreateSource(Source());
    _system.Update(_store, 0.016);

    EXPECT_CALL(_audio, FadeOut(sound_id, 1.0)).Times(1);
    EXPECT_CALL(_audio, Stop(_)).Times(0);

    // it plays on while it fades
    SourceOf(entity).FadeOut(1.0);
    _system.Update(_store, 0.016);
    EXPECT_TRUE(SourceOf(entity).playing);

    ON_CALL(_audio, IsPlaying(sound_id)).WillByDefault(Return(false));
    _system.Update(_store, 0.016);
    EXPECT_FALSE(SourceOf(entity).playing);
  }

  TEST_F(AudioPlaybackTest, LeavesASourceThatDoesNotPlayAloneWhenToldToFade)
  {
    auto source = Source();
    source.playing = false;
    const Entity entity = CreateSource(source);
    _system.Update(_store, 0.016);

    EXPECT_CALL(_audio, FadeOut(_, _)).Times(0);
    EXPECT_CALL(_audio, FadeTo(_, _, _)).Times(0);

    SourceOf(entity).FadeOut(1.0);
    _system.Update(_store, 0.016);
    SourceOf(entity).FadeTo(0.5f, 1.0);
    _system.Update(_store, 0.016);

    EXPECT_FALSE(SourceOf(entity).fade.has_value());
  }

  TEST_F(AudioPlaybackTest, CrossfadesFromOneSourceToAnother)
  {
    constexpr int first_id = 1;
    constexpr int second_id = 2;
    ON_CALL(_audio, CreateSound(Field(&SoundInfo::path, "assets://sounds/first.ogg"))).WillByDefault(Return(first_id));
    ON_CALL(_audio, CreateSound(Field(&SoundInfo::path, "assets://sounds/second.ogg"))).WillByDefault(Return(second_id));

    const Entity first = CreateSource(Source("assets://sounds/first.ogg"));
    auto waiting = Source("assets://sounds/second.ogg");
    waiting.playing = false;
    const Entity second = CreateSource(waiting);
    _system.Update(_store, 0.016);

    EXPECT_CALL(_audio, FadeOut(first_id, 3.0)).Times(1);
    EXPECT_CALL(_audio, FadeIn(second_id, 3.0)).Times(1);

    neon::Crossfade(SourceOf(first), SourceOf(second), 3.0);
    _system.Update(_store, 0.016);

    EXPECT_TRUE(SourceOf(second).playing);
  }

  TEST_F(AudioPlaybackTest, ForgetsTheFadeOfASoundThatCannotBeCreated)
  {
    ON_CALL(_audio, CreateSound(_)).WillByDefault(Return(-1));
    auto source = Source("assets://sounds/missing.wav");
    source.FadeIn(1.0);
    const Entity entity = CreateSource(source);

    EXPECT_CALL(_audio, FadeIn(_, _)).Times(0);

    _system.Update(_store, 0.016);

    EXPECT_FALSE(SourceOf(entity).fade.has_value());
  }

  // Listener

  TEST_F(AudioPlaybackTest, HearsTheWorldFromTheListener)
  {
    const Entity entity = _store.CreateEntity("camera");
    _store.Set(entity, PlacedAt({0.0f, 1.0f, 2.0f}));
    _store.Set(entity, SoundListener{.volume = 0.5f});

    ListenerInfo listener;
    EXPECT_CALL(_audio, SetListener(_)).WillOnce(SaveArg<0>(&listener));
    EXPECT_CALL(_audio, SetMasterVolume(0.5f));

    _system.Update(_store, 0.016);

    EXPECT_EQ(listener.position, glm::vec3(0.0f, 1.0f, 2.0f));
    EXPECT_EQ(listener.forward, glm::vec3(0.0f, 0.0f, -1.0f));
    EXPECT_EQ(listener.up, glm::vec3(0.0f, 1.0f, 0.0f));
    EXPECT_EQ(listener.velocity, glm::vec3(0.0f));
  }

  TEST_F(AudioPlaybackTest, GivesAListenerThatMovedTheSpeedItMovedAt)
  {
    const Entity entity = _store.CreateEntity("camera");
    _store.Set(entity, PlacedAt({0.0f, 0.0f, 0.0f}));
    _store.Set(entity, SoundListener{});
    _system.Update(_store, 0.5);

    ListenerInfo listener;
    EXPECT_CALL(_audio, SetListener(_)).WillOnce(SaveArg<0>(&listener));

    *_store.Get<Transform>(entity) = PlacedAt({0.0f, 0.0f, -1.0f});
    _system.Update(_store, 0.5);

    EXPECT_EQ(listener.velocity, glm::vec3(0.0f, 0.0f, -2.0f));
  }

  TEST_F(AudioPlaybackTest, LeavesTheListenerAsItIsWithoutAnEntityToHearFrom)
  {
    EXPECT_CALL(_audio, SetListener(_)).Times(0);
    EXPECT_CALL(_audio, SetMasterVolume(_)).Times(0);

    _system.Update(_store, 0.016);
  }

  // Time

  TEST_F(AudioPlaybackTest, AdvancesTheAudioByTheTimeOfTheFrame)
  {
    EXPECT_CALL(_audio, Advance(0.25)).Times(1);

    _system.Update(_store, 0.25);
  }

  TEST_F(AudioPlaybackTest, AdvancesTheAudioAfterItWasToldWhatChanged)
  {
    CreateSource(Source());

    ::testing::InSequence in_order;
    EXPECT_CALL(_audio, CreateSound(_)).WillOnce(Return(sound_id));
    EXPECT_CALL(_audio, Play(sound_id));
    EXPECT_CALL(_audio, Advance(_));

    _system.Update(_store, 0.016);
  }
} // namespace
