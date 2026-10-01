#include "headless-audio-system.hpp"

#include <algorithm>

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

      if (sound.stops_in >= 0.0)
      {
        sound.stops_in -= delta_time;
        if (sound.stops_in <= 0.0)
        {
          sound.playing = false;
          sound.stops_in = -1.0;
        }
      }
    }
  }

  void Headless_AudioSystem::CleanUp()
  {
    _logger->Info("Cleaning up headless audio system");
    _sounds.clear();
  }

  int Headless_AudioSystem::CreateSound(const SoundInfo &sound_info)
  {
    // said here as well, so that a run without sound finds a group that is
    // misspelt
    if (!_group_volumes.contains(sound_info.group))
    {
      _logger->Warn(
        "Sound {} is of group {}, which is not there. It is put among the effects",
        sound_info.path,
        sound_info.group);
    }

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
    if (const auto it = _sounds.find(sound_id); it != _sounds.end())
    {
      it->second.playing = true;
      it->second.stops_in = -1.0;
    }
  }

  void Headless_AudioSystem::Stop(const int sound_id)
  {
    if (const auto it = _sounds.find(sound_id); it != _sounds.end())
    {
      it->second.playing = false;
      it->second.stops_in = -1.0;
    }
  }

  bool Headless_AudioSystem::IsPlaying(const int sound_id)
  {
    const auto it = _sounds.find(sound_id);
    return it != _sounds.end() && it->second.playing;
  }

  void Headless_AudioSystem::FadeIn(const int sound_id, const double seconds)
  {
    Play(sound_id);
  }

  void Headless_AudioSystem::FadeTo(const int sound_id, const float volume, const double seconds)
  {
    // what is heard is not kept, but a fade puts off a fade out, which
    // decides when the sound stops
    if (const auto it = _sounds.find(sound_id); it != _sounds.end()) { it->second.stops_in = -1.0; }
  }

  void Headless_AudioSystem::FadeOut(const int sound_id, const double seconds)
  {
    const auto it = _sounds.find(sound_id);
    if (it == _sounds.end() || !it->second.playing) { return; }

    if (seconds <= 0.0) { Stop(sound_id); } else { it->second.stops_in = seconds; }
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

  void Headless_AudioSystem::AddGroup(const std::string &group)
  {
    _group_volumes.try_emplace(group, 1.0f);
  }

  void Headless_AudioSystem::SetGroupVolume(const std::string &group, const float volume)
  {
    const auto it = _group_volumes.find(group);
    if (it == _group_volumes.end())
    {
      _logger->Warn("The volume of group {} cannot be set, as there is no such group", group);
      return;
    }

    it->second = std::max(volume, 0.0f);
  }

  float Headless_AudioSystem::GetGroupVolume(const std::string &group)
  {
    const auto it = _group_volumes.find(group);
    return it == _group_volumes.end() ? 0.0f : it->second;
  }

  float Headless_AudioSystem::GetOutputLevel()
  {
    return 0.0f;
  }
} // neon
