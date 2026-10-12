#ifndef PROJECT_TEMPLATE_HPP
#define PROJECT_TEMPLATE_HPP

#include <memory>
#include <string>
#include <string_view>

#include <neon/filesystem/native-file-context.hpp>
#include <neon/logging/logger.hpp>

#include "new-project.hpp"

namespace neon::editor
{
  /// Makes a new project outside the engine: a folder under the parent
  /// folder, `~/Dev` unless another is given, with everything a project
  /// needs to run and to build, so that the first build works and the
  /// first run shows something. See docs/new-projects.md.
  ///
  ///     <parent>/<name>/
  ///       CMakeLists.txt             on cmake/NeonSdk.cmake of the engine
  ///       CMakePresets.json          the presets every project shares
  ///       README.md  .gitignore
  ///       assets/
  ///         project.yml  settings.yml
  ///         scenes/start.scene.yml   a camera, a light, a floor, a box
  ///         input/game.input.yml
  ///       extensions/<name>/
  ///         extension.yml
  ///         <name>.cpp               an extension that turns the box
  ///
  /// The folder is new: an existing one is never written into. Every
  /// problem is one message, for the dialog of the editor and the log.
  class ProjectTemplate final
  {
    NativeFileContext *_files;
    std::shared_ptr<Logger> _logger;

    /// Writes one file of the project, in a folder that is there already.
    bool Write(const std::string &native_path, const std::string &contents, std::string &error) const;

  public:
    /// The folder under the home folder that projects go in by default.
    static constexpr std::string_view default_parent_name = "Dev";

    ProjectTemplate(NativeFileContext *files, const std::shared_ptr<Logger> &logger);

    /// `Dev` under the home folder of the user. Returns false when the home
    /// folder is not known.
    bool DefaultParentDirectory(std::string &native_directory) const;

    /// Whether a name is plain: lowercase letters, digits, and dashes,
    /// starting with a letter. It is the rule of ProjectFile.
    [[nodiscard]] static bool IsPlainName(std::string_view name);

    /// Makes the project. `project_directory` receives the folder that was
    /// made. Returns false, with why in `error`, when the names are not
    /// plain, the engine is not where it is said to be, the folder is there
    /// already, or a file cannot be written; what was written stays.
    bool Create(const NewProject &project, std::string &project_directory, std::string &error) const;
  };
} // neon::editor

#endif //PROJECT_TEMPLATE_HPP
