#include "project-reading.hpp"

#include <neon/project/game-file.hpp>
#include <neon/project/project-file.hpp>

namespace neon
{
  ProjectReading::ProjectReading(
    FileSystemContext *file_system,
    DocumentFormat *format,
    LoggingContext *logging,
    const std::string &program)
    : _file_system(file_system), _format(format), _logging(logging), _program(program) {}

  bool ProjectReading::Read(
    Project &project,
    std::string &file,
    bool &none,
    std::vector<std::string> &errors) const
  {
    const bool is_built = _file_system->Exists(std::string(GameFile::path));
    const bool is_project = _file_system->Exists(std::string(ProjectFile::path));
    none = !is_built && !is_project;

    if (is_built && is_project)
    {
      _logging->CreateLogger(_program)->Info(
        "Both {} and {} are here, and the game is what runs: the project is the editor's file",
        GameFile::path,
        ProjectFile::path);
    }

    file = std::string(is_built ? GameFile::path : ProjectFile::path);

    if (is_built)
    {
      const GameFile game_file(_file_system, _format, _logging->CreateLogger("GameFile"));
      return game_file.Read(project, errors);
    }

    const ProjectFile project_file(_file_system, _format, _logging->CreateLogger("ProjectFile"));
    return project_file.Read(project, errors);
  }
} // neon
