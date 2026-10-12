#ifndef GAME_FILE_HPP
#define GAME_FILE_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>

#include "project-file.hpp"
#include "project.hpp"

namespace neon
{
  /// The file of a built game: what the export writes from `project.yml`
  /// and puts next to the runtime, with the subset a runtime needs to run
  /// that game and no other. The name and the organization, which place
  /// `user://`, the scenes and the entry scene, the input map, the settings
  /// the game declares, and the quality presets it offers; nothing of the
  /// editor's.
  ///
  ///     version: 1
  ///     name: neon-sandbox
  ///     organization: neon-engine
  ///
  ///     scenes:
  ///       - assets://scenes/start.scene.yml
  ///
  ///     graphics_presets:
  ///       potato:
  ///         anisotropy: 1
  ///
  ///     settings:
  ///       show_hints:
  ///         kind: flag
  ///         default: true
  ///
  /// It is read by the reader of ProjectFile, so it takes the same names
  /// and says the same about a wrong one, with its line. The runtime reads
  /// it before `project.yml`, so that a built game runs from it and a
  /// project under development, which has none, runs from `project.yml`.
  /// The runtime never writes it, and a player does not edit it: what a
  /// player changes is the settings file of the player, `user://settings.yml`,
  /// see SettingsFile.
  ///
  /// Which format the file has is up to the DocumentFormat that is handed in.
  class GameFile final
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;
    std::shared_ptr<Logger> _logger;

  public:
    /// The version of the layout, which is that of `project.yml`, since
    /// the two are read by one reader.
    static constexpr int version = ProjectFile::version;

    /// Where the file is: where `project.yml` is read from, so that the
    /// two move together.
    static constexpr std::string_view path = "assets://game.yml";

    GameFile(FileSystemContext *file_system, DocumentFormat *format, const std::shared_ptr<Logger> &logger);

    /// Reads the file into `game`. Returns false when the file cannot be
    /// read or something in it is wrong, and every problem that was found
    /// is in `errors` with its line, not only the first, as ProjectFile
    /// reports them. Nothing is logged but that the file is read: the
    /// caller decides where the problems go, since the log file is not
    /// open yet when the game is read.
    bool Read(Project &game, std::vector<std::string> &errors) const;

    /// The text of a `game.yml` that says what `project` says, for the
    /// export to put next to the runtime: a document in the style of the
    /// recipes, block style, one key a line, with what is at its default
    /// left out. The engine's quality presets are written as what the
    /// project did to them, so that reading it back gives the project as
    /// it was read. Nothing is written anywhere: where a built game goes is
    /// the export's to decide.
    [[nodiscard]] std::string WriteFrom(const Project &project) const;
  };
} // neon

#endif //GAME_FILE_HPP
