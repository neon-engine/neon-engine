#ifndef AUDIO_SYSTEM_HPP
#define AUDIO_SYSTEM_HPP

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>
#include <neon/runtime/settings-config.hpp>

#include "audio-context.hpp"
#include "sound-memory.hpp"

namespace neon
{
  /// Base class of what plays audio. Sounds are read through the file
  /// system, so a backend opens no files itself.
  class AudioSystem : public AudioContext
  {
  protected:
    SettingsConfig _settings_config;
    FileSystemContext *_file_system;
    std::shared_ptr<Logger> _logger;

    /// The sounds that were handed over in memory, by their names. A sound
    /// that was created from one keeps its bytes with it, so setting a name
    /// again takes nothing from a sound that plays.
    std::map<std::string, std::shared_ptr<const std::vector<std::uint8_t>>> _memory_sounds;

    /// The bytes a sound is created from: those that were handed over in
    /// memory for a path that names such a sound, and else those of the
    /// file. Nothing, after saying why, when there are none.
    [[nodiscard]] std::shared_ptr<const std::vector<std::uint8_t>> ReadSound(const std::string &path) const
    {
      if (SoundMemory::IsNamedBy(path))
      {
        const auto known = _memory_sounds.find(SoundMemory::NameOf(path));
        if (known == _memory_sounds.end())
        {
          _logger->Error("Sound {} was not handed to the audio, so there is nothing to play", path);
          return nullptr;
        }
        return known->second;
      }

      auto bytes = std::make_shared<std::vector<std::uint8_t>>();
      if (!_file_system->ReadBytes(path, *bytes))
      {
        _logger->Error("Sound {} cannot be read", path);
        return nullptr;
      }
      return bytes;
    }

    ~AudioSystem() = default;

  public:
    AudioSystem(
      const SettingsConfig &settings_config,
      FileSystemContext *file_system,
      const std::shared_ptr<Logger> &logger)
    {
      _settings_config = settings_config;
      _file_system = file_system;
      _logger = logger;
    }

    bool SetSound(const std::string &name, std::vector<std::uint8_t> bytes) override
    {
      if (name.empty() || bytes.empty()) { return false; }

      _memory_sounds[name] = std::make_shared<const std::vector<std::uint8_t>>(std::move(bytes));
      return true;
    }

    virtual void Initialize() = 0;

    /// Destroys every sound that is left.
    virtual void CleanUp() = 0;
  };
} // neon

#endif //AUDIO_SYSTEM_HPP
