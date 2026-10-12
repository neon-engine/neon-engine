#include "project-template-text.hpp"

#include <algorithm>
#include <string_view>

namespace neon::editor
{
  // Helpers of ProjectTemplateText: the name of the project put into a
  // text wherever `@name@` stands, and its organization for `@organization@`.
  namespace
  {
    std::string replaced(std::string text, const std::string_view what, const std::string_view with)
    {
      for (std::size_t at = text.find(what); at != std::string::npos; at = text.find(what, at + with.size()))
      {
        text.replace(at, what.size(), with);
      }
      return text;
    }

    std::string named(const std::string_view text, const NewProject &project)
    {
      return replaced(replaced(std::string(text), "@name@", project.name), "@organization@", project.organization);
    }
  }

  std::string ProjectTemplateText::CMakePath(const std::string &native_path)
  {
    std::string path = native_path;
    std::ranges::replace(path, '\\', '/');
    return path;
  }

  std::string ProjectTemplateText::ProjectFile(const NewProject &project)
  {
    return named(R"neon(# The project @name@: what it is called, who makes it, which scenes it has,
# and the input map it is played with. What the file holds is described in
# docs/projects.md of the engine.
#
# `name` and `organization` place user://, the folder the game writes to,
# so they are plain names that work as folder names on every platform.

version: 1
name: @name@
organization: @organization@

scenes:
  - assets://scenes/start.scene.yml

entry_scene: assets://scenes/start.scene.yml
input: assets://input/game.input.yml
)neon", project);
  }

  std::string ProjectTemplateText::Settings(const NewProject &project)
  {
    return named(R"neon(# The settings @name@ starts with: what it chooses for itself. What a
# player changes goes to user://settings.yml, on top of this. What is left
# out keeps the default of the engine; the names and what they take are
# listed in docs/settings.md of the engine.

version: 1

ui:
  pause_menu: engine://ui/pause.ui.yml
  settings_menu: engine://ui/settings.ui.yml
)neon", project);
  }

  std::string ProjectTemplateText::StartScene(const NewProject &project)
  {
    return named(R"neon(# The scene @name@ starts with: a box on a floor under one light, built by
# the engine from shapes, with nothing read from a file. The box carries a
# Spinner, the component of the project's extension, which turns it.

scene: start
version: 1

entities:
  - name: camera
    components:
      Transform:
        position: [0, 2, 5]
        rotation: [-15, 0, 0]
      Camera: Default

  - name: light
    components:
      Transform: Default
      Light:
        type: direction
        direction: [-0.5, -1, -0.3]
        ambient: [0.2, 0.2, 0.2]
        diffuse: [0.8, 0.8, 0.8]
        specular: [0, 0, 0]

  - name: floor
    components:
      Transform: Default
      Geometry:
        shape: plane
        size: [10, 0, 10]
      Renderable:
        shader: engine://shaders/basic-lit
        material:
          color: [0.5, 0.5, 0.5]

  - name: box
    components:
      Transform:
        position: [0, 0.5, 0]
      Geometry:
        shape: box
        size: [1, 1, 1]
      Renderable:
        shader: engine://shaders/basic-lit
        material:
          color: [0.9, 0.5, 0.2]
      Spinner:
        speed: 45
)neon", project);
  }

  std::string ProjectTemplateText::InputMap(const NewProject &project)
  {
    return named(R"neon(# The input map @name@ is played with: the actions a game reads, what is
# bound to each, and the states that say which are live. It starts as the
# map the engine brings, engine://input/default.input.yml, and is the
# project's own to change. What the file holds is described in docs/input.md
# of the engine.

version: 1

actions:
  move:  { type: axis2, keys: [w, s, a, d], stick: left }
  look:  { type: axis2, mouse: motion, stick: right, rate: 600 }
  jump:  { type: button, keys: [space], buttons: [south] }
  run:   { type: button, keys: [left-shift], buttons: [left-stick] }
  pause: { type: button, keys: [escape], buttons: [start] }

states:
  playing: [move, look, jump, run, pause]
  menu:    [pause]
)neon", project);
  }

  std::string ProjectTemplateText::ExtensionRecipe(const NewProject &project)
  {
    return named(R"neon(# The code of @name@ in C++: the library the runtime loads when it starts,
# one for each platform, built by the CMakeLists.txt at the root. What the
# file holds is described in docs/extensions.md of the engine.
version: 1
name: @name@

libraries:
  macos-arm64: @name@-macos-arm64.dylib
  macos-x86_64: @name@-macos-x86_64.dylib
  linux-x86_64: @name@-linux-x86_64.so
  linux-arm64: @name@-linux-arm64.so
  windows-x86_64: @name@-windows-x86_64.dll
  windows-arm64: @name@-windows-arm64.dll
)neon", project);
  }

