#ifndef PROJECT_FILE_HPP
#define PROJECT_FILE_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>

#include "project.hpp"

namespace neon
{
  /// The file at the root of a project that says what the project is.
  ///
  ///     version: 1
  ///     name: neon-runtime
  ///     organization: neon-engine
  ///
  ///     scenes:
  ///       - assets://scenes/demo.scene.yml
  ///       - assets://scenes/physics.scene.yml
  ///
  ///     entry_scene: assets://scenes/demo.scene.yml
  ///
  ///     graphics_presets:
  ///       low:
  ///         texture_scale: 1
  ///       potato:
  ///         anisotropy: 1
  ///         shadow_cascades: 1
  ///       ultra: off
  ///
  ///     settings:
  ///       difficulty:
  ///         kind: choice
  ///         default: normal
  ///         choices: [easy, normal, hard]
  ///         category: Gameplay
  ///
  /// `version` is the version of this layout. `graphics_presets` changes
  /// the quality presets the game offers, see GraphicsPresets: a built-in
  /// name is changed value by value, a new name is added with the defaults
  /// of the engine for what it leaves out, and `off` drops one. `settings`
  /// declares the settings of the game, see SettingDeclaration; what they
  /// are set to is in the settings files, see SettingsFile. A name that is not known is an
  /// error, so that a name that was misspelled does not go unnoticed. What
  /// the file holds is read before anything else of the project is, since
  /// `user://` has no place until the names are known.
  ///
  /// Which format the file has is up to the DocumentFormat that is handed in.
  class ProjectFile final
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;
    std::shared_ptr<Logger> _logger;

  public:
    /// The version of the layout of the file that is read, and the highest
    /// that is understood.
    static constexpr int version = 1;

    /// Where the file is. A project is the folder behind `assets://`, so
    /// the file is always at the same place.
    static constexpr std::string_view path = "assets://project.yml";

    ProjectFile(FileSystemContext *file_system, DocumentFormat *format, const std::shared_ptr<Logger> &logger);

    /// Reads the file into `project`. Returns false when the file cannot be
    /// read or something in it is wrong, and every problem that was found is
    /// in `errors` with its line, not only the first. Nothing is logged: the
    /// caller decides where the problems go, since the log file is not open
    /// yet when the project is read.
    bool Read(Project &project, std::vector<std::string> &errors) const;

    /// Whether a name may be used as a folder name on every platform, and
    /// reads well in a path: lowercase letters, digits, and dashes, starting
    /// with a letter.
    [[nodiscard]] static bool IsPlainName(std::string_view name);
  };
} // neon

#endif //PROJECT_FILE_HPP
