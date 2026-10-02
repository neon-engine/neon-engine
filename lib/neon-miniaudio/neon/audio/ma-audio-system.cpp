#include "ma-audio-system.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include <miniaudio.h>

namespace neon
{
  // Helpers of MA_AudioSystem, for this file alone.
  namespace
  {
    constexpr ma_uint32 channels = 2;

    /// The length of a fade in samples, which is what miniaudio counts in.
    /// Fades go by the samples that are mixed, so that without a sound card
    /// they follow the time Advance() is told.
    ma_uint64 Samples(const double seconds)
    {
      return static_cast<ma_uint64>(std::llround(std::max(seconds, 0.0) * MA_AudioSystem::sample_rate));
    }

    using FileContents = std::vector<unsigned char>;

    struct Sound
    {
      // the decoder reads from the file, and the sound from the decoder, so
      // they are released in the opposite order
      std::shared_ptr<FileContents> file;
      ma_decoder decoder{};
      ma_sound sound{};

      // the group it was put in, which is the effects for one that is not
      // there
      std::string group;

      // held where it is while the game is paused
      bool paused = false;
    };

    struct Group
    {
      float volume = 1.0f;
      bool pauses = true;

      // made when the engine is, as it is a part of it
      bool is_made = false;
      ma_sound_group group{};
    };
  }

  struct MA_AudioSystem::State
  {
    ma_engine engine{};
    bool initialized = false;

    // whether Advance() mixes, which it does when there is no sound card
    bool mixes_in_update = false;

    // what is left of a frame after mixing whole samples
    double unmixed_seconds = 0.0;

    std::vector<float> scratch;

    // written where the audio is mixed, which is a thread of miniaudio when
    // there is a sound card
    std::atomic<float> output_level{0.0f};

    // Sounds and groups are handed to miniaudio by address, so they must
    // not move. What a map holds stays where it is until it is erased.
    std::map<int, Sound> sounds;
    int next_id = 0;

    std::map<std::string, std::weak_ptr<FileContents>> files;

    std::map<std::string, Group> groups;
    bool paused = false;

    explicit State(const std::vector<SoundGroupSetting> &settings)
    {
      for (const auto &group : settings)
      {
        groups.try_emplace(group.name, Group{.volume = std::max(group.volume, 0.0f), .pauses = group.pauses});
      }
    }

    Sound *Find(const int sound_id)
    {
      const auto it = sounds.find(sound_id);
      return it == sounds.end() ? nullptr : &it->second;
    }

    /// Makes the sound group of miniaudio for a group, once the engine is
    /// there.
    ma_result Make(Group &group)
    {
      // the group is not a place in the world. Its sounds are placed each
      // for itself, and would be placed twice otherwise
      const auto result = ma_sound_group_init(
        &engine,
        MA_SOUND_FLAG_NO_SPATIALIZATION,
        nullptr,
        &group.group);

      if (result != MA_SUCCESS) { return result; }

      group.is_made = true;
      ma_sound_group_set_volume(&group.group, group.volume);
      return MA_SUCCESS;
    }

    static void OnProcess(void *user_data, float *frames, const ma_uint64 frame_count)
    {
      float peak = 0.0f;
      for (ma_uint64 i = 0; i < frame_count * channels; i++)
      {
        peak = std::max(peak, std::fabs(frames[i]));
      }

      static_cast<State *>(user_data)->output_level.store(std::min(peak, 1.0f));
    }
  };

  MA_AudioSystem::MA_AudioSystem(
    const SettingsConfig &settings_config,
    FileSystemContext *file_system,
    const std::shared_ptr<Logger> &logger)
    : AudioSystem(settings_config, file_system, logger)
  {
    _state = std::make_unique<State>(_settings_config.sound_groups);
  }

  MA_AudioSystem::~MA_AudioSystem()
  {
    CleanUp();
  }

