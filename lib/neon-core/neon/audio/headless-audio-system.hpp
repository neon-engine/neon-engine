#ifndef HEADLESS_AUDIO_SYSTEM_HPP
#define HEADLESS_AUDIO_SYSTEM_HPP

#include <map>

#include "audio-system.hpp"

namespace neon
{
  /// An audio system that plays nothing. It needs no sound card and reads no
  /// files, so the engine can run on a build server or from a script.
  ///
  /// It keeps track of what it is told, so that a game behaves as it does
  /// with sound. A sound that does not loop has no length here, and counts
  /// as ended at the next frame.
  // ReSharper disable once CppInconsistentNaming
  class Headless_AudioSystem final : public AudioSystem
  {
    struct Sound
    {
      bool playing = false;
      bool looping = false;
    };

    std::map<int, Sound> _sounds;
    int _next_id = 0;

  public:
    Headless_AudioSystem(
      const SettingsConfig &settings_config,
      FileSystemContext *file_system,
      const std::shared_ptr<Logger> &logger)
      : AudioSystem(settings_config, file_system, logger) {}

    void Initialize() override;

    void Advance(double delta_time) override;

    void CleanUp() override;

    int CreateSound(const SoundInfo &sound_info) override;

    void DestroySound(int sound_id) override;

    void Play(int sound_id) override;

    void Stop(int sound_id) override;

    bool IsPlaying(int sound_id) override;

    void SetVolume(int sound_id, float volume) override;

    void SetPitch(int sound_id, float pitch) override;

    void SetLooping(int sound_id, bool looping) override;

    void SetPosition(int sound_id, const glm::vec3 &position, const glm::vec3 &velocity) override;

    void SetListener(const ListenerInfo &listener_info) override;

    void SetMasterVolume(float volume) override;

    float GetOutputLevel() override;
  };
} // neon

#endif //HEADLESS_AUDIO_SYSTEM_HPP
