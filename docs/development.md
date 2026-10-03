# Development

## Toolchain

Every platform builds with the same compiler and C++ runtime so behavior is
identical across targets:

| Component | Version |
|---|---|
| Compiler | clang 20 (LLVM 20.1.x) |
| C++ standard library | libc++ |
| Language standard | C++20 |
| Build system | CMake 3.25+ with Ninja |

Build configurations live in [CMakePresets.json](../CMakePresets.json). Each
preset writes into `build/<preset-name>` and the resulting executable lands in
`bin/<build-type>/<os>-<arch>/<app-name>/`. Every target has a debug preset
and a release one, `macos-arm64-debug` and `macos-arm64-release` and so on.

### Release builds

A release preset is what a number is measured with and what a game ships
as. The debug presets compile with `-O0` and nothing inlined, for the
debugger; the release ones with:

| Flag | Why |
|---|---|
| `-O3 -DNDEBUG` | The optimiser at its highest, and the assertions of the libraries off |
| Link-time optimisation, `CMAKE_INTERPROCEDURAL_OPTIMIZATION` | The engine is a dozen static libraries in one executable; optimised as one program, a call into another library is inlined as one within a file is. Thin LTO with clang, so the link stays parallel |
| `-fno-math-errno` | `sqrt` and its kin compile to the instruction of the processor in place of a call that sets `errno`, which nothing reads |
| `-ffunction-sections -fdata-sections` with `-Wl,-dead_strip` on macOS and `-Wl,--gc-sections` elsewhere | What nothing calls is left out of the executable: most of assimp's importers, for one |
| `-mcpu=apple-m1` on macOS | Every Mac with Apple silicon, and the instructions all of them have |
| `-march=x86-64-v2` on Linux and Windows | SSE4.2 and POPCNT, which is what Jolt is built for already (`cmake/JoltPhysics.cmake`) and what every processor since 2009 has. AVX2 would be `x86-64-v3` and would shut out processors that Jolt's own build still runs on |

What is left out on purpose: `-ffast-math`, since it lets the compiler
reorder arithmetic, and the physics is built to give the same result on
every platform (`CROSS_PLATFORM_DETERMINISTIC`), which it then would not;
and `-march=native`, since a build has to run on machines other than the
one that built it.

```sh
cmake --preset macos-arm64-release
cmake --build --preset macos-arm64-release
./bin/release/darwin-arm64/NeonRuntime/NeonRuntime
```

| Option | What it does | Default |
|---|---|---|
| `NEON_BUILD_TESTS` | Makes the tests available to build | `ON` |

### Supported targets

| Target | How it is built |
|---|---|
| macOS arm64 | Natively, with Homebrew LLVM |
| Linux x64 | In the Linux Docker image |
| Windows x64 | In the Windows Docker image, cross-compiled with llvm-mingw |

A graphics card with a Vulkan 1.1 driver is needed to run the result.

## First-time setup

### Git LFS

