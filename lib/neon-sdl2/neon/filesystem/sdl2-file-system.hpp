#ifndef SDL_2_FILE_SYSTEM_HPP
#define SDL_2_FILE_SYSTEM_HPP

#include <memory>
#include <string>
#include <vector>
#include <neon/runtime/settings-config.hpp>
#include <neon/filesystem/file-system.hpp>
#include <neon/logging/logger.hpp>

namespace neon
{
  // ReSharper disable once CppInconsistentNaming
  class SDL2_FileSystem final : public FileSystem
  {
    /// Turns the output folder of the settings into the folder behind
    /// `output://`, and creates it when it is missing.
    void InitializeOutputDirectory();

  public:
    explicit SDL2_FileSystem(const SettingsConfig &settings_config, const std::shared_ptr<Logger> &logger)
      : FileSystem(settings_config, logger) {}

    void Initialize() override;

    void CleanUp() override;

    bool Exists(const std::string &path) override;

    bool ReadBytes(const std::string &path, std::vector<unsigned char> &contents) override;

    bool WriteBytes(const std::string &path, const std::vector<unsigned char> &contents) override;

  protected:
    bool ListDirectory(const std::string &native_directory, std::vector<std::string> &names) override;

    bool MakeDirectory(const std::string &native_directory) override;
  };
} // neon

#endif //SDL_2_FILE_SYSTEM_HPP
