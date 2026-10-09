# Command line

Every option of NeonRuntime, in one place, with who it is for. How to use
them is in the [development guide](development.md#command-line).

Every set of options is **owned** by one application, decided in #143. The
runtime owns the options a player needs; the editor owns the options that
load other content or look inside a game. Each set is an object of its own,
registered by the application that owns it.

| Set | Owner | Holds |
|---|---|---|
| `RuntimeOptions` | The runtime | `--renderer`, `--vulkan-version` |
| `DisplayOptions` | The runtime | `--window-size`, `--window-mode`, `--vsync`, `--max-fps`, `--anisotropy`, `--texture-scale`, `--target-scale`, `--target-mipmaps`, `--shadows`, `--shadow-map-size`, `--shadow-filter`, `--shadow-distance`, `--shadow-cascades`, `--ui-scale` |
| `EditorOptions` | The editor | `--scene`, `--ui`, and everything under the `Editor` heading below |

Until NeonEditor exists, NeonRuntime registers the editor's set too, so that
a scene can be started and a frame rendered without a window on a build
server. The editor will then register the runtime's sets and its own, and
the runtime only its own; a shipped game is then run by a runtime that has
no `--scene` at all. The aim is that a player cannot do what the game does
not want with an option, not to stop someone who is determined.

## The options

A switch is written as `--name`. An option with a value is written as
`--name value` or `--name=value`.

The last column is the decision of #143: who owns the option.

- **Runtime:** a player may use it. It stays when the editor takes its set.
- **Editor:** it loads other content or looks inside a game. Registered by
  NeonRuntime only until NeonEditor exists.

| Option | Group | What it does | Owned by |
|---|---|---|---|
| `--help` | General | Shows the options that are registered | Runtime |
| `--renderer vulkan` | General | The renderer to draw with | **Runtime.** With Metal, WebGPU, or DirectX 12 (#65, #66, #109), a player may need to fall back to another. Today it accepts `vulkan` alone |
| `--vulkan-version 1.N` | General | The version of Vulkan to ask for, over `rendering.vulkan_version` of the settings (#142) | **Runtime.** A way around a driver that does not work at the highest version, which a player needs before any menu is shown |
| `--window-size WxH` | Display | The size of the window in points, over `window` of the settings. Shows a window of that size unless `--window-mode` says otherwise | **Runtime.** Usually a menu's choice, but a game that comes up on a display it does not expect is reached this way |
| `--window-mode MODE` | Display | `windowed`, `borderless`, or `fullscreen`, over `window.mode` of the settings | **Runtime**, for the same reason. The mode wins over the window that `--window-size` shows |
| `--vsync on\|off` | Display | Whether a frame waits for the screen before it is shown, over `rendering.vsync` of the settings | **Runtime.** A menu's choice as well, and the way to tell whether a problem is the sync |
| `--anisotropy NUMBER` | Display | Samples a texture is read with where it is seen from the side: 1 for none, 2, 4, 8, or 16, over `rendering.anisotropy` of the settings | **Runtime.** A menu's choice as well, and a way to see what anisotropic filtering costs |
| `--texture-scale NUMBER` | Display | The size textures read from files are kept at: 1, 0.5, 0.25, or 0.125 of their size, over `rendering.texture_scale` of the settings | **Runtime.** A menu's choice as well, and a way to see how a game looks on a computer with little memory for textures |
| `--target-scale NUMBER` | Display | The size what a camera draws into is made at: 1, 0.5, or 0.25 of what it asks for, over `rendering.target_scale` of the settings | **Runtime.** A menu's choice as well |
| `--target-mipmaps NUMBER` | Display | The most levels of smaller copies a render target has: 0 for as many as its size allows, 1 for none, up to 16, over `rendering.target_mipmaps` of the settings | **Runtime.** A menu's choice as well, and a way to see what the smaller copies of a camera's picture cost (#431) |
| `--max-fps NUMBER` | Display | The most frames a second, from 30 to 300, or 0 for as many as can be drawn, over `rendering.max_fps` of the settings | **Runtime.** A menu's choice as well, and a way to keep a laptop cool or a measurement steady |
| `--shadows on\|off` | Display | Whether the direction light casts shadows at all, over `rendering.shadows` of the settings; `off` leaves the shadow pass out of the frame, see [vulkan-renderer.md](vulkan-renderer.md#shadows) | **Runtime.** A menu's choice as well, and the way to tell whether a problem is the shadows |
| `--shadow-map-size NUMBER` | Display | Texels the shadow map has along each side: 512, 1024, 2048, or 4096, over `rendering.shadow_map_size` of the settings | **Runtime.** A menu's choice as well, and a way to see what the map costs |
| `--shadow-filter NAME` | Display | How the shadow map is compared against: `none` for one comparison, `pcf` for nine averaged, over `rendering.shadow_filter` of the settings | **Runtime.** A menu's choice as well |
| `--shadow-distance METERS` | Display | How far from the camera the shadows reach, above 0, over `rendering.shadow_distance` of the settings | **Runtime.** A menu's choice as well |
| `--shadow-cascades NUMBER` | Display | How many cascades the shadow map has, 1 to 4, over `rendering.shadow_cascades` of the settings | **Runtime.** A menu's choice as well |
| `--ui-scale NUMBER` | Display | Makes the user interface larger or smaller, over `ui.scale` of the settings | **Runtime.** It helps players who need larger text before a settings menu can be read |
| `--scene PATH` | Editor | Starts with this scene in place of the entry scene of the project | **Editor.** It loads any content at all, the heart of #143 |
| `--ui PATH` | Editor | Shows this user interface on top | **Editor**, for the same reason |
| `--frames N` | Editor | Stops after N frames | Editor |
| `--screenshot PATH` | Editor | Saves the last frame as a PNG image | Editor |
| `--screenshot-at N[,N...]` | Editor | Saves the listed frames | Editor |
| `--output-dir DIR` | Editor | The folder that `output://` stands for. The only option that takes a native path | **Editor.** It lets a run write where it is told |
| `--time-step SECONDS` | Editor | Advances every frame by the same time | **Editor.** It changes how the game runs |
| `--headless-renderer` | Editor | Renders without a window: frames are drawn off-screen at the configured size, input comes from a script, the sound is mixed and discarded | **Editor.** For screenshots and checks on a machine with no display. A game is checked with `--check` |
| `--render-scale NUMBER` | Editor | Pixels for each point, without a window | **Editor.** It needs `--headless-renderer` |
| `--input SCRIPT` | Editor | Input from a script in place of devices | **Editor.** It plays the game without a player |
| `--input-script PATH` | Editor | The same from a file | Editor |
| `--spawn PATH` | Editor | Spawns this prefab at the top of the world once the scene is read, as a script would, see [prefabs.md](prefabs.md#spawning-at-run-time) | **Editor.** It is for checking a prefab on its own |
| `--log-entity NAME[,...]` | Editor | Logs where the named entities are in every frame, each by its path as a scene names it, such as `player` or `crates/upper`, with where the physics has its body when it is one, see [development.md](development.md#where-an-entity-is) | **Editor.** It looks inside a game, for checks that read the log in place of the pixels |
| `--jit on\|off` | Editor | Compiles the scripts as they run, or runs them in LuaJIT's interpreter, over `scripting.jit` of the settings, see [scripting.md](scripting.md) | **Editor.** A setting serves a game; the option compares the two |
| `--tonemapper NAME` | Editor | The curve for light brighter than white, `none`, `aces`, or `agx`, over `rendering.tonemapper` of the settings, see [vulkan-renderer.md](vulkan-renderer.md#tonemapping) | **Editor**. For comparing the curves on one scene; a game chooses in its settings |
| `--exposure NUMBER` | Editor | How bright the scene is taken to be, over `rendering.exposure` of the settings | **Editor**, for the same reason |
| `--headless` | Editor | A dedicated server (#144): no display, no renderer, no sound, no input devices. Not available yet, the option is refused and points at `--headless-renderer` | **Editor** for now. A server is a build of its own when #144 makes it, not an option of the runtime a player gets |

The command line is the last of the layers of settings, see
[settings.md](settings.md): it is read on top of `assets://settings.yml` and
`user://settings.yml`, and wins over both. Everything the runtime owns has a
place in the settings too, so that a game chooses its defaults and a player's
settings menu changes them without the command line.

### Headless renderer and headless runtime

The two headless options are different things, decided in #180. A **headless
renderer** (`--headless-renderer`) is the runtime as it is, with the window
taken away: the renderer draws off-screen, a screenshot can be saved, and the
input comes from a script. It is how the renderer is checked on a build
server. A **headless runtime** (`--headless`) has no display, no renderer, no
sound, and no input devices: the world steps, the physics runs, and the
network serves. That is the dedicated server of #144, which does not exist
yet, so `--headless` is refused until it does.

## What is not an option

Some of what a runtime does is fixed, or decided elsewhere:

| What | Where it is decided |
|---|---|
| The organization and application names that place `user://` | `project.yml`, see [projects.md](projects.md). Never something a player may change |
| The log file, `user://logs/neon-engine.log` | Compiled in. See [file-systems.md](file-systems.md#the-log-file) |
| The debug mode with a terminal (#116) | Not there yet. It is for the editor only |

## Open questions

- Whether a dedicated server (#144) is a third application with a set of
  options of its own.
- How the editor starts NeonRuntime when a game is run from it (#82), and
  whether it passes options or a project.
- How a runtime is bound to the game it ships with, once the editor exports
  one (#84): a key written into the runtime and checked against the packed
  assets (#81).
