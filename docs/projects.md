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
The file says what the project is called, who makes it, and which scenes it
has. What a game chooses for itself and what a player may change is not in
it; that is `settings.yml`, see [settings.md](settings.md), which the runtime
reads after the project.

## The file

```yaml
version: 1
name: neon-runtime
organization: neon-engine

scenes:
  - assets://scenes/demo.scene.yml
  - assets://scenes/physics.scene.yml

entry_scene: assets://scenes/demo.scene.yml
```

The project of the runtime is
[project.yml](../app/NeonRuntime/assets/project.yml).

| Name | Holds | When it is left out |
|---|---|---|
| `version` | The version of this layout, which is 1, as in every recipe, see [recipes.md](recipes.md#the-version) | An error. A file of a later layout is refused with its version |
| `name` | What the project is called. A plain name: lowercase letters, digits, and dashes, starting with a letter | An error, as is an empty name |
| `organization` | Who makes it, in the same plain form | `neon-engine`: a project without one is one of the engine's own |
| `scenes` | The scenes of the project, as virtual paths. At least one | An error |
| `entry_scene` | The scene the project starts with. One of `scenes` | The first of `scenes` |

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

## How the runtime starts

| Step | What happens | Why in this order |
|---|---|---|
| 1 | The command line is read | Options such as `--headless` and `--output-dir` decide how the file system comes up |
| 2 | The file system finds `assets://` and `output://` | `assets://` is next to the executable, `output://` is `--output-dir`. `user://` has no folder yet, and every `user://` path is refused |
| 3 | `assets://project.yml` is read | It is the only file that can be read before `user://` exists, and it is what places `user://` |
| 4 | `user://` is placed under the organization and the name | `FileSystem::PlaceUserDirectory`. The SDL2 backend asks the platform for the folder and creates it |
| 5 | The log file is opened under `user://` | What was logged before is written into it first, see [file-systems.md](file-systems.md#the-log-file) |
| 6 | `assets://settings.yml`, then `user://settings.yml`, then the command line again | The layers of settings, see [settings.md](settings.md). The command line wins |
| 7 | The rest of the systems start, and the entry scene is loaded | `--scene` names another scene for development. A shipped runtime runs what its project says (#143) |

The problems of the project go to the console, since the log file has no
place yet when the project is read.

## How it is built

| Piece | Location | Role |
|---|---|---|
| `Project` | neon-core, `neon/project/project.hpp` | What the file holds, as values |
| `ProjectFile` | neon-core, `neon/project/project-file.hpp` | Reads the file through the file system and a `DocumentFormat`, checks it, and collects every problem |
| `FileSystem::PlaceUserDirectory` | neon-core | The step that gives `user://` its folder, implemented by each backend |
| `main.cpp` | NeonRuntime | Reads the project, places `user://`, and carries the names and the entry scene into `SettingsConfig` |

`ProjectFile` knows no format: YAML comes from the `RYML_DocumentFormat`
that the application hands in, as it does for scenes. The tests of the reader
are in `tests/project-files`, with the file of the runtime read as it is in
the repository and its scenes checked against the folder.

How the folder is organised is in [project-layout.md](project-layout.md).

## Open questions

- **A compiled game, not a project** (#92, #143, #84): a shipped NeonRuntime
  knows nothing of `project.yml`. The exporter turns the project into a
  `game.yml` — the name, the organization, the entry scene, and the runtime
  it was built for — either embedded into the runtime or in a file next to
  it, with the assets. `ProjectFile` then belongs to the editor, which wraps
  or grows out of NeonRuntime with its own command-line options; nothing is
  split in the build for that.
- **More in the file**: a version of the project, the input maps (#117), the
  scene manager's names for scenes (#118), and the icon of the window are
  likely to come here. Each is added when its feature does.
- **A shipped runtime** (#143) runs only its project. `--scene` and the other
  options that load content are then for the editor.
