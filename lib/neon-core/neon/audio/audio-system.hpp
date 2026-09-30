#ifndef AUDIO_SYSTEM_HPP
#define AUDIO_SYSTEM_HPP

#include <memory>

#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>
#include <neon/runtime/settings-config.hpp>

#include "audio-context.hpp"

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

    virtual void Initialize() = 0;

    /// Destroys every sound that is left.
    virtual void CleanUp() = 0;
  };
} // neon

#endif //AUDIO_SYSTEM_HPP
