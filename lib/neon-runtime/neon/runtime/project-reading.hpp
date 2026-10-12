#ifndef PROJECT_READING_HPP
#define PROJECT_READING_HPP

#include <string>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logging-context.hpp>
#include <neon/project/project.hpp>

namespace neon
{
  /// The step of the start-up that reads the project behind `assets://`,
  /// see RuntimeComposition. A built game carries `game.yml`, which the
  /// export wrote from `project.yml`, and that is read first: a project
  /// under development has none and runs from `project.yml`, the editor's
  /// file. Nothing is written, so it runs before `user://` is placed.
  class ProjectReading final
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;
    LoggingContext *_logging;
    std::string _program;

  public:
    /// `program` names the logger a remark is logged under: the name of the
    /// application.
    ProjectReading(
      FileSystemContext *file_system,
      DocumentFormat *format,
      LoggingContext *logging,
      const std::string &program);

    /// Reads the game, or the project without one, into `project`. `file`
    /// is the path of the file that was read, or looked for, which the
    /// messages about the settings of the game name. False with the errors
    /// when it cannot be read; `none` says that neither file is here, so
    /// that the caller can say what to start instead.
    bool Read(Project &project, std::string &file, bool &none, std::vector<std::string> &errors) const;
  };
} // neon

#endif //PROJECT_READING_HPP