Model files under `app/NeonRuntime/assets/models/` are stored with
[Git LFS](https://git-lfs.com). The tracking rule lives in
[.gitattributes](../.gitattributes). Install the LFS hooks once per machine
before cloning or pulling, otherwise you get small pointer files instead of the
real models and the demo scene logs errors for every missing `.obj`:

```bash
brew install git-lfs   # macOS; use your package manager elsewhere
git lfs install
```

On an existing checkout that was cloned before LFS was set up, fetch the real
files with:

```bash
git lfs pull
```

Any new file dropped into the models folder is picked up by LFS automatically.
`git lfs ls-files` lists what is currently tracked.

### Submodules

The engine's dependencies are git submodules. Fetch the ones the build uses:

```bash
git submodule update --init --recursive \
  external/glm external/sdl2 external/assimp external/jolt-physics \
  external/spdlog external/rapidyaml external/stb external/googletest \
  external/freetype external/harfbuzz external/lunasvg external/luajit
```

LunaSVG brings PlutoVG as a folder of its own, not as a submodule.

`external/googletest` is only needed for the [tests](#tests).

## macOS (arm64)

Install the toolchain with Homebrew:

```bash
brew install llvm@20 cmake ninja glslang molten-vk vulkan-loader
```

| Package | Needed for |
|---|---|
| `llvm@20`, `cmake`, `ninja` | Building |
| `glslang` | Building. It compiles the shaders of the Vulkan renderer |
| `molten-vk`, `vulkan-loader` | Running. macOS has no Vulkan driver of its own, MoltenVK provides one on top of Metal |

Homebrew's LLVM is keg-only and never shadows Apple's clang. The macOS preset
points at it by absolute path, so nothing needs to be added to `PATH`.

Configure and build:

```bash
cmake --preset macos-arm64-debug
cmake --build --preset macos-arm64-debug
```

Run:

```bash
./bin/debug/darwin-arm64/NeonRuntime/NeonRuntime
```

Every platform has a release preset beside its debug one,
`macos-arm64-release`, `linux-x64-release`, and `windows-x64-release`, which
builds into `build/<preset>` and `bin/release/<os>-<arch>/`:

```bash
cmake --preset macos-arm64-release
cmake --build --preset macos-arm64-release
```


## Linux (x64)

Linux builds run inside a Docker image so the toolchain is reproducible. The
image is defined in [docker/linux-x64.dockerfile](../docker/linux-x64.dockerfile)
and installs LLVM 20 from apt.llvm.org plus the X11, Wayland, graphics, and audio
development packages that SDL2 needs to build from source.

Build the image once, from the repository root:

```bash
docker build --platform linux/amd64 \
  -t neon-engine/linux-x64 \
  -f docker/linux-x64.dockerfile docker
```

Configure and build with the repository mounted into the container:

```bash
docker run --rm --platform linux/amd64 -v "$PWD":/src -w /src neon-engine/linux-x64 \
  sh -c 'cmake --preset linux-x64-debug && cmake --build --preset linux-x64-debug'
```

The executable is written to `bin/debug/linux-x86_64/NeonRuntime/NeonRuntime`
on the host. It links libc++ statically, so it runs on any x64 Linux with a
glibc at least as new as Ubuntu 24.04's.

On an Apple Silicon Mac the `--platform linux/amd64` flag runs the image under
emulation. That works but is slow. Drop the flag to build a native arm64 image
and binary instead, which is handy for quick compile checks. The output then
lands under `bin/debug/linux-aarch64/` instead.

To build natively on a Linux host without Docker, install the same packages the
dockerfile lists and run the two preset commands directly.

## Windows (x64)

Windows containers only run on Windows hosts, so the Windows image is a Linux
image that cross-compiles. It is defined in
[docker/windows-x64.dockerfile](../docker/windows-x64.dockerfile) and installs
[llvm-mingw](https://github.com/mstorsjo/llvm-mingw), which bundles clang 20,
libc++, and the mingw-w64 UCRT headers and libraries. The CMake side of the
cross-compile lives in
[cmake/toolchains/windows-x64-llvm-mingw.cmake](../cmake/toolchains/windows-x64-llvm-mingw.cmake).

Build the image once, from the repository root:

```bash
docker build --platform linux/amd64 \
  -t neon-engine/windows-x64 \
  -f docker/windows-x64.dockerfile docker
```

Configure and build:

```bash
docker run --rm --platform linux/amd64 -v "$PWD":/src -w /src neon-engine/windows-x64 \
  sh -c 'cmake --preset windows-x64-debug && cmake --build --preset windows-x64-debug'
```

The executable is written to `bin/debug/windows-x86_64/NeonRuntime/NeonRuntime.exe`
on the host. The runtime is linked statically, so no llvm-mingw DLLs need to
ship alongside it. Copy the `NeonRuntime` folder to a Windows machine to run it.

As with Linux, dropping `--platform linux/amd64` on an Apple Silicon Mac uses a
native arm64 image. The output is still a Windows x64 executable either way.

To build natively on Windows, install llvm-mingw for Windows, put its `bin`
directory on `PATH`, and run the same two preset commands from a shell. The
toolchain file reads `LLVM_MINGW_ROOT` from the environment if llvm-mingw is not
installed at `/opt/llvm-mingw`.

## Renderer

The engine renders with Vulkan, through the neon-vulkan library. On macOS that
runs on top of Metal through MoltenVK. A Metal renderer of its own is planned.
An OpenGL renderer existed and was removed, see
[vulkan-renderer.md](vulkan-renderer.md) for the reasons and the design.

The rest of the engine reaches the renderer through the interfaces in
neon-core and sees no Vulkan type, so another renderer can be added next to it.

### Shaders

| | |
|---|---|
| Sources | `app/<app>/shaders/vulkan/*.vert` and `*.frag`, written in GLSL 450 |
| Shared declarations | `scene-data.glsl` in the same folder, pulled in with `#include` |
| Compiled by | The build, with glslang |
| Compiled to | `assets/shaders/<name>.vert.spv` and `<name>.frag.spv` next to the binary |

A material names its shader without an extension, such as
`assets://shaders/basic-lit`, and the renderer adds `.vert.spv` and
`.frag.spv`. Scene recipes therefore never mention a shader format.

| Shader | Draws |
|---|---|
| `basic-lit` | Textured or plain colored surfaces lit by one direction light and up to 64 point and 64 spot lights |
| `unlit` | The texture as it is, without lighting |
| `color` | The plain color of the material, without lighting |
| `flat` | Triangles in pixels, blended by their alpha, for the [user interface](user-interface.md) |

To add one, put its `.vert` and `.frag` files in the sources folder and run
the configure step again.

## Command line

Every option, with the application that owns it, is listed in
[command-line.md](command-line.md). The first two groups are the runtime's
own; the `Editor` group is the editor's set, which NeonRuntime registers
only until NeonEditor exists.

```
Usage: NeonRuntime [options]

  --help                    Show this text
  --renderer vulkan         Renderer to draw with. Default: vulkan
  --vulkan-version 1.N      Highest version of Vulkan to render with, for example 1.2. Default: 1.3

Display:
  --window-size WxH         Size of the window in points, for example 1280x720. Shows a window of that size in place of one that covers the display, unless --window-mode says otherwise
  --window-mode MODE        How the window is shown: windowed, borderless, or fullscreen, over window.mode of the settings
  --ui-scale NUMBER         Makes the user interface larger or smaller, for example 1.5

Editor:
  --scene PATH              Scene to start with in place of the entry scene of the project, for example assets://scenes/demo.scene.yml
  --ui PATH                 User interface to show on top, for example assets://ui/hud.ui.yml
  --frames N                Stop after N frames
  --screenshot PATH         Save the last frame as a PNG image, for example output://frame.png. Needs --frames or --screenshot-at
  --screenshot-at N[,N...]  Save these frames instead of the last one, counted from 1. Each file gets its frame in its name, as in frame-0030.png. Needs --screenshot
  --output-dir DIR          Folder of this machine that output:// stands for. Created when missing
  --time-step SECONDS       Advance the game by this much time in every frame, for example 0.016667, so that a run gives the same frames every time
  --headless-renderer       Render without a window, for screenshots and checks on a machine with no display
  --render-scale NUMBER     Pixels that are drawn for each point, for example 2 for what a display of high density shows. Needs --headless-renderer, a window takes the density of its display
  --input SCRIPT            Input in place of devices, for example "1: pointer 640 360; 2: click". Needs --headless-renderer
  --input-script PATH       The same from a file, for example assets://input/menu.input. Needs --headless-renderer
  --spawn PATH              Spawn this prefab at the top of the world once the scene is read, as a script would, for example assets://prefabs/target.prefab.yml
  --tonemapper NAME         Curve for light brighter than white: none, aces, or agx, over rendering.tonemapper of the settings
  --exposure NUMBER         How bright the scene is taken to be, for example 2 for twice the light, over rendering.exposure of the settings
  --jit on|off              Compile the scripts as they run, or run them in LuaJIT's interpreter. Over scripting.jit of the settings, for comparing the two
  --headless                Run as a dedicated server. Not available yet, see --headless-renderer
```

A switch is written as `--name`. An option with a value is written as
`--name value` or `--name=value`.

Together the development options render a scene without a window and save the
result, which is how the renderer is checked on a machine with no display:

```bash
NeonRuntime --headless-renderer --frames 3 --output-dir /some/where --screenshot output://frame.png
```

This writes `/some/where/frame.png`.

### Running without a window

| Option | What it does |
|---|---|
| `--headless-renderer` | Creates no window and reads no input devices. Frames are rendered off-screen at the configured size, and the sound is mixed and discarded. It is a headless renderer, not a headless runtime: `--headless` is kept for the dedicated server of #144 and refused until it exists, see [command-line.md](command-line.md#headless-renderer-and-headless-runtime) |
| `--frames N` | Stops after N frames |
| `--output-dir DIR` | Says which folder `output://` is. `DIR` is a path of the operating system, absolute or relative to the working directory. The folder is created when it is missing |
| `--screenshot PATH` | Saves the last frame as a PNG image at a virtual path, under the exact name given |
| `--screenshot-at N[,N...]` | Saves the listed frames instead of the last one. Frames are counted from 1 |
| `--time-step SECONDS` | Every frame advances the game by this much time, whatever time the frame took |
| `--input SCRIPT` | Input from a script in place of devices. A step is `frame: command`, with `;` or a line break between steps: `pointer X Y` or `pointer none`, `down`, `up`, `click`, `key NAME [shift] [control] [alt] [shortcut] [word]`, `text WHAT`, `compose WHAT`, `wheel X Y [precise]`, `hold ACTION [N]` for an action of the user interface such as `ui-accept` or a button of the input map such as `jump`, `hold-key KEY [N]` for a key by where it is such as `w`, `hold-button NAME [N]` for a button of a controller such as `south`, `stick X Y [N]` for the right stick and `left-stick X Y [N]` for the left one, `device keyboard` or `device gamepad`, `look X Y`. A step of a frame that was skipped is applied in the next. A hold of an action the map does not have is refused with the names it has, see [input.md](input.md#scripts) |
| `--input-script PATH` | The same from a file at a virtual path |
| `--spawn PATH` | Spawns the prefab at a virtual path at the top of the world once the scene is read, as a script would. A prefab that cannot be read is said in the log and sets the exit code, and the run goes on without it |
| `--jit on\|off` | Compiles the scripts as they run, or runs them in LuaJIT's interpreter, over `scripting.jit` of the settings |
| `--window-size WxH` | The size in points |
| `--render-scale N` | N pixels for each point, as a display of that density gives. `--window-size 1280x720 --render-scale 2` renders 2560 by 1440 |
| `--ui-scale N` | Makes the user interface larger or smaller, on top of the density |

`--output-dir` is the only place a native path is accepted. Everything else
names files by virtual paths. A screenshot can also go to `user://`, and the
log says where that is on the machine.

With `--screenshot-at`, each file carries the number of its frame in front of
its extension, with at least four digits:

```bash
NeonRuntime --headless-renderer --output-dir shots --screenshot output://frame.png --screenshot-at 1,30,60
```

This writes `shots/frame-0001.png`, `shots/frame-0030.png`, and
`shots/frame-0060.png`, and stops after frame 60.

| Given | How long the run is | What is saved |
|---|---|---|
| `--frames 60 --screenshot P` | 60 frames | Frame 60, as `P` |
| `--screenshot P --screenshot-at 10,30` | 30 frames, the highest frame asked for | Frames 10 and 30, numbered |
| `--frames 60 --screenshot P --screenshot-at 10,30` | 60 frames | Frames 10 and 30, numbered. Frame 60 is not saved |

The order of the list does not matter, and a frame that is listed twice is
saved once.

**Time.** Without a window, a frame advances the game by a sixtieth of a
second unless `--time-step` says otherwise. With a window, it advances by the
time that was measured unless `--time-step` is given. The value is written
with a dot, such as `0.05`. Game code receives it as the delta time of
`WindowContext::GetDeltaTime()` and needs no changes. Random numbers are not
fixed yet. A game that uses them can still differ between runs.

**Errors.** These are reported before anything is rendered, with the help
text, and the exit code is 1:

| Mistake | Message |
|---|---|
| `--screenshot` without `--frames` or `--screenshot-at` | `Option '--screenshot' needs '--frames' or '--screenshot-at'` |
| `--screenshot-at` without `--screenshot` | `Option '--screenshot-at' needs '--screenshot'` |
| A frame in `--screenshot-at` above `--frames` | `Frame 11 of '--screenshot-at' is never reached, '--frames' stops after 10` |
| A frame that is not a whole number above zero | `Option '--screenshot-at' needs whole numbers above zero, separated by commas` |
| A time step that is not a number above zero | `Option '--time-step' needs a number of seconds above zero, such as 0.016667` |
| A version that is not of Vulkan 1, such as `2.0` or `1.3.1` | `Option '--vulkan-version' needs a version of Vulkan 1, such as 1.3` |
| An `output://` screenshot without `--output-dir` | `'output://frame.png' needs '--output-dir', which says where output:// is` |

A version of Vulkan below 1.1 is a version all the same, and the command line
lets it through. The renderer refuses it when it starts, and ends the run
with exit code 1. See [versions of Vulkan](vulkan-renderer.md#versions-of-vulkan).

**Exit code.** A script can rely on it.

| Exit code | Meaning |
|---|---|
| 0 | The run did what it was asked to |
| 1 | The command line was not understood, something was logged as an error — a scene or a user interface with a problem, a screenshot that could not be written — or the run ended with an exception. The run goes on after an error, as a game does; the exit code says it happened, and the log says what |

A folder that cannot be created or written to is only found out when the
file system starts or the frame is written. The run then still renders, logs
the error, and ends with exit code 1.

### How it is built

Each set of options is owned by one application and registered by it, see
[command-line.md](command-line.md). The runtime owns `RuntimeOptions` and
`DisplayOptions`; the editor owns `EditorOptions`. Until NeonEditor exists,
NeonRuntime registers the editor's set too. The editor will then register
the runtime's sets plus its own, and the runtime only its own.

| Piece | Location | Role |
|---|---|---|
| `CommandLine` | [neon-core](../lib/neon-core/neon/command-line/command-line.hpp) | Parses arguments, checks them, and writes the help text. Knows no application |
| `CommandLineContext` | [neon-core](../lib/neon-core/neon/command-line/command-line-context.hpp) | The read side, for code that wants to know what was asked for |
| `CommandLineOptions` | [neon-core](../lib/neon-core/neon/command-line/command-line-options.hpp) | Interface of a set of options: what they are, and what they do to the settings |
| `RuntimeOptions` | [neon-core](../lib/neon-core/neon/command-line/runtime-options.hpp) | Owned by the runtime: `--renderer`, `--vulkan-version` |
| `DisplayOptions` | [neon-core](../lib/neon-core/neon/command-line/display-options.hpp) | Owned by the runtime, the window and the size of what is shown: `--window-size`, `--window-mode`, `--ui-scale` |
| `EditorOptions` | [neon-core](../lib/neon-core/neon/command-line/editor-options.hpp) | Owned by the editor: another scene, no window, screenshots, input from a script |

An application puts them together in `main.cpp`:

```cpp
neon::CommandLine command_line("NeonRuntime", "Runs a Neon Engine project.");
neon::RuntimeOptions runtime_options;
runtime_options.Register(command_line);
neon::DisplayOptions display_options;
display_options.Register(command_line);
// until NeonEditor exists, the runtime registers the editor's set too
neon::EditorOptions editor_options;
editor_options.Register(command_line);

if (!command_line.Parse(argc, argv)) { /* print GetError() and GetHelp() */ }
if (command_line.WantsHelp()) { /* print GetHelp() */ }

SettingsConfig settings_config{ /* defaults of the application */ };
runtime_options.Apply(command_line, settings_config, error);
/* and the other sets */
```

### Adding options

To add an option the runtime owns, declare it in `RuntimeOptions::Register()`
or `DisplayOptions::Register()` and act on it in the `Apply()` of the same
set. An option the editor owns goes in `EditorOptions`, and a row in the
table of [command-line.md](command-line.md) says who owns it.

To give one application options of its own, write a class that implements
`CommandLineOptions` and register it next to the runtime set:

```cpp
class EditorOptions final : public neon::CommandLineOptions
{
public:
  void Register(neon::CommandLine &command_line) override
  {
    command_line.Add({
      .name = "project",
      .value_name = "PATH",
      .description = "Project to open",
      .group = "Editor"
    });
  }

  bool Apply(const neon::CommandLineContext &command_line, SettingsConfig &settings, std::string &error) override;
};
```

| Field of an option | Meaning |
|---|---|
| `name` | Without dashes |
| `value_name` | What the help text calls the value. Leave it out for a switch |
| `description` | One line for the help text |
| `group` | Heading in the help text. Options without one come first |
| `allowed_values` | The values that are accepted. The parser rejects any other |
| `default_value` | The value when the option is not given |

Checks that involve one option belong to the parser: whether it is known,
whether it has a value, whether the value is accepted. Checks that involve
several, such as one option needing another, belong in `Apply()`.

## Window modes

The window mode is chosen through `SettingsConfig::window_mode`, defined in
[settings-config.hpp](../lib/neon-core/neon/runtime/settings-config.hpp).
It defaults to `Windowed`. NeonRuntime's [settings.yml](../app/NeonRuntime/assets/settings.yml)
sets `borderless` as `window.mode`, see [settings.md](settings.md).

| Mode | Behaviour | `width` and `height` |
|---|---|---|
| `WindowMode::Windowed` | A regular window with a title bar and borders | Size of the window |
| `WindowMode::Borderless` | No decorations, covers the whole display at the desktop's resolution. Switching applications stays instant because the display mode never changes | Ignored |
| `WindowMode::Fullscreen` | Exclusive fullscreen | Resolution the display is switched to |

```cpp
const auto settings_config = SettingsConfig{
  .width = 1920,
  .height = 1080,
  .selected_api = RenderingApi::Vulkan,
  .window_mode = WindowMode::Borderless
};
```

Because the window is not always the configured size, code must not use
`width` and `height` from the settings to mean the size on screen.

- Renderers ask the window for its real size through
  `WindowContext::GetDrawableSize()`. The Vulkan render system does this once
  in `Initialize()` to set its viewport and render resolution.
- Anything that needs the render size afterwards, such as the projection
  matrix, reads `RenderContext::GetRenderResolution()`.

The setting lives in neon-core and knows nothing about SDL. Each window system
translates it for its own backend. The SDL2 window system does so in
`SDL2_WindowSystem::Initialize()`.

The size is read once at startup. Resizing the window or changing the mode
while the app runs is not supported yet.

## File system and resource paths

All file access goes through the engine's own file system abstraction. Engine
and application code does not use `std::filesystem`, `std::ifstream`, or a
backend's file calls directly. That keeps the implementation replaceable.

### Virtual paths

Files are named by virtual paths, in the style of Godot. A virtual path is
written the same way on macOS, Linux, and Windows, so anything that refers to
files, such as a scene recipe, can be moved between platforms unchanged. Loading
also never depends on the directory the app was started from.

| Scheme | Points at | Status |
|---|---|---|
| `assets://` | `<directory of executable>/assets` | Read-only |
| `user://` | A folder of the current user, for saves, settings, and anything else the app writes | Read and write |
| `output://` | A folder chosen with `--output-dir` when the app is started, for what a run hands back, such as screenshots | Read and write. Rejected when no folder was chosen |

Where `user://` lives depends on the platform, and on the `organization` and
`name` of the project (`assets://project.yml`, see
[projects.md](projects.md)), so that projects do not share a folder. Until the
project is read, `user://` has no folder and every path in it is refused.

| Platform | `user://` |
|---|---|
| macOS | `~/Library/Application Support/<organization>/<application>/` |
| Linux | `~/.local/share/<organization>/<application>/` |
| Windows | `%APPDATA%\<organization>\<application>\` |

`output://` has no folder of its own. Without `--output-dir`, or when the
folder that was asked for cannot be created, every `output://` path is
rejected with an error, like a path that breaks a rule. The folder is a
native path, which `SettingsConfig` carries as `output_directory`. Only the
file system backend reads it.

`assets://models/sphere.obj` therefore reads
`<directory of executable>/assets/models/sphere.obj`. The build copies
`app/<app-name>/assets` next to the binary, which is the folder the scheme
resolves to.

### Path rules

The rules are enforced on every platform, including those whose own file
system would accept more. A path that works on one machine therefore works on
all of them, and a path that would fail somewhere fails everywhere.

| Rule | Accepted | Rejected |
|---|---|---|
| Starts with a known scheme | `assets://models/cube.obj` | `assets/models/cube.obj`, `/usr/share/x.obj`, `C:\game\x.obj` |
| Forward slashes between folders | `assets://models/cube.obj` | `assets://models\cube.obj` |
| No `..` segments | `assets://models/cube.obj` | `assets://../settings.ini` |
| None of `< > : " \| ? *` or control characters | `assets://models/cube-2.obj` | `assets://models/what?.obj` |
| No name ending in a dot or a space | `assets://models/cube.obj` | `assets://models/name./cube.obj` |
| Names a file | `assets://models/cube.obj` | `assets://` |
| Letter case matches the names on disk | `assets://models/cube.obj` for a file stored as `models/cube.obj` | `assets://Models/Cube.obj` for that same file |

Doubled slashes and `.` segments are harmless and are removed.

A rejected path is logged as an error that says which rule it broke, and is
then treated as a file that does not exist.

### Letter case

Windows and a default macOS disk ignore letter case. Linux does not. Left to
the operating system, `assets://Models/Cube.obj` would load a file stored as
`models/cube.obj` on a Mac and fail on Linux.

The file system does not leave it to the operating system. For every folder
and file name in a path, it lists the parent folder and requires an entry
spelled exactly the same. A path with the wrong case is refused on all three
platforms, and the error names the correct spelling:

```
Invalid path 'assets://Models/Cube.obj': 'Models' is named 'models' on disk,
letter case has to match on every platform
```

Asset names can use any case. The path only has to match the name. Lowercase
names are still the simplest convention to follow.

Things the rules do not check:

- **Reserved names on Windows**, such as `con`, `nul`, and `com1`. Avoid them
  as file or folder names.
- **Two names that differ only in case** in the same folder, such as
  `cube.obj` and `Cube.obj`. Linux allows that. Windows and macOS cannot store
  both. Never rely on it.
- **Accented characters** can be stored in more than one byte form. A path
  that looks identical can then fail to match. Plain ASCII names avoid this.

### Native paths stay hidden

Paths of the operating system never leave a backend. The interface has no
function that returns one, and none that takes one. The folder of
`--output-dir` is the one native path that comes in from outside. It goes
from the command line into `SettingsConfig` and from there into the backend. `FileSystem::Locate` is the single place a native
path is produced, and it is protected, so only backends can call it. It checks
the rules, matches each name against the disk, and joins the folders with the
platform's own separator.

### Writing a backend

A backend derives from `FileSystem` and implements:

| Function | Purpose |
|---|---|
| `Initialize()` | Set `_assets_directory`, `_user_directory`, and `_native_separator`. Set `_output_directory` from `output_directory` in the settings, as an absolute folder that exists, or leave it empty |
| `CleanUp()` | Release anything it holds |
| `Exists(path)`, `ReadBytes(path, contents)` | Call `Locate()` first, then use the native path it returns |
| `WriteBytes(path, contents)` | Call `LocateForWriting()` first, then use the native path it returns |
| `MakeDirectory(native_directory)` | Create one folder whose parent exists |
| `ListDirectory(native_directory, names)` | Report the names in a folder, spelled exactly as stored. `Locate()` relies on it for the letter case check |

The path rules and the letter case check live in the base class, so every
backend enforces them the same way.

### Structure

| Piece | Location | Role |
|---|---|---|
| `FileSystemContext` | [neon-core](../lib/neon-core/neon/filesystem/file-system-context.hpp) | The interface consumers depend on |
| `FileSystem` | [neon-core](../lib/neon-core/neon/filesystem/file-system.hpp) | Base class for backends. Owns the lifecycle and the scheme handling |
| `SDL2_FileSystem` | [neon-sdl2](../lib/neon-sdl2/neon/filesystem/sdl2-file-system.hpp) | The implementation, built on SDL2 |

This is the same split the window system uses. `main.cpp` constructs the SDL2
implementation and injects it as a `FileSystemContext`.

### Interface

| Function | Purpose |
|---|---|
| `Exists(path)` | Whether the file can be opened for reading |
| `ReadBytes(path, contents)` | Reads a whole file as raw bytes |
| `ReadText(path, contents)` | Reads a whole file as text |
| `WriteBytes(path, contents)` | Writes a whole file, replacing it if it exists |
| `WriteText(path, contents)` | Writes a whole file as text |

Every function returns `false` when it fails. Writing creates the folders
that lead to the file. It is refused outside a writable scheme, and it holds
letter case to the same standard as reading: a name that matches an existing
one in everything but case is rejected.

### How the loaders use it

| Loader | How it reads |
|---|---|
| Shaders | `ReadText` for the `.vert` and `.frag` files |
| Textures | `ReadBytes`, then stb_image decodes from memory |
| Models | assimp is given an I/O handler backed by the file system, so the model and any file it refers to, such as an `.obj` material file, come through the engine |

No loader opens a file itself. That is what allows a future backend to serve
files from an archive without the loaders changing.

### Rules for new code

- Write virtual paths, in code and in scene recipes, following the rules above.
- Never build or store a native path outside a backend.
- Take a `FileSystemContext *` through the constructor and read through it.
- Keep the stored path in its scheme form so log messages stay readable.
- Do not include SDL or use `std::filesystem` for file access outside a
  backend library.

### Lifecycle

The file system is created and initialized in `main.cpp` right after logging,
before any system that loads files. It is cleaned up last, after the
application has shut its systems down.

The log file is `user://logs/neon-engine.log`, the `logpath` in
`SettingsConfig`. Logging starts before the file system, so the file is only
opened once the file system has started, with `FileSystem::PlaceLogFile`.
Until then everything goes to the console, and is held back for the file,
which gets it first. Where the log file is on each platform, and why, is in
[file-systems.md](file-systems.md#the-log-file).

The libraries that were considered and the conditions for moving to a full
virtual file system are recorded in [file-systems.md](file-systems.md).

## Tests

The engine is tested with [GoogleTest](https://github.com/google/googletest)
1.18.0, which includes GoogleMock. It is a submodule in `external/googletest`.

```bash
cmake --preset macos-arm64-debug
cmake --build --preset macos-arm64-debug-tests
ctest --preset macos-arm64-debug
```

Tests are never part of the default build, and never part of a library or an
application. `cmake --build --preset macos-arm64-debug` builds NeonRuntime and
nothing else, as before.

### Where tests are stored

The layout follows
[P1204R0, Canonical Project Structure](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1204r0.html).

| Kind | Tests | Stored | Label |
|---|---|---|---|
| Unit test | One source file, with everything around it replaced by mocks and fakes | Next to the file it tests, with `.test` in front of the extension. `command-line.cpp` is tested by `command-line.test.cpp` | `unit` |
| Functional test | Several libraries together, a real backend, or the application itself | In `tests/`, each in a folder of its own | `integration` |

A unit test next to its source is found without searching, and is moved,
renamed, and deleted together with it. A file without a test next to it is
visibly untested.

Each `*.test.cpp` is built into an executable of its own. Started without
arguments it runs all of its tests and returns 0 when they pass. One test can
therefore be built, run, and debugged without the others:

```bash
cmake --build --preset macos-arm64-debug --target neon-core.command-line.test
./build/macos-arm64-debug/tests/neon-core.command-line.test/neon-core.command-line.test
```

Where the paper and the repository differ, the repository stays as it is:

| The paper | The repository |
|---|---|
| Sources are in a folder named after the project, `neon/` | The same, once for every library: `lib/neon-core/neon/`, `lib/neon-sdl2/neon/`, and so on. The libraries share the `neon` namespace and its include paths |
| Executables of tests are named after the file they test | Named after the library and the file, such as `neon-core.command-line.test`, since the libraries are built in one project and two of them may have a file of the same name |
| Says nothing about mocks | Shared mocks and fakes are headers in `lib/neon-core/neon/testing/` |
| Functional tests are programs in `tests/` | Three of the five are CMake scripts that start NeonRuntime, since that is what they test |

### Building and running

| Command | What it does |
|---|---|
| `cmake --build --preset <preset>-tests` | Builds every test, and NeonRuntime for the tests that start it |
| `cmake --build --preset <preset> --target neon-tests` | The same |
| `cmake --build --preset <preset> --target <test>` | Builds one test |
| `ctest --preset <preset>` | Runs every test |
| `ctest --preset <preset>-unit` | Runs the unit tests only |
| `ctest --preset <preset> -L integration` | Runs the functional tests only |
| `ctest --preset <preset> -R FileSystem` | Runs the tests whose name holds `FileSystem` |
| `<test> --gtest_filter='CommandLine*'` | Runs some tests of one executable. `--help` lists the other options |
| `cmake --build --preset <preset> --target user-interface.benchmark` | Builds the benchmark of the user interface, which is not a test. It is run from `build/<preset>/tests/user-interface.benchmark/` and prints how long a frame takes |

There are 2703 tests in 64 programs: 1936 unit tests and 767 functional
tests, of which 704 are of the user interface.

`<preset>` is `macos-arm64-debug` or `linux-x64-debug`.

| Platform | Build the tests | Run the tests |
|---|---|---|
| macOS arm64 | Yes | Yes |
| Linux x64 | Yes, in the Docker image | Yes, in the Docker image. A machine without a Vulkan driver skips the tests that render |
| Windows x64 | Yes, with `windows-x64-debug-tests` | Not where they are built. The cross-compiled tests have to be copied to a Windows machine and started there |

Tests are found when `ctest` runs, not while building. A test that was
cross-compiled cannot be started on the machine that built it, which finding
them during the build would need.

`-DNEON_BUILD_TESTS=OFF` leaves the tests and GoogleTest out of the project
altogether.

### Adding a test

1. Write `<name>.test.cpp` next to `<name>.cpp`.
2. Add one line to the `CMakeLists.txt` of the library, where the other tests
   are. Source files are listed by hand, and so are tests:

   ```cmake
   neon_add_unit_test(${TARGET} neon/command-line/command-line.test.cpp)
   ```

3. Run `cmake --preset <preset>` again.

```cpp
#include "command-line.hpp"

#include <gtest/gtest.h>

namespace
{
  TEST(CommandLine, RejectsAnOptionItDoesNotKnow)
  {
    neon::CommandLine command_line("program", "");
    const char *argv[] = {"program", "--unknown"};

    EXPECT_FALSE(command_line.Parse(2, argv));
    EXPECT_EQ(command_line.GetError(), "Unknown option '--unknown'");
  }
}
```

| Rule | Reason |
|---|---|
| The suite is named after the class, the test says what is expected: `TEST(CommandLine, RejectsAnOptionItDoesNotKnow)` | A failing test then reads as a sentence about what is broken |
| A test goes through the public interface | It keeps passing when the code behind the interface is rewritten |
| A test in neon-core does not call SDL, Vulkan, or Flecs | The same inversion of control as in the engine. A test of a backend library may use that backend |
| A test writes to a `TemporaryDirectory` only | Nothing is left in the repository or in the home folder |
| A test does not depend on another, or on the order they run in | Each one can be run by itself |
| A test that fails is never weakened or deleted | When the fix is not small, the test is kept with `DISABLED_` in front of its name and a comment that says why |

`neon_add_unit_test` takes `LIBRARIES <target>...` for a test that needs more
than its library. A functional test is declared in a `CMakeLists.txt` of its
own folder, with `neon_add_functional_test` for a program and
`neon_add_application_test` for a script that starts an application. All three
are in [cmake/NeonTests.cmake](../cmake/NeonTests.cmake).

### Mocks and fakes

They are headers in
[lib/neon-core/neon/testing](../lib/neon-core/neon/testing), in the namespace
`neon::testing`, and are included as `<neon/testing/recording-logger.hpp>`.
The tests of every library share them. They belong to the target
`neon-testing`, which only exists when tests are built, and which every test
links.

A mock checks what it is asked to do. A fake does the work in a simple way.

| Header | What it holds | Stands for |
|---|---|---|
| `recording-logger.hpp` | `RecordingLogger`, a fake that keeps what it is told | `Logger` |
| `memory-file-system.hpp` | `MemoryFileSystem`, a backend that keeps its files in memory. The path rules are those of `FileSystem` | `FileSystem`, `FileSystemContext` |
| `mock-file-system-context.hpp` | `MockFileSystemContext` | `FileSystemContext` |
| `mock-window-system.hpp` | `MockWindowSystem`, `MockWindowContext` | `WindowSystem`, `WindowContext` |
| `mock-input-system.hpp` | `MockInputSystem`, and `FakeInputContext` whose state a test sets | `InputSystem`, `InputContext` |
| `mock-render-context.hpp` | `MockRenderContext` | `RenderContext` |
| `mock-render-system.hpp` | `MockRenderSystem` | `RenderSystem` |
| `mock-render-pipeline.hpp` | `MockRenderPipeline` | `RenderPipeline` |
| `mock-render-2d-context.hpp` | `MockRender2DContext`, and `RecordingRenderer2D`, which keeps what it is asked to draw | `Render2DContext` |
| `mock-ui-system.hpp` | `MockUiSystem`, `MockUiContext` | `UiSystem`, `UiContext` |
| `fake-font-rasterizer.hpp` | `FakeFontRasterizer`, a font whose characters are all the same box | `FontRasterizer` |
| `mock-world-system.hpp` | `MockWorldSystem` | `WorldSystem` |
| `mock-entity-store.hpp` | `MockEntityStore` | `EntityStore` |
| `fake-entity-store.hpp` | `FakeEntityStore`, a store that is as simple as one can be | `EntityStore` |
| `mock-entity-world.hpp` | `MockEntitySystem`, `MockScene` | `EntitySystem`, `Scene` |
| `fake-physics-context.hpp` | `FakePhysicsContext`, a physics in which nothing collides, and which writes down what it was asked | `PhysicsContext` |
| `fake-entropy-context.hpp` | `FakeEntropyContext`, an entropy whose bytes a test chooses | `EntropyContext` |
| `temporary-directory.hpp` | `TemporaryDirectory`, a folder that is removed with the object | |

`TemporaryDirectory` uses `std::filesystem`, which the engine itself must not.
It is how a test prepares what a backend is expected to find, and looks at
what it left behind.

### What is covered

| Library | File | Tests | What is tested |
|---|---|---|---|
| neon-core | `command-line/command-line` | 94 | Every way to write an option, every message of the parser, defaults, whole numbers, numbers, the help text |
| neon-core | `command-line/runtime-options` | 67 | Every option and every message of `Apply`. The help text is compared with the one in this guide |
| neon-core | `common/data-buffer` | 20 | Ids, capacity, reuse of slots, what is thrown |
| neon-core | `common/rotation` | 21 | The quaternion of known angles, the order of the turns, and the angles of a quaternion |
| neon-core | `common/transform` | 11 | `Forward` and `Right`. 1 disabled |
| neon-core | `common/util` | 12 | All four functions |
| neon-core | `filesystem/file-system` | 164 | Every path rule for reading and writing, letter case, the native path, `output://` without a folder, where the log file is placed |
| neon-core | `input/input-state` | 17 | Actions, the motion of the mouse, the pointer, `Reset` |
| neon-core | `input/headless-input-system` | 7 | Nothing is ever pressed |
| neon-core | `logging/spd-logger` | 10 | Levels, arguments, what is thrown for a message that cannot be formatted |
| neon-core | `logging/deferred-file-sink` | 9 | What is held back until the file is opened, the limit, a file that cannot be opened |
| neon-core | `logging/logging-system` | 15 | The log file, in a temporary folder, what was logged before it was opened, the console when it cannot be |
| neon-core | `physics/model-geometry` | 7 | The points and triangles of a model, moved and sized as the renderer does it |
| neon-core | `random/entropy-seed` | 4 | The seed of known bytes, with the first byte lowest, and nothing without entropy. See [random-numbers.md](random-numbers.md) |
| neon-core | `render/forward-render-pipeline` | 24 | What is drawn in which order, the projection, lights and their limit |
| neon-core | `render/model` | 25 | Loading from a file system in memory, material files, textures, the normalization matrix. 1 disabled |
| neon-core | `runtime/frame-capture` | 21 | `NumberedPath`, and which frames are saved |
| neon-core | `runtime/runtime` | 34 | The order of `Initialize` and `CleanUp`, the loop, `HasFailed`, with and without a user interface, the entropy it is given |
| neon-core | `window/headless-window-system` | 16 | Closing, the time step, the size |
| neon-core | `world-system/ecs/component-info` | 15 | `ComponentInfo::Of` with a type that owns memory |
| neon-core | `world-system/ecs/entity-store` | 16 | What the templates of `EntityStore` ask a backend for, and `EntityBlock` |
| neon-core | `world-system/ecs/entity-world` | 38 | What is initialized in which order, the order of the systems in a frame, the steps of the world, what is released |
| neon-core | `world-system/ecs/fixed-clock` | 19 | The steps of the time that passed at every frame rate, a frame that took long, the blend |
| neon-core | `world-system/ecs/scene-file/physics-component-formats` | 43 | Reading and writing the components of the physics, and every message |
| neon-core | `world-system/ecs/systems/physics-simulation` | 73 | Creating and releasing bodies, shapes of several colliders, entities with a parent, what is handed to the physics and taken back, what is drawn between two steps |
| neon-core | `world-system/ecs/systems/spectator-movement` | 25 | Moving and turning by the input |
| neon-core | `world-system/ecs/systems/transform-propagation` | 21 | Parent times child, over several levels |
| neon-core | `world-system/ecs/systems/render-submission` | 31 | The camera, the lights, render objects. 1 disabled |
| neon-core | `layout/flex-layout-engine` | 120 | Flexbox as the specification describes it: growing and shrinking, alignment in both directions, wrapping, gaps, percentages, absolute boxes with every combination of sides, the box model |
| neon-core | `text/utf8`, `text/font-atlas`, `text/text-layout` | 57 | Reading UTF-8, what is wrong with it, the atlas of a font, where characters go, breaking lines |
| neon-core | `ui/*` | 151 | Values of CSS, properties, values of a game, batches of draw calls against a mock, fonts and images that are loaded once, what is left of the input |
| neon-core | `data/data-reader` | 5 | What a reader reports about what it reads as a whole |
| neon-core | `world-system/ecs/systems/ui-view-loading` | 12 | Showing the user interface of an entity, and taking it away |
| neon-stb | `text/stb-font-rasterizer` | 17 | Characters of Inter, which is compiled into the test |
| neon-vulkan | `render/vk-renderer-2d` | 13 | What needs no graphics card |
| neon-flecs | `world-system/flecs-entity-store` | 104 | Everything `EntityStore` promises, through that interface |
| neon-jolt | `physics/jolt-physics-system` | 89 | Everything `PhysicsContext` promises, through that interface, with worlds that are small enough to know what has to come out |
| neon-sdl2 | `filesystem/sdl2-file-system` | 41 | Real files in the three schemes, letter case on a disk that ignores it, the log file in `user://` |
| neon-vulkan | `render/vk-material` | 27 | What is handed to the shaders, textures that cannot be used |
| neon-vulkan | `render/vk-mesh` | 6 | A mesh with nothing to draw |
| neon-vulkan | `render/vk-model` | 4 | A model that cannot be used |
| neon-vulkan | `render/vk-shader` | 6 | Which files are read, shaders that cannot be used |
| neon-vulkan | `render/vk-shader-data` | 8 | Where every field lies that the shaders read |
| neon-vulkan | `render/vk-texture` | 7 | Textures that cannot be used |
| `tests/` | `world-with-flecs` | 16 | The world as an application puts it together, with the store of Flecs and the forward pipeline |
| `tests/` | `runtime-command-line` | 11 | NeonRuntime with a command line it refuses: exit code and message |
| `tests/` | `runtime-headless` | 8 | NeonRuntime without a window: exit code, and the images it saved, with and without a user interface |
| `tests/` | `user-interface` | 300 | UI recipes through the whole user interface: every element, every message for a file that is wrong, layout in frames of several sizes, values, input, focus, events, and what is drawn |
| `tests/` | `physics-with-jolt` | 21 | The physics as an application puts it together, with Flecs, Jolt, and a scene recipe. The same state after the same steps at every frame rate |
| `tests/` | `runtime-physics` | 2 | NeonRuntime with the scene of the physics: the same images twice, and at every frame rate |
| neon-core | `text/glyph-atlas`, `text/shaped-text`, `text/text-case` | 75 | Glyphs that are drawn on demand into pages, parts of a text by font, script, and direction, lines, quarters of a pixel, capitals |
| neon-core | `ui/css-functions`, `ui/ui-paint`, `ui/ui-box-paint`, `ui/ui-values-fallback` | 69 | Gradients, shadows, and transforms as CSS writes them, the shape of a box, where an image goes, values that fall back on others |
| neon-core | `image/image-pixels` | 11 | Smaller copies with alpha multiplied in: white next to nothing stays white |
| neon-core | `render/forward-render-pipeline-cameras` | 11 | Cameras that draw into a texture |
| neon-core | `world-system/ecs/systems/ui-clock` | 2 | The time of the user interface |
| neon-freetype | `text/ft-font-rasterizer`, `text/hb-text-shaper` | 30 | Glyphs of Inter and of Noto Sans Arabic, which are compiled into the tests: parts of a pixel, hinting, distances, the line around a glyph, kerning, a ligature, Arabic in its order and joined |
| neon-lunasvg | `image/luna-vector-image-rasterizer` | 14 | An SVG at several sizes, as sharp at each, and what is refused |
| neon-stb | `image/stb-image-decoder` | 9 | PNG, JPEG, BMP, TGA, and PSD with 8 and 16 bits |
| neon-platform | `random/os-entropy` | 5 | Bytes of the operating system are filled and differ between two fills, nothing to fill, more than one call's worth, a seed |
| neon-vulkan | `render/vk-shader-values`, `render/vk-render-target` | 19 | The values a compiled shader declares, and what needs no graphics card of a render target |
| `tests/` | `text-shaping` | 20 | Text with real fonts through the core: kerning, a ligature, Arabic, a second font for what the first does not have, quarters of a pixel, an atlas that grows |
| `tests/` | `user-interface`, what was added | 211 | The properties of text, boxes, images, shaders of elements, and surfaces: what is handed to the renderer, where the pointer is on round corners and on what is moved and turned, what is wrong in a file |
| `tests/` | `runtime-surfaces` | 4 | NeonRuntime without a window with the gallery, shaders at two times, and the scene with surfaces in the world |
| `tests/` | `runtime-vertex-colours` | 4 | NeonRuntime without a window with a model painted by vertex: the colours unlit and lit, and a material the file marks double-sided seen from behind, with and without the scene's say |

The headers that only declare an interface or a plain structure have no test
of their own. There is nothing in them that can be wrong by itself.

**Disabled tests.** Each one describes what the code should do and does not.
The fix is a question of design, so the test waits with a comment that says
what is open. `ctest` lists them as not run.

| Test | What is open |
|---|---|
| `Transform.RightIsANumberWhenLookingStraightUp` | `Right()` is the cross product of forward and up. Straight up and down the two are parallel and the result is not a number |
| `ModelTest.FailsAndSaysSoWhenTheFileHoldsNoModel` | A file that is named like a model and holds none loads without an error, as a model without meshes |
| `RenderSubmissionTest.DoesNotAskTheRendererAgainForAnEntityItCouldNotCreate` | An entity the renderer could not create is handed to it again in every frame. -1 stands for both "not created yet" and "could not be created" |

### What is not covered

| What | Why | What would make it testable |
|---|---|---|
| `SDL2_WindowSystem`, `SDL2_InputSystem` | They need a display and input devices. Both call SDL directly, which keeps its state for the whole process | Running the tests under a display server that draws nowhere, such as Xvfb or the `dummy` video driver of SDL. Turning an SDL event into an `InputState` is logic of its own and could move into a function that takes the event |
| `VK_Device`, `VK_RenderSystem`, and the successful paths of `VK_Mesh`, `VK_Model`, `VK_Shader`, `VK_Texture`, `VK_Material` | They need a graphics card. Vulkan is reached through global function pointers, which cannot be replaced by a mock | They are covered from the outside by `runtime-headless`, on a machine that has a graphics card. A software renderer such as lavapipe would let that test run everywhere |
| The helpers in `vk-device.cpp` and `vk-render-system.cpp`, such as `rate_device_type`, `align_up`, and `BuildSceneData` | They are private to their file or their class | Moving them into a header of their own. `BuildSceneData` turns lights into shader data and needs no graphics card to do it |
| `SDL2_FileSystem` on Windows | Windows is asked for the folder of the user directly and takes no hint, so the tests would write into the real one. They are skipped there | Handing the folder behind `user://` to the file system through the settings, as `output_directory` is |
| An output folder that is relative to the working directory | The test would have to change the working directory of its process | |
| `LoggingSystem::CreateLogger()` before `Initialize()` | It uses a logger that does not exist yet and ends the process | Creating the sinks in the constructor, or returning a logger that holds back what it is told |
| `Logger::CreateChildLogger` | It is protected and nothing calls it | |
| An `EntityWorld` that goes away before its store, without `CleanUp()` | The store tells a `Renderable` that it is removed through a function that belongs to the world. Called after the world is gone, it reads memory that was released. Applications call `CleanUp()` first, as `main.cpp` does | The world cleaning up in its destructor, as `Runtime` does |
| `app/` | It is put together in `main()` | It is covered from the outside by the tests in `tests/` |

## Code style

The conventions of the code are described in the
[style guide](style-guide.md). Two targets check code against it, and
neither is part of a normal build:

```bash
cmake --build build/macos-arm64-debug --target tidy
cmake --build build/macos-arm64-debug --target format-check
```

| Target | Does |
|---|---|
| `tidy` | Runs clang-tidy with the rules in [.clang-tidy](../.clang-tidy) |
| `format-check` | Lists what clang-format would change with the rules in [.clang-format](../.clang-format). It changes nothing |

When tests are built, which is the default, both targets check the code of
the tests as well.

Both tools are part of LLVM 20. The Docker images need `clang-tidy` and
`clang-format` on `PATH` for the targets to work there.

## Build notes

A few settings in the top-level [CMakeLists.txt](../CMakeLists.txt) exist only
to keep the three platforms building with the same toolchain. Each carries a
comment, but in short:

- The engine's own targets, the libraries under `lib/`, the applications, and
  the tests, are compiled with `-Wall -Wextra -Werror`, set per target by
  `neon_warnings()` in [cmake/Warnings.cmake](../cmake/Warnings.cmake). The
  libraries under `external/` are not, and their headers are included as
  system headers. The [style guide](style-guide.md#compiler-warnings) lists
  the two categories that are off, and why.
- `BUILD_SHARED_LIBS` is forced off. assimp turns it on by default and that
  would leak into every dependency added after it, producing executables that
  depend on `.so` or `.dll` files from the build tree.
- assimp builds without `-Werror`. Each new clang release adds warnings and
  assimp's own code is not ours to fix.
- assimp always uses its bundled zlib. A system zlib on macOS pulls the SDK's
  C headers in ahead of libc++'s and breaks every standard header include under
  Homebrew LLVM.
- On macOS, that bundled zlib gets `fdopen=fdopen` predefined to dodge a zlib
  1.2.13 bug that defines `fdopen` as `NULL` on Apple targets.
- On MinGW, rapidyaml gets `C4_MINGW` defined because c4core only detects
  MinGW under GCC, and without it includes `<alloca.h>`, which mingw-w64 lacks.
- The spdlog submodule is pinned to v1.15.3 or newer. The fmt 10 bundled with
  older releases does not compile under clang 20 in C++20 mode.

`main.cpp` includes `<SDL_main.h>` and takes `argc`/`argv`. SDL2main supplies
the real Windows entry point and expects that signature; without it the Windows
link fails on an undefined `SDL_main`.

## Manual configuration

The presets are only a convenience. The equivalent manual invocation, using
macOS as an example:

```bash
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm@20/bin/clang \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm@20/bin/clang++ \
  -S . -B build/macos-arm64-debug
cmake --build build/macos-arm64-debug --target NeonRuntime
```

For Windows, replace the compiler variables with
`-DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/windows-x64-llvm-mingw.cmake`.

## IDE

The project was primarily developed with CLion, which picks up
`CMakePresets.json` automatically. VS Code with the CMake Tools extension does
the same.

[.vscode/tasks.json](../.vscode/tasks.json) also defines plain VS Code tasks
that wrap the preset commands, so no extension is needed. Run them with
`Terminal > Run Task`; each one asks which preset to use, debug or release
of each target, and defaults to the debug build of macOS.

| Task | What it runs |
|---|---|
| Configure | `cmake --preset <preset>` |
| Build | `cmake --build --preset <preset>` (default build task, Cmd+Shift+B). A preset that has not been configured yet is configured first, so a preset can be picked and built without running Configure |
| Clean | the preset's `clean` target; nothing when the preset has not been configured yet |
| Rebuild | Clean, then Build |
| Full Clean | deletes the preset's build directory and the `bin` output of its build type |
| Run | builds, then runs the macOS binary of the preset from its own folder without a debugger. It asks for a scene: left empty, the entry scene of the project starts; a name such as `prototype` starts `assets://scenes/prototype.scene.yml`; a virtual path is taken as it is |
| Build in Docker (Linux x64 / Windows x64) | configure and build inside the matching image, debug or release as asked |

### Running and debugging from VS Code

[.vscode/launch.json](../.vscode/launch.json) has one launch configuration per
platform binary: macOS arm64, Linux x86_64, Linux aarch64, and Windows x86_64.
Pick the one matching the machine VS Code is running on. Each builds first,
then starts the binary from its own folder so the relative asset paths resolve.

They all use the
[CodeLLDB](https://marketplace.visualstudio.com/items?itemName=vadimcn.vscode-lldb)
extension, which VS Code offers to install when you open the workspace and
which works on all three operating systems. The Linux and Windows
configurations only make sense on a native checkout there, since a binary
produced inside Docker on a Mac cannot be launched from that Mac. On Windows
the Build task expects llvm-mingw on `PATH`, as described in the Windows
section above.

- **F5** starts it under the debugger.
- **Ctrl+F5** (Run > Run Without Debugging) starts it without one.
- The **Run** task does the same as Ctrl+F5 from the task menu.

Breakpoints, stepping, and variable inspection work as normal. The build is
Debug with optimisation off, so nothing is inlined away.
