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
`bin/<build-type>/<os>-<arch>/<app-name>/`.

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
  external/spdlog external/rapidyaml external/stb
```

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
`.frag.spv`. Scene files therefore never mention a shader format.

| Shader | Draws |
|---|---|
| `basic-lit` | Textured or plain colored surfaces lit by one direction light and up to 64 point and 64 spot lights |
| `unlit` | The texture as it is, without lighting |
| `color` | The plain color of the material, without lighting |

To add one, put its `.vert` and `.frag` files in the sources folder and run
the configure step again.

## Command line

```
Usage: NeonRuntime [options]

  --help             Show this text
  --renderer vulkan  Renderer to draw with. Default: vulkan

Development:
  --frames N         Stop after N frames
  --screenshot PATH  Save the last frame as a PNG image before stopping. Needs --frames
  --headless         Run without a window
```

A switch is written as `--name`. An option with a value is written as
`--name value` or `--name=value`.

Together the development options render a scene without a window and save the
result, which is how the renderer is checked on a machine with no display:

```bash
NeonRuntime --headless --frames 3 --screenshot user://screenshots/frame.png
```

The log says where `user://` is on the machine.

### How it is built

Each application decides which options it accepts. NeonRuntime has one set.
NeonEditor will have that set and one of its own, which the runtime never
sees.

| Piece | Location | Role |
|---|---|---|
| `CommandLine` | [neon-core](../lib/neon-core/neon/command-line/command-line.hpp) | Parses arguments, checks them, and writes the help text. Knows no application |
| `CommandLineContext` | [neon-core](../lib/neon-core/neon/command-line/command-line-context.hpp) | The read side, for code that wants to know what was asked for |
| `CommandLineOptions` | [neon-core](../lib/neon-core/neon/command-line/command-line-options.hpp) | Interface of a set of options: what they are, and what they do to the settings |
| `RuntimeOptions` | [neon-core](../lib/neon-core/neon/command-line/runtime-options.hpp) | The set every runtime has |

An application puts them together in `main.cpp`:

```cpp
neon::CommandLine command_line("NeonRuntime", "Runs a Neon Engine project.");
neon::RuntimeOptions runtime_options;
runtime_options.Register(command_line);

if (!command_line.Parse(argc, argv)) { /* print GetError() and GetHelp() */ }
if (command_line.WantsHelp()) { /* print GetHelp() */ }

SettingsConfig settings_config{ /* defaults of the application */ };
runtime_options.Apply(command_line, settings_config, error);
```

### Adding options

To add an option to every runtime, declare it in `RuntimeOptions::Register()`
and act on it in `RuntimeOptions::Apply()`.

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
It defaults to `Windowed`. NeonRuntime sets `Borderless` in its `main.cpp`.

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
files, such as a scene file, can be moved between platforms unchanged. Loading
also never depends on the directory the app was started from.

| Scheme | Points at | Status |
|---|---|---|
| `assets://` | `<directory of executable>/assets` | Read-only |
| `user://` | A folder of the current user, for saves, settings, and anything else the app writes | Read and write |

Where `user://` lives depends on the platform, and on the `organization` and
`application` names in `SettingsConfig`, so that applications do not share a
folder.

| Platform | `user://` |
|---|---|
| macOS | `~/Library/Application Support/<organization>/<application>/` |
| Linux | `~/.local/share/<organization>/<application>/` |
| Windows | `%APPDATA%\<organization>\<application>\` |

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
function that returns one. `FileSystem::Locate` is the single place a native
path is produced, and it is protected, so only backends can call it. It checks
the rules, matches each name against the disk, and joins the folders with the
platform's own separator.

### Writing a backend

A backend derives from `FileSystem` and implements:

| Function | Purpose |
|---|---|
| `Initialize()` | Set `_assets_directory`, `_user_directory`, and `_native_separator` |
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

- Write virtual paths, in code and in scene files, following the rules above.
- Never build or store a native path outside a backend.
- Take a `FileSystemContext *` through the constructor and read through it.
- Keep the stored path in its scheme form so log messages stay readable.
- Do not include SDL or use `std::filesystem` for file access outside a
  backend library.

### Lifecycle

The file system is created and initialized in `main.cpp` right after logging,
before any system that loads files. It is cleaned up last, after the
application has shut its systems down.

The log file path in `SettingsConfig` is still relative to the working
directory and does not go through the file system yet. `user://` is where it
belongs.

The libraries that were considered and the conditions for moving to a full
virtual file system are recorded in [file-systems.md](file-systems.md).

## Build notes

A few settings in the top-level [CMakeLists.txt](../CMakeLists.txt) exist only
to keep the three platforms building with the same toolchain. Each carries a
comment, but in short:

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
`Terminal > Run Task`; each one asks which preset to use and defaults to macOS.

| Task | What it runs |
|---|---|
| Configure | `cmake --preset <preset>` |
| Build | `cmake --build --preset <preset>` (default build task, Cmd+Shift+B) |
| Clean | the preset's `clean` target |
| Rebuild | Clean, then Build |
| Full Clean | deletes the preset's build directory and the `bin` output |
| Run | builds, then runs the macOS binary from its own folder without a debugger |
| Build in Docker (Linux x64 / Windows x64) | configure and build inside the matching image |

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
