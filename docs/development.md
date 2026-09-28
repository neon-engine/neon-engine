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

A GPU with OpenGL 3.3 support is needed to run the result.

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
brew install llvm@20 cmake ninja
```

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
and installs LLVM 20 from apt.llvm.org plus the X11, Wayland, OpenGL, and audio
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
