#include "ui-audio.hpp"

#include <map>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/mock-audio-context.hpp>
#include <neon/testing/mock-ui-system.hpp>
#include <neon/world-system/ecs/components/sound-source.hpp>
#include <neon/world-system/ecs/components/ui-sound-switch.hpp>
#include <neon/world-system/ecs/components/ui-volume.hpp>

namespace
{
  using neon::Entity;
  using neon::SoundFade;
  using neon::SoundSource;
  using neon::UiAudio;
  using neon::UiSoundSwitch;
  using neon::UiVolume;
  using neon::testing::FakeEntityStore;
  using neon::testing::MockAudioContext;
  using neon::testing::MockUiContext;
  using ::testing::_;
  using ::testing::FloatEq;
  using ::testing::NiceMock;

  class UiAudioTest : public ::testing::Test
  {
  protected:
    NiceMock<MockAudioContext> _audio;
    NiceMock<MockUiContext> _ui;
    FakeEntityStore _store;
    UiAudio _system{&_ui, &_audio};

    // the values of the user interface, as a player left them
    std::map<std::string, std::string> _values;

    void SetUp() override
    {
      ON_CALL(_ui, GetValue(_, _)).WillByDefault([this](const std::string &name, bool *is_set)
      {
        const auto found = _values.find(name);
        if (is_set != nullptr) { *is_set = found != _values.end(); }
        return found == _values.end() ? std::string{} : found->second;
      });

      // as the user interface reads a number: a text that holds none is no
      // number
      ON_CALL(_ui, GetNumber(_, _)).WillByDefault([this](const std::string &name, double &number)
      {
        const auto found = _values.find(name);
        if (found == _values.end()) { return false; }

        try { number = std::stod(found->second); } catch (const std::exception &) { return false; }
        return true;
      });

      // as the world does: every component is registered, by this system
      // and by AudioPlayback, before any system makes its queries
      _store.Initialize();
      _system.Register(_store);
      _store.Register<SoundSource>("SoundSource");
      _system.Initialize(_store);
    }

    void AddVolume(const UiVolume &volume)
    {
      _store.Set(_store.CreateEntity(""), volume);
    }

    Entity AddTrack(const std::string &name, const std::string &equals)
    {
      const Entity entity = _store.CreateEntity(name);
      SoundSource source;
      source.sound.path = "assets://sounds/" + name + ".wav";
      source.sound.group = "music";
      _store.Set(entity, source);
      _store.Set(entity, UiSoundSwitch{.value = "track", .equals = equals, .fade = 2.0f});
      return entity;
    }

    SoundSource &SourceOf(const Entity entity)
    {
      return *_store.Get<SoundSource>(entity);
    }
  };

  // UiVolume

  TEST_F(UiAudioTest, SetsTheVolumeOfAGroupFromAValueOfTheUserInterface)
  {
    AddVolume({.value = "music", .group = "music"});
    _values["music"] = "60";

    EXPECT_CALL(_audio, SetGroupVolume("music", FloatEq(0.6f)));

    _system.Update(_store, 0.016);
  }

  TEST_F(UiAudioTest, TakesTheFullValueAsAVolumeOf1)
  {
    AddVolume({.value = "effects", .group = "effects", .full = 10.0f});
    _values["effects"] = "5";

    EXPECT_CALL(_audio, SetGroupVolume("effects", FloatEq(0.5f)));

    _system.Update(_store, 0.016);
  }

