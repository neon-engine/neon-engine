#ifndef PROJECT_TEMPLATE_TEXT_HPP
#define PROJECT_TEMPLATE_TEXT_HPP

#include <string>

#include "new-project.hpp"

namespace neon::editor
{
  /// The text of each file a new project is made of, see ProjectTemplate.
  /// Every file is written with the name of the project in it, and nothing
  /// else of the person's choices but the engine's folder, so that two
  /// projects are the same but for their names: the presets, the layout,
  /// and the commands that build them are shared by every project.
  class ProjectTemplateText final
  {
  public:
    ProjectTemplateText() = delete;

    /// `assets/project.yml`: the name, the organization, the one scene, and
    /// the input map, see docs/projects.md.
    [[nodiscard]] static std::string ProjectFile(const NewProject &project);

    /// `assets/settings.yml`: the pause and settings menus of the engine.
    [[nodiscard]] static std::string Settings(const NewProject &project);

    /// `assets/scenes/start.scene.yml`: a camera, a light, a floor, and a box
    /// the extension turns, all built by the engine from Geometry.
    [[nodiscard]] static std::string StartScene(const NewProject &project);

    /// `assets/input/game.input.yml`: the actions of the engine's own map,
    /// as the project's own to change.
    [[nodiscard]] static std::string InputMap(const NewProject &project);

    /// `extensions/<name>/extension.yml`: the recipe, with the library of
    /// every platform.
    [[nodiscard]] static std::string ExtensionRecipe(const NewProject &project);

    /// `extensions/<name>/<name>.cpp`: an extension in C++ with one
    /// component, Spinner, and the system that turns what carries it.
    [[nodiscard]] static std::string ExtensionSource(const NewProject &project);

    /// `CMakeLists.txt`: NEON_ENGINE_DIRECTORY, NeonSdk.cmake, and
    /// neon_add_project.
    [[nodiscard]] static std::string CMakeLists(const NewProject &project);

    /// `CMakePresets.json`: debug and release for macOS arm64, Linux x64,
    /// and Windows x64, the same in every project.
    [[nodiscard]] static std::string CMakePresets();

    /// `.gitignore`: the build folder and what a machine leaves behind.
    [[nodiscard]] static std::string GitIgnore();

    /// `README.md`: how to build and run, and how the engine is found.
    [[nodiscard]] static std::string ReadMe(const NewProject &project);

    /// A native path as CMake reads it: forward slashes on every platform.
    [[nodiscard]] static std::string CMakePath(const std::string &native_path);
  };
} // neon::editor

#endif //PROJECT_TEMPLATE_TEXT_HPP
