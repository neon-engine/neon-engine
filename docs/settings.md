# Settings

This note records how the runtime is told what to do: what a game chooses for
itself, what a player may change, and what the command line sets. What a
project *is* - its name, who makes it, its scenes - is not a setting; that is
[projects.md](projects.md).

**Current decision:** settings come in layers, each read on top of the one
before. A layer only changes what it writes down.

| Layer | Where | Who writes it |
|---|---|---|
| 1 | The defaults in `SettingsConfig` | The engine |
| 2 | `assets://settings.yml` | The author of the project. It ships with the game |
| 3 | `user://settings.yml` | The player, through a settings menu: the graphics of the runtime's menu are written there with Apply, see `PlayerSettings` |
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
  pause_menu: engine://ui/pause.ui.yml
  settings_menu: engine://ui/settings.ui.yml

world:
  steps_per_second: 60
  most_steps_per_frame: 8

input:
  gyro: false
  mouse_switches_device: true

rendering:
  vulkan_version: "1.3"
  max_light_sources: 1024
  max_render_objects: 16384
  tonemapper: none
  exposure: 1
  vsync: true
  max_fps: 0

audio:
  groups:
    - radio
    - name: crowd
      volume: 0.5
  volumes:
    music: 0.6
    ambience: 0.5
```

Every project brings its own. Those of the sandbox are
[settings.yml](../projects/sandbox/assets/settings.yml).

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
| `input.mouse_switches_device` | Whether moving or clicking the mouse makes keyboard and mouse what the player uses, as a key does, so that the hints of a gamepad go. A game played with a gamepad whose stick or gyro is bound to the mouse outside the game, as Steam Input can, sets it to `false`: the hints of the gamepad stay until a key is pressed. See [input.md](input.md#the-device-in-use) | `true` |
| `scripting.jit` | Whether LuaJIT compiles the scripts to machine code as they run; `false` runs them in its interpreter, for comparing the two, see [scripting.md](scripting.md#which-lua) | `true` |
| `rendering.vulkan_version` | The version of Vulkan to ask for, in quotes, since `1.10` as a number is `1.1` | `"1.3"` |
| `rendering.max_light_sources` | How many lights a frame may hold. A whole number above zero | 1024 |
| `rendering.max_render_objects` | How many render objects a frame may hold, each one draw. A whole number above zero. It sizes a per-frame buffer of twice 208 bytes an object, once for the scene and once for the shadow pass, so a larger number costs little; a scene that passes it is told, with this name | 16384 |
| `rendering.shadow_distance` | How far from the camera the shadow of the direction light reaches, in meters, along its view, see [vulkan-renderer.md](vulkan-renderer.md#shadows). What is further is lit. Farther is coarser in the far cascades. Above zero | 120 |
| `rendering.shadow_cascades` | How many cascades the shadow map has, 1 to 4: slices of what the camera sees, the nearest drawn the finest. More is finer near the camera at the same distance, and the casters drawn once more each | 4 |
| `rendering.tonemapper` | The curve the resolve step maps light brighter than white through: `none` cuts it off flat, `aces` and `agx` roll it off, see [vulkan-renderer.md](vulkan-renderer.md#tonemapping) | `none` |
| `rendering.vsync` | Whether a frame waits for the screen before it is shown. With it no frame is torn and no more frames are drawn than the screen shows; without it frames are shown as soon as they are done, where the driver can. See [vulkan-renderer.md](vulkan-renderer.md#vertical-sync-and-the-window) | `true` |
| `rendering.anisotropy` | How many samples a texture is read with where its surface is seen from the side, which keeps a floor sharp into the distance: 1 for none, 2, 4, 8, or 16. Held to what the graphics card allows. See [vulkan-renderer.md](vulkan-renderer.md#what-the-shaders-bind) | 8 |
| `rendering.texture_scale` | The size textures read from files are kept at, for a graphics card with little memory: 1, 0.5, 0.25, or 0.125 of their size. Each step leaves out the largest level of the smaller copies a texture has, made before it is uploaded; no side goes below 32 pixels. Images of the user interface, fonts, render targets, and images made while the game runs keep their size. See [vulkan-renderer.md](vulkan-renderer.md#what-the-shaders-bind) | 1 |
| `rendering.target_scale` | The size what a camera draws into is made at: 1, 0.5, or 0.25 of the size its camera asks for, never below 16 pixels a side. Fewer pixels to draw, and a level fewer to make for every halving. Images of the user interface keep their size, so that their text stays sharp. See [user-interface.md](user-interface.md#what-a-camera-sees) | 1 |
| `rendering.target_mipmaps` | The most levels of smaller copies a render target has, which are made again in every frame it is drawn into: 0 for as many as its size allows, 1 for none, up to 16. Lowers what a camera asks for with `mipmaps`, never raises it | 0 |
| `rendering.max_fps` | The most frames a second: a whole number from 30 to 300, or 0 for as many as can be drawn. It holds with `vsync` on and off, whichever of the two allows fewer. See [vulkan-renderer.md](vulkan-renderer.md#vertical-sync-and-the-window) | 0 |
| `rendering.exposure` | How bright the scene is taken to be: the light of the scene is multiplied by it before the curve. A number above zero; 2 doubles the light, 0.5 halves it | 1 |
| `audio.groups` | The [groups of sounds](audio.md#groups) the project has besides those of the engine: a list of names, or of maps with a `name` and the `volume` the group starts at. A name that is there already is an error. A group of the project is held still while the game is paused, as the effects are | None. The groups of the engine are `music`, `effects`, `voices`, and `ambience` |
| `audio.volumes` | The volume of a group by its name, 0 for silence and 1 for the loudness of its sounds, not below 0. A name that is not a group of the engine or of the project, declared above or in a file read before, is an error | 1 for every group |

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
| `--headless-renderer`, `--frames`, `--screenshot`, `--output-dir`, `--time-step`, `--input` | The command line only, in the editor's set | They are for a run, not for a game |
| How the window is shown while the game runs: its mode, its size, vertical sync | `WindowContext::SetWindowMode`, `SetWindowSize`, `RenderContext::SetVerticalSync`, and the same for an extension | A menu changes them at once, and the game keeps what the player chose, as it keeps its other settings. See [vulkan-renderer.md](vulkan-renderer.md#vertical-sync-and-the-window) |

## How it is built

| Piece | Location | Role |
|---|---|---|
| `SettingsConfig` | neon-core, `neon/runtime/settings-config.hpp` | Every setting, with its default |
| `SettingsFile` | neon-core, `neon/settings/settings-file.hpp` | Reads one file on top of a `SettingsConfig` through the file system and a `DocumentFormat`, checks it, and collects every problem |
| `PlayerSettings` | neon-core, `neon/settings/player-settings.hpp` | Writes what a settings menu changed to `user://settings.yml`, keeping what the file held already |
| `GraphicsMenu` | neon-core, `neon/runtime/graphics-menu.hpp` | The graphics of the runtime's settings menu: shows them, changes them, and keeps or puts them back |
| `RuntimeOptions`, `DisplayOptions` | neon-core, `neon/command-line/` | The command line, layer 4 |
| `main.cpp` | NeonRuntime | Reads the layers in order |

The tests of the reader are in `tests/settings-files`, with the file of the
runtime read as it is in the repository.

## Open questions

- **The volumes in layer 3** (#125 follow-up): the menu of `settings.ui.yml`
  writes the graphics to `user://settings.yml` with Apply (#356), through
  `PlayerSettings`, but not the volumes yet, which it changes and keeps
  nothing of. The volumes of the sound groups are settings, `audio.volumes`,
  and the menu's sliders start at the values of its file, not at them.
- **The renderer as a setting**: `--window-size`, `--window-mode`,
  `--ui-scale`, and `--vulkan-version` have a place here now, and the
  runtime owns the options too (#143, see [command-line.md](command-line.md)).
  `--renderer` has no setting yet, since there is one renderer.
- **Per-platform settings**: a project may want another size or mode on
  another platform. Not needed yet.