  void MA_AudioSystem::Initialize()
  {
    if (_state->initialized) { return; }

    _logger->Info("Initializing miniaudio");

    auto config = ma_engine_config_init();
    config.channels = channels;
    config.sampleRate = sample_rate;
    config.listenerCount = 1;
    config.onProcess = State::OnProcess;
    config.pProcessUserData = _state.get();

    auto result = MA_ERROR;

    if (_settings_config.audio_output == AudioOutput::Device)
    {
      result = ma_engine_init(&config, &_state->engine);
      if (result != MA_SUCCESS)
      {
        const std::string reason = ma_result_description(result);
        _logger->Warn(
          "No sound card could be opened, nothing will be heard: {}",
          reason);
      }
    }

    _state->mixes_in_update = result != MA_SUCCESS;

    if (_state->mixes_in_update)
    {
      config.noDevice = MA_TRUE;
      result = ma_engine_init(&config, &_state->engine);
      if (result != MA_SUCCESS)
      {
        throw std::runtime_error(
          std::string("Failed to initialize miniaudio: ") + ma_result_description(result));
      }

      _logger->Info("Audio is mixed without a sound card");
    } else if (const auto *device = ma_engine_get_device(&_state->engine); device != nullptr)
    {
      const std::string name = device->playback.name;
      _logger->Info("Playing through {}", name);
    }

    for (auto &[name, group] : _state->groups)
    {
      if (const auto made = _state->Make(group); made != MA_SUCCESS)
      {
        const std::string reason = ma_result_description(made);
        _logger->Error("Group {} could not be made, its sounds go straight to the output: {}", name, reason);
      }
    }

    _state->unmixed_seconds = 0.0;
    _state->output_level.store(0.0f);
    _state->initialized = true;
  }

  void MA_AudioSystem::Advance(const double delta_time)
  {
    if (!_state->initialized || !_state->mixes_in_update || delta_time <= 0.0) { return; }

    // at most a second, so that a frame that took long does not hold up the
    // next one
    _state->unmixed_seconds = std::min(_state->unmixed_seconds + delta_time, 1.0);

    auto frames = static_cast<ma_uint64>(_state->unmixed_seconds * sample_rate);
    _state->unmixed_seconds -= static_cast<double>(frames) / sample_rate;

    constexpr ma_uint64 block = 1024;
    _state->scratch.resize(block * channels);

    while (frames > 0)
    {
      const auto count = std::min(frames, block);
      ma_engine_read_pcm_frames(&_state->engine, _state->scratch.data(), count, nullptr);
      frames -= count;
    }
  }

  void MA_AudioSystem::CleanUp()
  {
    if (!_state->initialized) { return; }

    _logger->Info("Cleaning up miniaudio");

    while (!_state->sounds.empty())
    {
      DestroySound(_state->sounds.begin()->first);
    }

    // after the sounds, which are mixed into them
    for (auto &[name, group] : _state->groups)
    {
      if (!group.is_made) { continue; }
      ma_sound_group_uninit(&group.group);
      group.is_made = false;
    }

    ma_engine_uninit(&_state->engine);

    _state->files.clear();
    _state->initialized = false;
  }

  int MA_AudioSystem::CreateSound(const SoundInfo &sound_info)
  {
    if (!_state->initialized)
    {
      _logger->Error("Sound {} was created before the audio system was initialized", sound_info.path);
      return -1;
    }

    // The sound is made where it stays, since miniaudio keeps its address.
    // It is taken out again when it cannot be made, and the id is not used
    // up then.
    const int id = _state->next_id;
    Sound *sound = &_state->sounds[id];

    if (const auto known = _state->files.find(sound_info.path); known != _state->files.end())
    {
      sound->file = known->second.lock();
    }

    if (sound->file == nullptr)
    {
      sound->file = std::make_shared<FileContents>();
      if (!_file_system->ReadBytes(sound_info.path, *sound->file))
      {
        _logger->Error("Sound {} cannot be read", sound_info.path);
        _state->sounds.erase(id);
        return -1;
      }
      _state->files[sound_info.path] = sound->file;
    }

    const auto decoder_config = ma_decoder_config_init(ma_format_f32, 0, sample_rate);

    auto result = ma_decoder_init_memory(
      sound->file->data(),
      sound->file->size(),
      &decoder_config,
      &sound->decoder);

    if (result != MA_SUCCESS)
    {
      _logger->Error(
        "{} is not a sound that can be played. It has to be WAV, FLAC, MP3, or Ogg Vorbis",
        sound_info.path);
      _state->sounds.erase(id);
      return -1;
    }

    auto group = _state->groups.find(sound_info.group);
    if (group == _state->groups.end())
    {
      _logger->Warn(
        "Sound {} is of group {}, which is not there. It is put among the effects",
        sound_info.path,
        sound_info.group);
      group = _state->groups.find(sound_group::effects);
    }

    const ma_uint32 flags = sound_info.spatial ? 0 : MA_SOUND_FLAG_NO_SPATIALIZATION;

    result = ma_sound_init_from_data_source(
      &_state->engine,
      &sound->decoder,
      flags,
      group->second.is_made ? &group->second.group : nullptr,
      &sound->sound);
    if (result != MA_SUCCESS)
    {
      ma_decoder_uninit(&sound->decoder);
      _state->sounds.erase(id);
      const std::string reason = ma_result_description(result);
      _logger->Error("Sound {} could not be created: {}", sound_info.path, reason);
      return -1;
    }

    sound->group = group->first;
    ma_sound_set_looping(&sound->sound, sound_info.looping ? MA_TRUE : MA_FALSE);
    ma_sound_set_volume(&sound->sound, sound_info.volume);
    ma_sound_set_pitch(&sound->sound, sound_info.pitch);

    if (sound_info.spatial)
    {
      ma_sound_set_min_distance(&sound->sound, sound_info.min_distance);
      ma_sound_set_max_distance(&sound->sound, sound_info.max_distance);
    }

    _state->next_id++;

    _logger->Debug("Created sound {} from {}", id, sound_info.path);
    return id;
  }