  TEST_F(UiAudioTest, HandsTheVolumeOverOnlyWhenItChanges)
  {
    AddVolume({.value = "music", .group = "music"});
    _values["music"] = "60";

    {
      ::testing::InSequence in_order;
      EXPECT_CALL(_audio, SetGroupVolume("music", FloatEq(0.6f))).Times(1);
      EXPECT_CALL(_audio, SetGroupVolume("music", FloatEq(0.25f))).Times(1);
    }

    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);
    _values["music"] = "25";
    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);
  }

  TEST_F(UiAudioTest, LeavesTheGroupAloneWhileTheValueIsNotThere)
  {
    AddVolume({.value = "music", .group = "music"});

    EXPECT_CALL(_audio, SetGroupVolume(_, _)).Times(0);

    _system.Update(_store, 0.016);
  }

  TEST_F(UiAudioTest, LeavesTheGroupAloneForAValueThatIsNoNumber)
  {
    AddVolume({.value = "music", .group = "music"});
    _values["music"] = "loud";

    EXPECT_CALL(_audio, SetGroupVolume(_, _)).Times(0);

    _system.Update(_store, 0.016);
  }

  TEST_F(UiAudioTest, TakesAValueBelowZeroAsSilence)
  {
    AddVolume({.value = "music", .group = "music"});
    _values["music"] = "-20";

    EXPECT_CALL(_audio, SetGroupVolume("music", FloatEq(0.0f)));

    _system.Update(_store, 0.016);
  }

  // UiSoundSwitch

  TEST_F(UiAudioTest, FadesInTheChosenSoundFromTheStart)
  {
    const Entity chilled = AddTrack("chilled", "chilled");
    _values["track"] = "chilled";

    _system.Update(_store, 0.016);

    EXPECT_TRUE(SourceOf(chilled).playing);
    ASSERT_TRUE(SourceOf(chilled).fade.has_value());
    EXPECT_EQ(SourceOf(chilled).fade->kind, SoundFade::Kind::In);
    EXPECT_DOUBLE_EQ(SourceOf(chilled).fade->seconds, 2.0);
  }

  TEST_F(UiAudioTest, KeepsASoundThatIsNotChosenSilentFromTheStart)
  {
    const Entity fight = AddTrack("fight", "fight");
    _values["track"] = "chilled";

    _system.Update(_store, 0.016);

    // never heard at all, not even fading out
    EXPECT_FALSE(SourceOf(fight).playing);
    EXPECT_FALSE(SourceOf(fight).fade.has_value());
  }

  TEST_F(UiAudioTest, KeepsEverySoundSilentWhileTheValueIsNotThere)
  {
    const Entity chilled = AddTrack("chilled", "chilled");

    _system.Update(_store, 0.016);

    EXPECT_FALSE(SourceOf(chilled).playing);
  }

  TEST_F(UiAudioTest, FadesFromOneSoundToAnotherWhenTheChoiceChanges)
  {
    const Entity chilled = AddTrack("chilled", "chilled");
    const Entity fight = AddTrack("fight", "fight");
    _values["track"] = "chilled";
    _system.Update(_store, 0.016);
    SourceOf(chilled).fade.reset();

    _values["track"] = "fight";
    _system.Update(_store, 0.016);

    ASSERT_TRUE(SourceOf(chilled).fade.has_value());
    EXPECT_EQ(SourceOf(chilled).fade->kind, SoundFade::Kind::Out);
    ASSERT_TRUE(SourceOf(fight).fade.has_value());
    EXPECT_EQ(SourceOf(fight).fade->kind, SoundFade::Kind::In);
    EXPECT_TRUE(SourceOf(fight).playing);
  }

  TEST_F(UiAudioTest, LeavesTheSoundsAloneWhileTheChoiceStaysTheSame)
  {
    const Entity chilled = AddTrack("chilled", "chilled");
    const Entity fight = AddTrack("fight", "fight");
    _values["track"] = "chilled";
    _system.Update(_store, 0.016);
    SourceOf(chilled).fade.reset();

    _system.Update(_store, 0.016);

    EXPECT_FALSE(SourceOf(chilled).fade.has_value());
    EXPECT_FALSE(SourceOf(fight).fade.has_value());
  }

  TEST_F(UiAudioTest, FadesOutTheChosenSoundWhenTheValueGoes)
  {
    const Entity chilled = AddTrack("chilled", "chilled");
    _values["track"] = "chilled";
    _system.Update(_store, 0.016);
    SourceOf(chilled).fade.reset();

    _values.erase("track");
    _system.Update(_store, 0.016);

    ASSERT_TRUE(SourceOf(chilled).fade.has_value());
    EXPECT_EQ(SourceOf(chilled).fade->kind, SoundFade::Kind::Out);
  }
} // namespace
