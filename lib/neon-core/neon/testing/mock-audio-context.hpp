#ifndef MOCK_AUDIO_CONTEXT_HPP
#define MOCK_AUDIO_CONTEXT_HPP

#include <gmock/gmock.h>

#include <neon/audio/audio-context.hpp>

namespace neon::testing
{
  class MockAudioContext : public AudioContext
  {
  public:
    MOCK_METHOD(int, CreateSound, (const SoundInfo &sound_info), (override));

    MOCK_METHOD(void, DestroySound, (int sound_id), (override));

    MOCK_METHOD(void, Play, (int sound_id), (override));

    MOCK_METHOD(void, Stop, (int sound_id), (override));

    MOCK_METHOD(bool, IsPlaying, (int sound_id), (override));

    MOCK_METHOD(void, FadeIn, (int sound_id, double seconds), (override));

    MOCK_METHOD(void, FadeTo, (int sound_id, float volume, double seconds), (override));

    MOCK_METHOD(void, FadeOut, (int sound_id, double seconds), (override));

    MOCK_METHOD(void, SetVolume, (int sound_id, float volume), (override));

    MOCK_METHOD(void, SetPitch, (int sound_id, float pitch), (override));

    MOCK_METHOD(void, SetLooping, (int sound_id, bool looping), (override));

    MOCK_METHOD(void, SetPosition, (int sound_id, const glm::vec3 &position, const glm::vec3 &velocity), (override));

    MOCK_METHOD(void, SetListener, (const ListenerInfo &listener_info), (override));

    MOCK_METHOD(void, SetMasterVolume, (float volume), (override));

    MOCK_METHOD(void, AddGroup, (const std::string &group), (override));

    MOCK_METHOD(void, SetGroupVolume, (const std::string &group, float volume), (override));

    MOCK_METHOD(float, GetGroupVolume, (const std::string &group), (override));

    MOCK_METHOD(void, Advance, (double delta_time), (override));

    MOCK_METHOD(float, GetOutputLevel, (), (override));
  };
} // neon::testing

#endif //MOCK_AUDIO_CONTEXT_HPP