  void MA_AudioSystem::DestroySound(const int sound_id)
  {
    const auto it = _state->sounds.find(sound_id);
    if (it == _state->sounds.end()) { return; }

    ma_sound_uninit(&it->second.sound);
    ma_decoder_uninit(&it->second.decoder);
    _state->sounds.erase(it);
  }

  void MA_AudioSystem::Play(const int sound_id)
  {
    auto *sound = _state->Find(sound_id);
    if (sound == nullptr) { return; }

    // a fade out that is under way would stop the sound again, and a sound
    // that faded to silence would not be heard
    ma_sound_reset_stop_time(&sound->sound);
    ma_sound_set_fade_in_pcm_frames(&sound->sound, 1.0f, 1.0f, 0);

    ma_sound_seek_to_pcm_frame(&sound->sound, 0);
    sound->paused = false;

    if (const auto result = ma_sound_start(&sound->sound); result != MA_SUCCESS)
    {
      const std::string reason = ma_result_description(result);
      _logger->Error("Sound {} could not be played: {}", sound_id, reason);
    }
  }

  void MA_AudioSystem::Stop(const int sound_id)
  {
    if (auto *sound = _state->Find(sound_id); sound != nullptr)
    {
      sound->paused = false;
      ma_sound_stop(&sound->sound);
    }
  }

  bool MA_AudioSystem::IsPlaying(const int sound_id)
  {
    // a sound that is held is playing, and goes on when the game does
    auto *sound = _state->Find(sound_id);
    return sound != nullptr && (sound->paused || ma_sound_is_playing(&sound->sound) == MA_TRUE);
  }

  void MA_AudioSystem::FadeIn(const int sound_id, const double seconds)
  {
    auto *sound = _state->Find(sound_id);
    if (sound == nullptr) { return; }

    Play(sound_id);

    // miniaudio takes up the fade where the sound is mixed, which is after
    // it was started
    ma_sound_set_fade_in_pcm_frames(&sound->sound, 0.0f, 1.0f, Samples(seconds));
  }

  void MA_AudioSystem::FadeTo(const int sound_id, const float volume, const double seconds)
  {
    auto *sound = _state->Find(sound_id);
    if (sound == nullptr) { return; }

    // a sound that has stopped is past its time to stop, and putting that
    // time off would play it again
    if (ma_sound_is_playing(&sound->sound) == MA_TRUE) { ma_sound_reset_stop_time(&sound->sound); }

    // a volume below 0 tells miniaudio to begin where the fade is now
    ma_sound_set_fade_in_pcm_frames(&sound->sound, -1.0f, std::max(volume, 0.0f), Samples(seconds));
  }

  void MA_AudioSystem::FadeOut(const int sound_id, const double seconds)
  {
    auto *sound = _state->Find(sound_id);
    if (sound == nullptr || ma_sound_is_playing(&sound->sound) == MA_FALSE) { return; }

    ma_sound_stop_with_fade_in_pcm_frames(&sound->sound, Samples(seconds));
  }

