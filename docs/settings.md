# Settings

This note records how the runtime is told what to do: what a game chooses for
itself, what a player may change, and what the command line sets. What a
project *is* — its name, who makes it, its scenes — is not a setting; that is
[projects.md](projects.md).

**Current decision:** settings come in layers, each read on top of the one
before. A layer only changes what it writes down.

| Layer | Where | Who writes it |
|---|---|---|
| 1 | The defaults in `SettingsConfig` | The engine |
| 2 | `assets://settings.yml` | The author of the project. It ships with the game |
| 3 | `user://settings.yml` | The player, through a settings menu. Not written by anything yet |
| 4 | The command line | Whoever starts the runtime, see [command-line.md](command-line.md) |

The command line is applied twice: once before the file system comes up,
since `--headless-renderer` and `--output-dir` decide how it does, and once after the
files, so that it wins. Applying it twice gives the same result; the options
are written so.

## The file

```yaml
version: 1

window:
  title: A Game
  width: 1920
  height: 1080
  mode: borderless

ui:
  scale: 1
  start: assets://ui/hud.ui.yml
  pause_menu: assets://ui/pause.ui.yml
  settings_menu: assets://ui/settings.ui.yml

world:
  steps_per_second: 60
  most_steps_per_frame: 8

input:
  gyro: false

rendering:
  vulkan_version: "1.3"
  max_light_sources: 1024
```

The settings of the runtime are
[settings.yml](../app/NeonRuntime/assets/settings.yml).

| Name | Holds | Default |
|---|---|---|
| `version` | The version of this layout, which is 1, as in every recipe, see [recipes.md](recipes.md#the-version) | Required |
| `window.title` | The title of the window | The name of the project |
| `window.width`, `window.height` | The size of the window in points, and of the frame without a window. Whole numbers above zero | 1280 by 720 |
| `window.mode` | `windowed`, `borderless`, or `fullscreen`, see the [development guide](development.md#window-modes) | `windowed` |
| `ui.scale` | What the user interface is made larger or smaller by. Above zero | 1 |
| `ui.start` | A user interface shown from the start, on top of the scene | None |
| `ui.pause_menu` | The menu shown when the player pauses | None, and pause does nothing |
| `ui.settings_menu` | The menu the pause menu's `settings` button opens | None, and the button does nothing |
| `world.steps_per_second` | Steps the world takes in a second. Above zero | 60 |
| `world.most_steps_per_frame` | The most steps one frame takes. A whole number above zero | 8 |
| `input.gyro` | The player's switch for the gyro of a controller, over what the input map says, see [input.md](input.md#sensors) | Left to the map, which has it off unless an action says `enabled: true` |
| `rendering.vulkan_version` | The version of Vulkan to ask for, in quotes, since `1.10` as a number is `1.1` | `"1.3"` |
| `rendering.max_light_sources` | How many lights a frame may hold. A whole number above zero | 1024 |

A name that is not known is an error, as in every recipe: the file is a
configuration file, not a recipe, since its name says no kind, and the rules
of [recipes.md](recipes.md#made-to-be-changed-by-hand) hold for it. Every
problem is reported with its line, not only the first, and a file with a
mistake changes nothing: the settings are read on top of a copy, so that a
file does not apply by halves.

A mistake in the project's file stops the runtime, since the game would not
be what its author meant. A mistake in the player's file is said in the log
and the file is left out, since a player should not be locked out of the game
by a file a menu wrote.

## What is not here

| What | Where it is decided | Why not a setting |
|---|---|---|
| The name and organization of the project, the scenes, the entry scene | `project.yml`, see [projects.md](projects.md) | They say what the project is. A player may not change them |
| The actions of the game and what is bound to them | The input map, see [input.md](input.md) | A project's recipe. What a player rebinds will be a layer of its own, `user://input.yml`. Whether a sensor is on is a setting, `input.gyro` |
| `--headless-renderer`, `--frames`, `--screenshot`, `--output-dir`, `--time-step`, `--input` | The command line only | They are for a run, not for a game |
| The volume of the sound groups | Nothing yet | They belong in layer 3 once a settings menu writes it, through the audio interfaces of #71 |
| The present mode, the renderer | Nothing yet | Open in [command-line.md](command-line.md#open-questions) |

## How it is built

| Piece | Location | Role |
|---|---|---|
| `SettingsConfig` | neon-core, `neon/runtime/settings-config.hpp` | Every setting, with its default |
| `SettingsFile` | neon-core, `neon/settings/settings-file.hpp` | Reads one file on top of a `SettingsConfig` through the file system and a `DocumentFormat`, checks it, and collects every problem |
| `RuntimeOptions`, `DisplayOptions` | neon-core, `neon/command-line/` | The command line, layer 4 |
| `main.cpp` | NeonRuntime | Reads the layers in order |

The tests of the reader are in `tests/settings-files`, with the file of the
runtime read as it is in the repository.

## Open questions

- **A settings menu that writes layer 3** (#125 follow-up): the menu of
  `settings.ui.yml` changes volumes today and keeps nothing. Writing
  `user://settings.yml` needs `DocumentFormat::Write` on the values that
  changed, and the volumes of the sound groups as settings.
- **Which command-line options become settings**: `--window-size`,
  `--ui-scale`, `--vulkan-version`, and `--renderer` have a place here now;
  whether a shipped runtime keeps the options is #143.
- **Per-platform settings**: a project may want another size or mode on
  another platform. Not needed yet.