  std::string ProjectTemplateText::ExtensionSource(const NewProject &project)
  {
    return named(R"neon(// The extension of @name@: its components, and the systems that run over
// them. It starts with one of each, a Spinner and the Spinning that turns
// what carries one, so that the first run shows the extension at work.
// See docs/extensions.md of the engine.

#include <neon/extension/neon-extension.hpp>

namespace game
{
  /// Turns the entity that carries it around the vertical axis.
  struct Spinner
  {
    float speed = 90.0f;
    float angle = 0.0f;
  };

  class Spinning final : public neon::extension::System
  {
    neon::extension::Query<Spinner> _spinners;
    NeonField _rotation = 0;

  public:
    void Start(neon::extension::World &world) override
    {
      _spinners = world.CreateQuery<Spinner>();
      _rotation = world.FindField("Transform", "rotation");
    }

    void FixedUpdate(neon::extension::World &world, const double step) override
    {
      _spinners.Each([&](const neon::extension::Entity entity, Spinner &spinner)
      {
        spinner.angle += spinner.speed * static_cast<float>(step);
        if (spinner.angle >= 360.0f) { spinner.angle -= 360.0f; }
        world.SetVector3(entity, _rotation, {0.0f, spinner.angle, 0.0f});
      });
    }
  };

  class Game final : public neon::extension::Extension
  {
  public:
    bool Initialize(neon::extension::World &world) override
    {
      AddSystem<Spinning>("Spinning");
      world.Info("The extension of @name@ is up");
      return true;
    }

    void RegisterComponents(neon::extension::World &world) override
    {
      using neon::extension::Field;

      world.RegisterComponent<Spinner>("Spinner", "Turns what carries it", {
        Field("speed", &Spinner::speed, "How far it turns in a second, in degrees"),
        Field("angle", &Spinner::angle),
      });
    }
  };
} // game

NEON_EXTENSION(game::Game)
)neon", project);
  }

  std::string ProjectTemplateText::CMakeLists(const NewProject &project)
  {
    return replaced(named(R"neon(# @name@: a game on Neon Engine, a project with an extension. See README.md.
#
# Nothing of the engine is compiled here. The engine is found by one path,
# and what is used of it is cmake/NeonSdk.cmake, the headers of extensions,
# and a NeonRuntime that is built already.
cmake_minimum_required(VERSION 3.25 FATAL_ERROR)

set(NEON_ENGINE_DIRECTORY "@engine@" CACHE PATH
        "The checkout of Neon Engine this is built against")

if (NOT EXISTS "${NEON_ENGINE_DIRECTORY}/cmake/NeonSdk.cmake")
  message(FATAL_ERROR
          "Neon Engine is not at ${NEON_ENGINE_DIRECTORY}. Say where the checkout is with "
          "-DNEON_ENGINE_DIRECTORY=<folder>")
endif ()

# A preset that builds for another platform names a toolchain file of the
# engine, which has to be set before the project is declared.
if (NEON_TOOLCHAIN AND NOT CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "${NEON_ENGINE_DIRECTORY}/cmake/toolchains/${NEON_TOOLCHAIN}.cmake")
endif ()

project(@name@ C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include("${NEON_ENGINE_DIRECTORY}/cmake/NeonSdk.cmake")

# The game is put together in the build folder: the runtime, its engine
# folder, assets/, and the extension built from these files. New files are
# listed here.
neon_add_project(@name@
        SOURCES
        extensions/@name@/@name@.cpp
)
)neon", project), "@engine@", CMakePath(project.engine_directory));
  }

  std::string ProjectTemplateText::CMakePresets()
  {
    return R"neon({
  "version": 6,
  "cmakeMinimumRequired": { "major": 3, "minor": 25, "patch": 0 },
  "configurePresets": [
    {
      "name": "base",
      "hidden": true,
      "generator": "Ninja",
      "binaryDir": "${sourceDir}/build/${presetName}",
      "cacheVariables": { "CMAKE_BUILD_TYPE": "Debug" },
      "warnings": { "dev": false, "deprecated": false }
    },
    {
      "name": "macos-arm64-debug",
      "displayName": "macOS arm64 (Debug)",
      "inherits": "base",
      "condition": { "type": "equals", "lhs": "${hostSystemName}", "rhs": "Darwin" },
      "cacheVariables": {
        "CMAKE_C_COMPILER": "/opt/homebrew/opt/llvm@20/bin/clang",
        "CMAKE_CXX_COMPILER": "/opt/homebrew/opt/llvm@20/bin/clang++"
      }
    },
    {
      "name": "linux-x64-debug",
      "displayName": "Linux x64 (Debug)",
      "inherits": "base",
      "condition": { "type": "equals", "lhs": "${hostSystemName}", "rhs": "Linux" },
      "cacheVariables": {
        "CMAKE_C_COMPILER": "clang",
        "CMAKE_CXX_COMPILER": "clang++",
        "CMAKE_CXX_FLAGS": "-stdlib=libc++"
      }
    },
    {
      "name": "windows-x64-debug",
      "displayName": "Windows x64 (Debug, cross-compiled with llvm-mingw)",
      "inherits": "base",
      "cacheVariables": { "NEON_TOOLCHAIN": "windows-x64-llvm-mingw" }
    },
    {
      "name": "release",
      "hidden": true,
      "inherits": "base",
      "cacheVariables": {
        "CMAKE_BUILD_TYPE": "Release",
        "CMAKE_C_FLAGS_RELEASE": "-O3 -DNDEBUG -fno-math-errno -ffunction-sections -fdata-sections",
        "CMAKE_CXX_FLAGS_RELEASE": "-O3 -DNDEBUG -fno-math-errno -ffunction-sections -fdata-sections",
        "CMAKE_INTERPROCEDURAL_OPTIMIZATION": "ON"
      }
    },
    {
      "name": "macos-arm64-release",
      "displayName": "macOS arm64 (Release)",
      "inherits": "release",
      "condition": { "type": "equals", "lhs": "${hostSystemName}", "rhs": "Darwin" },
      "cacheVariables": {
        "CMAKE_C_COMPILER": "/opt/homebrew/opt/llvm@20/bin/clang",
        "CMAKE_CXX_COMPILER": "/opt/homebrew/opt/llvm@20/bin/clang++",
        "CMAKE_C_FLAGS": "-mcpu=apple-m1",
        "CMAKE_CXX_FLAGS": "-mcpu=apple-m1"
      }
    },
    {
      "name": "linux-x64-release",
      "displayName": "Linux x64 (Release)",
      "inherits": "release",
      "condition": { "type": "equals", "lhs": "${hostSystemName}", "rhs": "Linux" },
      "cacheVariables": {
        "CMAKE_C_COMPILER": "clang",
        "CMAKE_CXX_COMPILER": "clang++",
        "CMAKE_C_FLAGS": "-march=x86-64-v2",
        "CMAKE_CXX_FLAGS": "-stdlib=libc++ -march=x86-64-v2"
      }
    },
    {
      "name": "windows-x64-release",
      "displayName": "Windows x64 (Release, cross-compiled with llvm-mingw)",
      "inherits": "release",
      "cacheVariables": {
        "NEON_TOOLCHAIN": "windows-x64-llvm-mingw",
        "CMAKE_C_FLAGS": "-march=x86-64-v2",
        "CMAKE_CXX_FLAGS": "-march=x86-64-v2"
      }
    }
  ],
  "buildPresets": [
    { "name": "macos-arm64-debug", "configurePreset": "macos-arm64-debug" },
    { "name": "macos-arm64-release", "configurePreset": "macos-arm64-release" },
    { "name": "linux-x64-debug", "configurePreset": "linux-x64-debug" },
    { "name": "linux-x64-release", "configurePreset": "linux-x64-release" },
    { "name": "windows-x64-debug", "configurePreset": "windows-x64-debug" },
    { "name": "windows-x64-release", "configurePreset": "windows-x64-release" }
  ]
}
)neon";
  }

