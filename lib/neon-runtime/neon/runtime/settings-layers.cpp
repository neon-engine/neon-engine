#include "settings-layers.hpp"

#include <neon/settings/settings-file.hpp>

namespace neon
{
  SettingsLayers::SettingsLayers(
    FileSystemContext *file_system,
    DocumentFormat *format,
    LoggingContext *logging,
    const std::string &program)
    : _file_system(file_system), _format(format), _logging(logging), _program(program) {}

  bool SettingsLayers::ReadFiles(SettingsConfig &settings, std::vector<std::string> &errors) const
  {
    const SettingsFile settings_file(_file_system, _format, _logging->CreateLogger("SettingsFile"));
    const auto logger = _logging->CreateLogger(_program);

    bool found = false;
    if (!settings_file.Read(std::string(SettingsFile::of_the_project), settings, found, errors)) { return false; }

    if (!settings_file.Read(std::string(SettingsFile::of_the_player), settings, found, errors))
    {
      for (const auto &error : errors) { logger->Warn("{}", error); }
      logger->Warn("The settings of the player are left out until they are corrected");
      errors.clear();
    }

    return true;
  }
} // neon
