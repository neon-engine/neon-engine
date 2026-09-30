#include "headless-audio-system.hpp"

namespace neon
{
  void Headless_AudioSystem::Initialize()
  {
    _logger->Info("Initializing headless audio system");
  }

  void Headless_AudioSystem::Advance(const double delta_time)
  {
    for (auto &[id, sound] : _sounds)
    {
      if (!sound.looping) { sound.playing = false; }
    }
  }

  void Headless_AudioSystem::CleanUp()
  {
    _logger->Info("Cleaning up headless audio system");
    _sounds.clear();
  }

  int Headless_AudioSystem::CreateSound(const SoundInfo &sound_info)
  {
    const int id = _next_id++;
    _sounds[id] = {.playing = false, .looping = sound_info.looping};
    return id;
  }

  void Headless_AudioSystem::DestroySound(const int sound_id)
  {
    _sounds.erase(sound_id);
  }

  void Headless_AudioSystem::Play(const int sound_id)
  {
    if (const auto it = _sounds.find(sound_id); it != _sounds.end()) { it->second.playing = true; }
  }

  void Headless_AudioSystem::Stop(const int sound_id)
  {
    if (const auto it = _sounds.find(sound_id); it != _sounds.end()) { it->second.playing = false; }
  }

  bool Headless_AudioSystem::IsPlaying(const int sound_id)
  {
    const auto it = _sounds.find(sound_id);
    return it != _sounds.end() && it->second.playing;
  }

  void Headless_AudioSystem::SetVolume(const int sound_id, const float volume) {}

  void Headless_AudioSystem::SetPitch(const int sound_id, const float pitch) {}

  void Headless_AudioSystem::SetLooping(const int sound_id, const bool looping)
  {
    if (const auto it = _sounds.find(sound_id); it != _sounds.end()) { it->second.looping = looping; }
  }

  void Headless_AudioSystem::SetPosition(const int sound_id, const glm::vec3 &position, const glm::vec3 &velocity) {}

  void Headless_AudioSystem::SetListener(const ListenerInfo &listener_info) {}

  void Headless_AudioSystem::SetMasterVolume(const float volume) {}

  float Headless_AudioSystem::GetOutputLevel()
  {
    return 0.0f;
  }
} // neon
