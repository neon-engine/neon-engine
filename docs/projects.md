# Projects

This note records what a project is on disk, how the runtime loads one, and
what is still open.

**Current decision:** a project is a folder with a `project.yml` at its root.
It is the author's file: what the editor and the exporter read. The runtime
runs one project today, the folder that `assets://` stands for, which the
build puts next to the executable, since there is no editor and no exporter
yet and the runtime is the player the editor will grow into or wrap. A
shipped runtime will instead run a **compiled game** and read what the
exporter made of the project, see [the open questions](#open-questions).
The file says what the project is called, who makes it, which scenes it
has, and which quality presets it offers. What a game chooses for itself and
what a player may change is not in it; that is `settings.yml`, see
[settings.md](settings.md), which the runtime reads after the project.

## The file

```yaml
version: 1
name: neon-sandbox
organization: neon-engine

scenes:
  - assets://scenes/title.scene.yml
  - assets://scenes/level.scene.yml

entry_scene: assets://scenes/title.scene.yml
input: assets://input/game.input.yml

graphics_presets:
  low:
    texture_scale: 1
  potato:
    anisotropy: 1
    shadow_cascades: 1
  ultra: off
```

NeonRuntime ships no project: it is a runtime and knows no game. Every game
is a project of its own, put together next to a copy of the runtime, as the
sandbox's [project.yml](../projects/sandbox/assets/project.yml), with one
scene, and the museum's, [project.yml](../projects/museum/assets/project.yml),
see [scenes.md](scenes.md#where-the-scenes-are). Started without one, the
runtime says so and stops.

| Name | Holds | When it is left out |
|---|---|---|
| `version` | The version of this layout, which is 1, as in every recipe, see [recipes.md](recipes.md#the-version) | An error. A file of a later layout is refused with its version |
| `name` | What the project is called. A plain name: lowercase letters, digits, and dashes, starting with a letter | An error, as is an empty name |
| `organization` | Who makes it, in the same plain form | `neon-engine`: a project without one is one of the engine's own |
| `scenes` | The scenes of the project, as virtual paths. At least one | An error |
| `entry_scene` | The scene the project starts with. One of `scenes` | The first of `scenes` |
| `input` | The input map the game is played with, as a virtual path, see [input.md](input.md) | The map the engine brings, `engine://input/default.input.yml` |
| `graphics_presets` | The [quality presets](#quality-presets) the game offers, by name: a map of the values a preset decides changes a preset of the engine or adds one, and `off` drops one | The four of the engine, `low`, `medium`, `high`, and `ultra`, as they are |

A name that is not known is an error, as in every recipe, so that a name
that was misspelled does not go unnoticed. Every problem is reported with its
line, not only the first, and the runtime stops before anything else starts.
The file is a configuration file, not a recipe, since its name says no
kind; it is read by the same reader as every recipe and follows the rules of
[recipes.md](recipes.md#made-to-be-changed-by-hand).

### Why plain names

`name` and `organization` become a folder: `user://` is the folder the
platform keeps for an application, under `<organization>/<application>`, see
[file-systems.md](file-systems.md). A folder name has to work on macOS,
Linux, and Windows alike, and read well in a path, which rules out spaces,
capitals that only some file systems tell apart, and punctuation. The title
of the window is the name too, until a project can say otherwise.

Two projects of one organization share the organization's folder and have a
folder each inside it. Two organizations never share one.

### Quality presets

A quality preset sets every graphics setting that costs frame time at once:
`rendering.quality` of the settings names one, `--quality` on the command
line does, and so does the Quality row of the settings menu, see
[settings.md](settings.md#quality-presets). Which presets there are is part
of what the project is, not a setting a layer changes, so the table is
written here, and the settings and the player's file only choose from it.
The engine brings four, which every project has without a word:

| Preset | `anisotropy` | `texture_scale` | `target_scale` | `target_mipmaps` | `shadow_map_size` | `shadow_filter` | `shadow_cascades` | `shadow_distance` |
|---|---|---|---|---|---|---|---|---|
| `low` | 2 | 0.5 | 0.5 | 1 | 1024 | `none` | 1 | 40 |
| `medium` | 4 | 1 | 1 | 4 | 2048 | `pcf` | 2 | 80 |
| `high` | 8 | 1 | 1 | 0 | 2048 | `pcf` | 4 | 120 |
| `ultra` | 16 | 1 | 1 | 0 | 4096 | `pcf` | 4 | 160 |

`graphics_presets` is a map of presets by name. Each name holds a map of the
values a preset decides, the eight columns above, each taking what the
setting of that name takes in [settings.md](settings.md#the-file) and
refused as it is refused there, or `off`:

| A name that | Does |
|---|---|
| Is one of the engine's, with a map | Changes that preset value by value. What the map leaves out stays as the engine has it, so a project writes only what differs: `low: {texture_scale: 1}` is the engine's `low` with full-size textures |
| Is new, with a map | Adds a preset after the engine's, in the order the file writes them. What the map leaves out is the default of the engine for that setting, which is `high`: a `potato` with only `anisotropy: 1` is `high` without filtering |
| Is one of the engine's, with `off` | Drops it from the table, so that the menu does not offer it and the settings cannot name it. `off` is one word on the line of the preset, next to the maps, rather than a second list to keep in step with them |
| Is `custom` | An error. `custom` is the name of no preset: it is what the menu shows when the values match none, and what a file writes to change nothing |
| Is not a plain name, as `name` is | An error, since a preset is named in the settings, on the command line, and as the value of the menu's row |

The order of the menu's names is the engine's, with the project's after
them and `custom` last. A table with every preset turned off is an error:
the settings menu offers at least one. A `presets` map under `rendering` of
`settings.yml` is an error that points here, so that a file written for the
wrong place does not go unnoticed.

The sandbox adds a `potato` for a machine that struggles, see its
[project.yml](../projects/sandbox/assets/project.yml). The runtime reads
the table before the settings files, so that `assets://settings.yml` names
a preset of the project, and keeps it in `SettingsConfig::graphics_presets`,
which `rendering.quality`, `--quality`, and `GraphicsMenu` work from.

## How the runtime starts

| Step | What happens | Why in this order |
|---|---|---|
| 1 | The command line is read | Options such as `--headless-renderer` and `--output-dir` decide how the file system comes up |
| 2 | The file system finds `assets://`, `extensions://`, and `output://` | `assets://` and `extensions://` are next to the executable, `output://` is `--output-dir`. `user://` has no folder yet, and every `user://` path is refused |
| 3 | `assets://project.yml` is read | It is the only file that can be read before `user://` exists, and it is what places `user://` |
| 4 | `user://` is placed under the organization and the name | `FileSystem::PlaceUserDirectory`. The SDL2 backend asks the platform for the folder and creates it |
| 5 | The log file is opened under `user://` | What was logged before is written into it first, see [file-systems.md](file-systems.md#the-log-file) |
| 6 | `assets://settings.yml`, then `user://settings.yml`, then the command line again | The layers of settings, see [settings.md](settings.md). The command line wins. The display options, `--quality` among them, are applied here for the first time, since a preset they name may be the project's |
| 7 | The input map the project names is read, `InputMapFile` | The input system has it before a script of input is checked against it, see [input.md](input.md) |
| 8 | The rest of the systems start, and the entry scene is loaded | `--scene` names another scene for development. A shipped runtime runs what its project says (#143) |

The problems of the project go to the console, since the log file has no
place yet when the project is read.

## How it is built

| Piece | Location | Role |
|---|---|---|
| `Project` | neon-core, `neon/project/project.hpp` | What the file holds, as values |
| `ProjectFile` | neon-core, `neon/project/project-file.hpp` | Reads the file through the file system and a `DocumentFormat`, checks it, and collects every problem |
| `GraphicsPresets`, `GraphicsPresetReader` | neon-core, `neon/render/` | The table of the quality presets, the engine's four and what the project does to them, and the reader of a preset's values, which `rendering` of a settings file is read with too |
| `FileSystem::PlaceUserDirectory` | neon-core | The step that gives `user://` its folder, implemented by each backend |
| `main.cpp` | NeonRuntime | Reads the project, places `user://`, and carries the names, the entry scene, and the quality presets into `SettingsConfig` |

`ProjectFile` knows no format: YAML comes from the `RYML_DocumentFormat`
that the application hands in, as it does for scenes. The tests of the reader
are in `tests/project-files`, with the file of the runtime read as it is in
the repository and its scenes checked against the folder.

How the folder is organized is in [project-layout.md](project-layout.md).

## Open questions

- **A compiled game, not a project** (#92, #143, #84): a shipped NeonRuntime
  knows nothing of `project.yml`. The exporter turns the project into a
  `game.yml` - the name, the organization, the entry scene, and the runtime
  it was built for - either embedded into the runtime or in a file next to
  it, with the assets. `ProjectFile` then belongs to the editor, which wraps
  or grows out of NeonRuntime with its own command-line options; nothing is
  split in the build for that.
- **More in the file**: a version of the project, names for scenes so that a
  button or an exit says `level-2` in place of a path, and the icon of the
  window are likely to come here. Each is added when its feature does, as
  `input` was with the input maps.
- **A shipped runtime** (#143) runs only its project. `--scene` and the other
  options that load content are then for the editor.

## A project with code in C++

A game's code is Lua, see [scripting.md](scripting.md), or C++ in an
extension, see [extensions.md](extensions.md), or both. What a game is made
of follows from that:

| A game is | Then it has |
|---|---|
| A project alone | `assets/` with its `project.yml`, scenes, and scripts in Lua. Nothing is compiled |
| An extension alone | `extensions/<name>/` with its library and what it needs to run, put next to a runtime that runs some project. A loader for another engine's files, a tool, something several projects use |
| A project and an extension | Both, side by side. The bench is one |

```
bench/
  CMakeLists.txt             neon_add_project(bench SOURCES extensions/bench/bench.cpp ...)
  README.md
  assets/
    project.yml              the project: its name, its scenes, the scene it starts with
    scenes/ prefabs/ ...       the game: its scenes, prefabs, scripts, user interfaces
  extensions/
    bench/                   laid out as it lies next to the runtime
      extension.yml
      bench.cpp ...            its code
```

| Decision | Reason |
|---|---|
| The code is an extension, a library next to the runtime | The runtime is not built again for a game. Whoever writes the game builds the game's own files and nothing else, see [extensions.md](extensions.md#building-an-extension) |
| The game lives in `assets/`, even what uses the extension's components | `extensions://` is for code and what code needs to run; a scene is the game's, so a scene that holds a `NativeSpawner` is `assets://scenes/rain-cpp-heavy-copy.scene.yml` |
| `extensions/<name>/` in the source is laid out as next to the runtime | One layout to know. The build adds the library to it |
| `project.yml` is in `assets/` | It is where the runtime reads it. It belongs at the root, next to `assets/` and `extensions/`, which waits for a scheme of its own (#368) |
| `neon_add_project(<name> SOURCES ...)` puts the game together: `NeonRuntime`, `engine/`, `assets/`, `extensions/<name>/` | It is what a shipped game looks like, and it runs from there. Exporting (#84) will make the same folder for another platform |
| The runtime brings its executable and its folder `engine`, and nothing of a game | The runtime knows no game. Its shaders, fonts, a plain pause and settings menu, and an input map are `engine://`, which a project uses or replaces; everything else a game shows and reads is the project's own |

**Built on its own**, which is how a game is worked on: the folder is a
build of its own that includes `cmake/NeonSdk.cmake` of the engine. Only the
extension is compiled, with any compiler, and the game is put together in
the build folder with a runtime that is built already, the engine's release
build, else its debug build, or the folder `-DNEON_RUNTIME_DIRECTORY=` names.

```sh
cmake -S projects/bench -B build/bench -G Ninja
cmake --build build/bench
build/bench/bench/NeonRuntime
```

**Built by the engine**, for a project in `projects/` of the engine: the
folders there are found by the engine's build, so that a new one needs no
edit outside its folder. The target puts the game together under
`bin/<build>/<platform>/<name>/`, with the runtime the engine builds. It is
not part of the default target: the build presets name each project, so
`cmake --build --preset <preset>` builds them with the runtime, and a build
without a preset builds the engine alone. A new project is added to the
`targets` of the build presets in `CMakePresets.json`.

```sh
cmake --build --preset macos-arm64-debug
bin/debug/darwin-arm64/bench/NeonRuntime
```

The functions are in [NeonProjects.cmake](../cmake/NeonProjects.cmake) and
[NeonSdk.cmake](../cmake/NeonSdk.cmake).