  void MA_AudioSystem::SetVolume(const int sound_id, const float volume)
  {
    if (auto *sound = _state->Find(sound_id); sound != nullptr)
    {
      ma_sound_set_volume(&sound->sound, std::max(volume, 0.0f));
    }
  }

  void MA_AudioSystem::SetPitch(const int sound_id, const float pitch)
  {
    // miniaudio does not take a pitch of 0 or below
    if (auto *sound = _state->Find(sound_id); sound != nullptr && pitch > 0.0f)
    {
      ma_sound_set_pitch(&sound->sound, pitch);
    }
  }

  void MA_AudioSystem::SetLooping(const int sound_id, const bool looping)
  {
    if (auto *sound = _state->Find(sound_id); sound != nullptr)
    {
      ma_sound_set_looping(&sound->sound, looping ? MA_TRUE : MA_FALSE);
    }
  }

  void MA_AudioSystem::SetPosition(const int sound_id, const glm::vec3 &position, const glm::vec3 &velocity)
  {
    if (auto *sound = _state->Find(sound_id); sound != nullptr)
    {
      ma_sound_set_position(&sound->sound, position.x, position.y, position.z);
      ma_sound_set_velocity(&sound->sound, velocity.x, velocity.y, velocity.z);
    }
  }

  void MA_AudioSystem::SetListener(const ListenerInfo &listener_info)
  {
    if (!_state->initialized) { return; }

    const auto &[position, forward, up, velocity] = listener_info;
    ma_engine_listener_set_position(&_state->engine, 0, position.x, position.y, position.z);
    ma_engine_listener_set_direction(&_state->engine, 0, forward.x, forward.y, forward.z);
    ma_engine_listener_set_world_up(&_state->engine, 0, up.x, up.y, up.z);
    ma_engine_listener_set_velocity(&_state->engine, 0, velocity.x, velocity.y, velocity.z);
  }

  void MA_AudioSystem::SetMasterVolume(const float volume)
  {
    if (!_state->initialized) { return; }
    ma_engine_set_volume(&_state->engine, std::max(volume, 0.0f));
  }

  void MA_AudioSystem::AddGroup(const std::string &group)
  {
    const auto [it, added] = _state->groups.try_emplace(group);
    if (!added || !_state->initialized) { return; }

    if (const auto made = _state->Make(it->second); made != MA_SUCCESS)
    {
      const std::string reason = ma_result_description(made);
      _logger->Error("Group {} could not be made, its sounds go straight to the output: {}", group, reason);
    }
  }

  void MA_AudioSystem::StopGroup(const std::string &group)
  {
    if (!_state->groups.contains(group))
    {
      _logger->Warn("The sounds of group {} cannot be stopped, as there is no such group", group);
      return;
    }

    for (auto &[id, sound] : _state->sounds)
    {
      if (sound.group == group) { Stop(id); }
    }
  }

  void MA_AudioSystem::SetPaused(const bool paused)
  {
    if (paused == _state->paused) { return; }
    _state->paused = paused;

    for (auto &[id, sound] : _state->sounds)
    {
      if (paused)
      {
        // stopped without seeking, so that it goes on from where it is
        if (!_state->groups.at(sound.group).pauses || ma_sound_is_playing(&sound.sound) == MA_FALSE) { continue; }
        ma_sound_stop(&sound.sound);
        sound.paused = true;
      } else if (sound.paused)
      {
        sound.paused = false;
        ma_sound_start(&sound.sound);
      }
    }
  }

  void MA_AudioSystem::SetGroupVolume(const std::string &group, const float volume)
  {
    const auto it = _state->groups.find(group);
    if (it == _state->groups.end())
    {
      _logger->Warn("The volume of group {} cannot be set, as there is no such group", group);
      return;
    }

    it->second.volume = std::max(volume, 0.0f);
    if (it->second.is_made) { ma_sound_group_set_volume(&it->second.group, it->second.volume); }
  }

  float MA_AudioSystem::GetGroupVolume(const std::string &group)
  {
    const auto it = _state->groups.find(group);
    return it == _state->groups.end() ? 0.0f : it->second.volume;
  }

  float MA_AudioSystem::GetOutputLevel()
  {
    return _state->output_level.load();
  }
} // neon
