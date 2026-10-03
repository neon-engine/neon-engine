# Command line

Every option of NeonRuntime, in one place, with who it is for. How to use
them is in the [development guide](development.md#command-line). This page is
the list to go through when a shipped runtime gets only the options a player
needs (#143).

Today one executable serves the editor, development, automated checks, and a
shipped game alike, so every option below is open to whoever starts it.

## The options

A switch is written as `--name`. An option with a value is written as
`--name value` or `--name=value`.

The last column is a proposal, to be decided in #143:

- **Editor only:** left out of a shipped runtime when it is built.
- **Keep:** a player may use it.
- **Decide:** open, often because a setting (#125) or a menu may serve
  better.

| Option | Group | What it does | Shipped game |
|---|---|---|---|
| `--help` | General | Shows the options | Keep, listing only what is kept |
| `--scene PATH` | General | Starts with this scene in place of the entry scene of the project | **Editor only.** It loads any content at all, the heart of #143 |
| `--ui PATH` | General | Shows this user interface on top | **Editor only**, for the same reason |
| `--renderer vulkan` | General | The renderer to draw with | Decide. With Metal, WebGPU, or DirectX 12 (#65, #66, #109), a player may need to fall back to another. Perhaps a setting ([settings.md](settings.md)) |
| `--frames N` | Development | Stops after N frames | Editor only |
| `--screenshot PATH` | Development | Saves the last frame as a PNG image | Editor only |
| `--screenshot-at N[,N...]` | Development | Saves the listed frames | Editor only |
| `--output-dir DIR` | Development | The folder that `output://` stands for. The only option that takes a native path | Editor only. It lets a run write where it is told |
| `--time-step SECONDS` | Development | Advances every frame by the same time | Editor only. It changes how the game runs |
| `--headless-renderer` | Development | Renders without a window: frames are drawn off-screen at the configured size, input comes from a script, the sound is mixed and discarded | Editor only. For screenshots and checks on a machine with no display |
| `--headless` | Development | A dedicated server (#144): no display, no renderer, no sound, no input devices. Not available yet, the option is refused and points at `--headless-renderer` | Decide with #144, which may make the server a third kind of runtime |
| `--input SCRIPT` | Development | Input from a script in place of devices | Editor only. It plays the game without a player |
| `--input-script PATH` | Development | The same from a file | Editor only |
| `--spawn PATH` | Development | Spawns this prefab at the top of the world once the scene is read, as a script would, see [prefabs.md](prefabs.md#spawning-at-run-time) | Editor only. It is for checking a prefab on its own |
| `--jit on\|off` | Development | Compiles the scripts as they run, or runs them in LuaJIT's interpreter, over `scripting.jit` of the settings, see [scripting.md](scripting.md#which-lua) | Decide. A setting serves a game; the option compares the two |
| `--window-size WxH` | Display | The size of the window in points, over `window` of the settings | Decide. Usually a menu's choice, but useful for displays a game does not expect |
| `--render-scale NUMBER` | Display | Pixels for each point, without a window | Editor only. It needs `--headless-renderer` |
| `--ui-scale NUMBER` | Display | Makes the user interface larger or smaller, over `ui.scale` of the settings | Decide. It helps players who need larger text, which a settings menu may offer instead |
| `--vulkan-version 1.N` | Display, rendering | The version of Vulkan to ask for, over `rendering.vulkan_version` of the settings (#142) | Decide. A way around a driver that does not work at the highest version |

The command line is the last of the layers of settings, see
[settings.md](settings.md): it is read on top of `assets://settings.yml` and
`user://settings.yml`, and wins over both.

### Headless renderer and headless runtime

The two headless options are different things, decided in #180. A **headless
renderer** (`--headless-renderer`) is the runtime as it is, with the window
taken away: the renderer draws off-screen, a screenshot can be saved, and the
input comes from a script. It is how the renderer is checked on a build
server. A **headless runtime** (`--headless`) has no display, no renderer, no
sound, and no input devices: the world steps, the physics runs, and the
network serves. That is the dedicated server of #144, which does not exist
yet, so `--headless` is refused until it does.

## What is not an option yet

Some of what a runtime does is fixed today, and may become options or
settings later:

| What | Where it is decided |
|---|---|
| The organization and application names that place `user://` | `project.yml`, see [projects.md](projects.md). Never something a player may change |
| The log file, `user://logs/neon-engine.log` | Compiled in. See [file-systems.md](file-systems.md#the-log-file) |
| The present mode, which is what vsync chooses | The renderer chooses by itself, mailbox first. `RenderCapabilities` (#142) will say what a menu can offer |
| The debug mode with a terminal (#116) | Not there yet. It is for the editor only |

## Open questions

- Which of the **Decide** options a shipped game keeps, and which become
  settings in `settings.yml` (#125) or choices in a menu.
- Whether a dedicated server (#144) is a third build, with its own set of
  options.
- How the editor starts NeonRuntime when a game is run from it (#82), and
  whether it passes options or a project.
