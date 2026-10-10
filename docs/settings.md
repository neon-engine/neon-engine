# Settings

This note records how the runtime is told what to do: what a game chooses for
itself, what a player may change, and what the command line sets. What a
project *is* - its name, who makes it, its scenes - is not a setting; that is
[projects.md](projects.md).

**Current decision:** settings come in layers, each read on top of the one
before. A layer only changes what it writes down, with one exception: a
quality preset holds over the graphics values it decides, see
[Quality presets](#quality-presets).

| Layer | Where | Who writes it |
|---|---|---|
| 1 | The defaults in `SettingsConfig` | The engine |
| 2 | `assets://settings.yml` | The author of the project. It ships with the game |
| 3 | `user://settings.yml` | The player, through a settings menu: the graphics and the volumes of the runtime's menu and the settings of the game are written there with Apply, see `PlayerSettings` |
| 4 | The command line | Whoever starts the runtime, see [command-line.md](command-line.md) |

The command line is applied twice: once before the file system comes up,
since `--headless-renderer` and `--output-dir` decide how it does, and once after the
files, so that it wins. Applying it twice gives the same result; the options
are written so. `--quality` alone waits for the second pass, since the
preset it names may be one the project defines.

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
  quality: high
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

game:
  difficulty: hard
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
| `rendering.quality` | A preset of the graphics that cost frame time, by name: one of the project's table, which is the engine's `low`, `medium`, `high`, and `ultra` unless its `project.yml` says otherwise, or `custom` for none, see [Quality presets](#quality-presets). A preset holds: the names below that it decides are left out with a warning, in this file and in a layer after it, until a layer writes `custom` and then its own values | `custom`: no preset holds, and the values below, which are those of `high`, apply as a file writes them |
| `rendering.shadow_distance` | How far from the camera the shadow of the direction light reaches, in meters, along its view, see [vulkan-renderer.md](vulkan-renderer.md#shadows). What is further is lit. Farther is coarser in the far cascades. Above zero | 120 |
| `rendering.shadow_cascades` | How many cascades the shadow map has, 1 to 4: slices of what the camera sees, the nearest drawn the finest. More is finer near the camera at the same distance, and the casters drawn once more each | 4 |
| `rendering.shadows` | Whether the direction light casts shadows at all. `false` leaves the shadow pass out of the frame and shades every light as if it had `casts_shadows: false`, so a frame is byte for byte what it is for such a light; the other shadow settings are kept for when it is `true` again. A flag, as `vsync` is | `true` |
| `rendering.shadow_map_size` | How many texels the shadow map has along each side, every cascade: 512, 1024, 2048, or 4096. More is a sharper shadow at the same distance, and more to draw and to hold, four bytes a texel a cascade. Changed while the game runs, the map is made again at the new size before the next frame | 2048 |
| `rendering.shadow_filter` | How the shadow map is compared against at a point: `none` compares once, which the sampler blends across the four texels around the place, so the edge of a shadow is about a texel wide; `pcf` compares nine times one texel apart and averages, which softens the edge over about three texels. See [vulkan-renderer.md](vulkan-renderer.md#shadows) | `pcf` |
| `rendering.tonemapper` | The curve the resolve step maps light brighter than white through: `none` cuts it off flat, `aces` and `agx` roll it off, see [vulkan-renderer.md](vulkan-renderer.md#tonemapping) | `none` |
| `rendering.vsync` | Whether a frame waits for the screen before it is shown. With it no frame is torn and no more frames are drawn than the screen shows; without it frames are shown as soon as they are done, where the driver can. See [vulkan-renderer.md](vulkan-renderer.md#vertical-sync-and-the-window) | `true` |
| `rendering.anisotropy` | How many samples a texture is read with where its surface is seen from the side, which keeps a floor sharp into the distance: 1 for none, 2, 4, 8, or 16. Held to what the graphics card allows. See [vulkan-renderer.md](vulkan-renderer.md#what-the-shaders-bind) | 8 |
| `rendering.texture_scale` | The size textures read from files are kept at, for a graphics card with little memory: 1, 0.5, 0.25, or 0.125 of their size. Each step leaves out the largest level of the smaller copies a texture has, made before it is uploaded; no side goes below 32 pixels. Images of the user interface, fonts, render targets, and images made while the game runs keep their size. See [vulkan-renderer.md](vulkan-renderer.md#what-the-shaders-bind) | 1 |
| `rendering.target_scale` | The size what a camera draws into is made at: 1, 0.5, or 0.25 of the size its camera asks for, never below 16 pixels a side. Fewer pixels to draw, and a level fewer to make for every halving. Images of the user interface keep their size, so that their text stays sharp. See [user-interface.md](user-interface.md#what-a-camera-sees) | 1 |
| `rendering.target_mipmaps` | The most levels of smaller copies a render target has, which are made again in every frame it is drawn into: 0 for as many as its size allows, 1 for none, up to 16. Lowers what a camera asks for with `mipmaps`, never raises it | 0 |
| `rendering.max_fps` | The most frames a second: a whole number from 30 to 300, or 0 for as many as can be drawn. It holds with `vsync` on and off, whichever of the two allows fewer. See [vulkan-renderer.md](vulkan-renderer.md#vertical-sync-and-the-window) | 0 |
| `rendering.exposure` | How bright the scene is taken to be: the light of the scene is multiplied by it before the curve. A number above zero; 2 doubles the light, 0.5 halves it | 1 |
| `audio.groups` | The [groups of sounds](audio.md#groups) the project has besides those of the engine: a list of names, or of maps with a `name` and the `volume` the group starts at. A name that is there already is an error. A group of the project is held still while the game is paused, as the effects are | None. The groups of the engine are `music`, `effects`, `voices`, and `ambience` |
| `audio.volumes` | The volume of a group by its name, 0 for silence and 1 for the loudness of its sounds, not below 0. A name that is not a group of the engine or of the project, declared above or in a file read before, is an error. The runtime's settings menu shows them and writes the ones the player changed to `user://settings.yml` with Apply, see [user-interface.md](user-interface.md#the-volumes-of-the-settings-menu) | 1 for every group |
| `game` | The [settings of the game](#settings-of-the-game), each set by its name, which the project declares | The default each is declared with |

### Quality presets

A preset sets every graphics setting that costs frame time at once, so that
a player picks one word and a game ships with one (#356). The settings
that are a matter of taste rather than cost, the tonemapper and the
exposure, vertical sync, the frame limit, and the window, are not part of
one. The table is `GraphicsPresets` in neon-core, with a unit test over it,
and the engine brings four presets:

| Preset | `anisotropy` | `texture_scale` | `target_scale` | `target_mipmaps` | `shadow_map_size` | `shadow_filter` | `shadow_cascades` | `shadow_distance` |
|---|---|---|---|---|---|---|---|---|
| `low` | 2 | 0.5 | 0.5 | 1 | 1024 | `none` | 1 | 40 |
| `medium` | 4 | 1 | 1 | 4 | 2048 | `pcf` | 2 | 80 |
| `high` | 8 | 1 | 1 | 0 | 2048 | `pcf` | 4 | 120 |
| `ultra` | 16 | 1 | 1 | 0 | 4096 | `pcf` | 4 | 160 |

`rendering.shadows`, whether shadows are drawn at all, is not part of a
preset: off changes what is seen rather than how finely, so it is the
player's own choice, as vertical sync is, and a preset leaves it alone.

The default is `high`, since it is what the defaults of the engine have
been: every setting at the full quality of the renderer apart from the
anisotropy, which has been 8 with 16 a step above it. `ultra` turns that up
and reaches the shadows farther; `low` halves the textures and the camera
pictures, and keeps one cascade close by. `custom` names no preset and
changes nothing: it is what the settings menu shows when the values match
none, and what it writes then.

The same preset is `--quality` on the command line, applied before the
other options of its set, and the Quality row of the settings menu, which
sets the rows below it as if the player had chosen each, see
[user-interface.md](user-interface.md#the-graphics-of-the-settings-menu).

#### What a preset holds over

A preset is used as it is, or not at all: while one holds, the eight
settings it decides are the preset's, and a file or a command line that
writes one of them next to a preset, or in a layer after it, has that name
left out with a warning in the log, `'anisotropy' of 'rendering' of the
settings is set by the preset low, and is left out unless 'quality' is
custom`. The file still has to be right: a wrong value is an error as it
is without a preset. The settings no preset decides, `vsync`, `max_fps`,
`shadows`, `tonemapper`, and `exposure`, apply as always.

| Layer writes | Then |
|---|---|
| `quality: low` | The values of `low` hold. `anisotropy: 16` next to it is left out, with a warning |
| `quality: custom` | No preset holds. `anisotropy: 16` next to it applies, and so does a value in a layer after it that names no preset |
| Nothing of `quality` | Whatever held before holds: a preset a layer before chose, or none from the start, which is the default, so that a file that says nothing of a preset sets what it writes |
| `quality: low` in the project's file, `anisotropy: 16` in the player's | Left out with a warning: the project's preset holds until the player's file writes `quality: custom` |
| `quality: low` in the project's file, `quality: custom` and `anisotropy: 16` in the player's | 16, and the rest of `low` stays as the project set it |
| `--quality low --anisotropy 16` | 2, and the log says that `--anisotropy` is set by the preset |
| `--quality custom --anisotropy 16` | 16 |

The settings menu writes what reads back the same: a preset chosen in the
menu is written alone, and the values it decides are taken out of the
player's file, since they would be left out next to it; a row changed by
hand is written with `quality: custom`, so that it applies over a preset
the project chose, see
[user-interface.md](user-interface.md#the-graphics-of-the-settings-menu).

What the presets are is the project's to say, in its `project.yml` under
`graphics_presets`, see [projects.md](projects.md#quality-presets): a
preset of the engine changed value by value, a new one after them with the
engine's defaults for what it leaves out, and one turned `off` so that the
menu does not offer it. It is written there and not here because the table
is part of what the project is, not a setting a layer changes: the
settings, the project's and the player's alike, only choose from it with
`rendering.quality`. A `presets` map under `rendering` is an error that
says so.

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

## Settings of the game

A game has settings of its own, which it declares in its `project.yml`
under `settings`: what each holds, its default, its range or its choices,
and where and how the settings menu shows it, see
[projects.md](projects.md#settings-of-the-game). The settings files set
them, by name, under `game`, the project's file and the player's alike,
each on top of the one before:

```yaml
game:
  look_sensitivity: 2.5
  invert_look: true
```

A value that does not fit its kind, its range, or its choices, a name the
project does not declare, and a value written for an action, which holds
nothing, are mistakes like any other in the file: in the project's file
they stop the runtime, in the player's file they are said in the log and
the file is left out until it is corrected. A declaration written here, a
map with a `kind` and a `default`, is a mistake that points at
`project.yml`.

What holds while the game runs is in the `SettingsStore`: `Get`, `Set`,
which refuses what does not fit and says so once in the log, `Trigger` for an
action, and `OnChange(name, callback)`, whose subscription ends when its
holder goes. A change tells every subscriber of the name after the value is
stored; a value set to what it is already tells nobody. The settings menu
sets the store as the player changes a row, and keeps what the player
chose with Apply, as `game` of `user://settings.yml`; a script reads and
hears of it through the `settings` library, see
[scripting.md](scripting.md#what-a-script-reaches-of-the-engine). How the
menu shows the sections is in
[user-interface.md](user-interface.md#the-settings-of-the-game-in-the-menu).

The engine's own settings, `rendering.*`, `window.*`, and the volumes, are
not in the store yet: they join it later, and the graphics and the audio of
the menu then become rows like any other (#542).

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
| `GraphicsPreset`, `GraphicsPresets` | neon-core, `neon/render/graphics-preset.hpp`, `graphics-presets.hpp` | One quality preset, and the table of them: the engine's four and what the project does to them, which one a set of values is, and the names a setting takes |
| `GraphicsPresetReader` | neon-core, `neon/render/graphics-preset-reader.hpp` | Reads the values a preset decides, checked as the settings of those names are; `rendering` of a settings file and a preset of `project.yml` are read with it |
| `GraphicsMenu` | neon-core, `neon/runtime/graphics-menu.hpp` | The graphics of the runtime's settings menu: shows them, changes them, and keeps or puts them back |
| `AudioMenu` | neon-core, `neon/runtime/audio-menu.hpp` | The volumes of the runtime's settings menu, one for every group: shows them, changes them, and keeps them as `audio.volumes` or puts them back |
| `SettingsStore`, `SettingsSubscription` | neon-core, `neon/settings/settings-store.hpp` | The settings of the game while it runs: what each holds, `Set` with its checks, and the change callbacks. `main.cpp` declares it from what `ProjectFile` read and the layers set, and hands it to the runtime and the scripts |
| `GameMenu` | neon-core, `neon/runtime/game-menu.hpp` | The settings of the game in the settings menu: builds a section per category, binds every value named after a setting, sets the store at once, keeps with Apply, puts back with Back |
| `settings` in Lua | neon-lua, `neon/scripting/lua-libraries.cpp` | `settings.get`, `settings.set`, `settings.trigger`, and `settings.on_change` over the store |
| `RuntimeOptions`, `DisplayOptions` | neon-core, `neon/command-line/` | The command line, layer 4 |
| `main.cpp` | NeonRuntime | Reads the layers in order |

The tests of the reader are in `tests/settings-files`, with the file of the
sandbox read as it is in the repository; those of the declarations in
`tests/project-files`, and those of the store, the menu, and the library
next to each. `tests/runtime-scripts` runs a script that reads a setting
and hears of a change.

## Open questions

- **The renderer as a setting**: `--window-size`, `--window-mode`,
  `--ui-scale`, and `--vulkan-version` have a place here now, and the
  runtime owns the options too (#143, see [command-line.md](command-line.md)).
  `--renderer` has no setting yet, since there is one renderer.
- **Per-platform settings**: a project may want another size or mode on
  another platform. Not needed yet.
