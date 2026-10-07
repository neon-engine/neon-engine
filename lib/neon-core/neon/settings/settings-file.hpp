#ifndef SETTINGS_FILE_HPP
#define SETTINGS_FILE_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>
#include <neon/runtime/settings-config.hpp>

namespace neon
{
  /// A file of settings: what a game chooses for itself, and what a player
  /// may change, as opposed to what the project is (ProjectFile).
  ///
  ///     version: 1
  ///
  ///     window:
  ///       width: 1920
  ///       height: 1080
  ///       mode: borderless
  ///
  ///     ui:
  ///       scale: 1
  ///       pause_menu: engine://ui/pause.ui.yml
  ///       settings_menu: engine://ui/settings.ui.yml
  ///
  ///     world:
  ///       steps_per_second: 60
  ///
  ///     input:
  ///       gyro: false
  ///
  ///     rendering:
  ///       vulkan_version: "1.3"
  ///       tonemapper: aces
  ///       exposure: 1
  ///       vsync: true
  ///       max_fps: 0
  ///       anisotropy: 8
  ///
  ///     audio:
  ///       groups:
  ///         - radio
  ///         - {name: crowd, volume: 0.5}
  ///       volumes:
  ///         music: 0.6
  ///         ambience: 0.5
  ///
  /// What a file leaves out keeps the value it had, so that the files are
  /// read in layers, each on top of the one before: the defaults in code,
  /// the file of the project at `assets://settings.yml`, the file of the
  /// player at `user://settings.yml`, and then the command line. A name
  /// that is not known is an error, so that a name that was misspelled
  /// does not go unnoticed.
  ///
  /// Which format the file has is up to the DocumentFormat that is handed in.
  class SettingsFile final
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;
    std::shared_ptr<Logger> _logger;

  public:
    /// The version of the layout of the file that is read, and the highest
    /// that is understood.
    static constexpr int version = 1;

    /// The settings the project ships with, and the settings the player
    /// changed, which a settings menu writes.
    static constexpr std::string_view of_the_project = "assets://settings.yml";
    static constexpr std::string_view of_the_player = "user://settings.yml";

    SettingsFile(FileSystemContext *file_system, DocumentFormat *format, const std::shared_ptr<Logger> &logger);

    /// Reads the file at `path` on top of `settings`. A file that is not
    /// there is not an error: `found` says whether there was one, and
    /// nothing changes. Returns false when the file is there and something
    /// in it is wrong; every problem is in `errors` with its line, and
    /// `settings` is left as it was, so that a file with one mistake does
    /// not apply by halves.
    bool Read(const std::string &path, SettingsConfig &settings, bool &found, std::vector<std::string> &errors) const;
  };
} // neon

#endif //SETTINGS_FILE_HPP
