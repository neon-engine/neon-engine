#ifndef MA_AUDIO_SYSTEM_HPP
#define MA_AUDIO_SYSTEM_HPP

#include <memory>

#include <neon/audio/audio-system.hpp>

namespace neon
{
  /// Plays audio with miniaudio.
  ///
  /// With AudioOutput::Device, miniaudio mixes on a thread of its own and
  /// hands the result to the sound card. With AudioOutput::None there is no
  /// sound card and no thread. Advance() mixes as much as the frame lasted, so
  /// that a run with a fixed time step mixes the same every time. When no
  /// sound card can be opened, the system goes on as with AudioOutput::None.
  ///
  /// A file is read once through the file system and kept in memory, however
  /// many sounds are created from it. It is decoded while it plays.
  // ReSharper disable once CppInconsistentNaming
  class MA_AudioSystem final : public AudioSystem
  {
    // holds what is declared by miniaudio, to keep it out of this file
    struct State;
    std::unique_ptr<State> _state;

  public:
    /// What sounds are mixed at, in samples per second.
    static constexpr int sample_rate = 48000;

    MA_AudioSystem(
      const SettingsConfig &settings_config,
      FileSystemContext *file_system,
      const std::shared_ptr<Logger> &logger);

    ~MA_AudioSystem();

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

#endif //MA_AUDIO_SYSTEM_HPP
