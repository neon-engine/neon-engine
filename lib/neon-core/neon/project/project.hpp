#ifndef PROJECT_HPP
#define PROJECT_HPP

#include <string>
#include <string_view>
#include <vector>

#include <neon/render/graphics-presets.hpp>

namespace neon
{
  /// What a project says about itself, as its `project.yml` holds it. A
  /// project is a folder with that file at its root, and the runtime runs one
  /// project: the folder that `assets://` stands for.
  struct Project
  {
    /// What the project is called, for the title of its window and for the
    /// folder it writes to. A plain name: lowercase letters, digits, and
    /// dashes, starting with a letter, since it becomes a folder name on
    /// every platform.
    std::string name;

    /// Who makes it, in the same plain form. Together with the name it
    /// places `user://`, so that projects of one maker share a folder and
    /// projects of different makers do not. A project that names none is
    /// one of the engine's own.
    static constexpr std::string_view default_organization = "neon-engine";
    std::string organization = std::string(default_organization);

    /// The scenes of the project, as virtual paths. The first of them is
    /// where the project starts unless `entry_scene` says otherwise.
    std::vector<std::string> scenes;

    /// The scene the project starts with. One of `scenes`.
    std::string entry_scene;

    /// The input map the project plays with, as a virtual path, such as
    /// `assets://input/game.input.yml`. Empty for the map the engine brings,
    /// `engine://input/default.input.yml`.
    std::string input;

    /// The quality presets the game offers, as `graphics_presets` has them:
    /// the engine's `low`, `medium`, `high`, and `ultra`, changed value by
    /// value, added to, and dropped from by the project. What the presets
    /// are is part of what the project is; which one is chosen is a
    /// setting, `rendering.quality`.
    GraphicsPresets graphics_presets;
  };
} // neon

#endif //PROJECT_HPP
