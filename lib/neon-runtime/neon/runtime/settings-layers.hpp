#ifndef SETTINGS_LAYERS_HPP
#define SETTINGS_LAYERS_HPP

#include <string>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logging-context.hpp>
#include <neon/runtime/settings-config.hpp>

namespace neon
{
  /// The step of the start-up that reads the files of the settings, each on
  /// top of the layer before, see RuntimeComposition and settings.md: the
  /// settings of the project, then what the player changed. The command
  /// line is the layer above both, and the composition applies it.
  ///
  /// A mistake in the project's file stops the application, since the game
  /// would not be what its author meant. A mistake in the player's file is
  /// said in the log and the file is left out, since a player should not be
  /// locked out of the game by a file a menu wrote.
  class SettingsLayers final
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;
    LoggingContext *_logging;
    std::string _program;

  public:
    /// `program` names the logger the player's mistakes are logged under:
    /// the name of the application.
    SettingsLayers(
      FileSystemContext *file_system,
      DocumentFormat *format,
      LoggingContext *logging,
      const std::string &program);

    /// Reads both files onto `settings`. False with the errors of the
    /// project's file, which stop the start-up.
    bool ReadFiles(SettingsConfig &settings, std::vector<std::string> &errors) const;
  };
} // neon

#endif //SETTINGS_LAYERS_HPP