  std::string ProjectTemplateText::GitIgnore()
  {
    return R"neon(# what a build writes
/build/

# the editor's file of this user: what they chose on this machine
user-editor.yml

# what a desktop and an IDE leave in folders
.DS_Store
.idea/
)neon";
  }

  std::string ProjectTemplateText::ReadMe(const NewProject &project)
  {
    return replaced(named(R"neon(# @name@

A game on [Neon Engine](@engine@), made by @organization@: a project with
an extension in C++. It starts as a box on a floor that the extension turns.

## Building and running

The engine is built once, with a preset of its own, which leaves a
NeonRuntime under its `bin/`. Then, from this folder, which puts the game
together under the name of the project:

```sh
cmake --preset macos-arm64-debug
cmake --build --preset macos-arm64-debug
build/macos-arm64-debug/@name@/@name@
```

The presets are `macos-arm64-debug`, `linux-x64-debug`, `windows-x64-debug`,
and the same with `-release`; every project made by the editor has these
and is built with these commands. Only the extension is compiled: the game
is put together in the build folder with a runtime the engine built
already, its release build, else its debug build, or the folder
`-DNEON_RUNTIME_DIRECTORY=<folder with NeonRuntime and its engine folder>`
names.

## Where the engine is

`CMakeLists.txt` names the engine as the default of `NEON_ENGINE_DIRECTORY`,
the folder with `cmake/NeonSdk.cmake`: the checkout the editor that made
this project was built from. Another is given
with `-DNEON_ENGINE_DIRECTORY=<folder>`.

## What is where

| Path | Holds |
|---|---|
| `assets/project.yml` | What the project is: its name, its scenes, its input map |
| `assets/settings.yml` | What it chooses for itself; a player's changes go on top |
| `assets/scenes/` | The scenes, `*.scene.yml` |
| `assets/input/` | The input maps, `*.input.yml` |
| `extensions/@name@/` | The code in C++, and the recipe of the extension |

The layout is described in `docs/project-layout.md` of the engine.
)neon", project), "@engine@", CMakePath(project.engine_directory));
  }
} // neon::editor
