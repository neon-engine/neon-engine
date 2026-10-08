# Quick start

The fastest way from a clone to a running game: the museum, one scene walked
through in the first person, with a hall for each part of the engine. The
[development guide](development.md) covers the rest in depth: every platform
and preset, native builds without Docker, the build options, and the tests.

1. Clone this repository with the URL from the page you are reading, and fetch
   its submodules:

   ```sh
   cd neon-engine
   git submodule update --init --recursive
   ```

2. Build. Each build also produces the museum, the bench (scenes that measure
   Lua scripts against C++), and the sandbox.

   **macOS on Apple silicon**, with [Homebrew](https://brew.sh):

   ```sh
   brew install llvm@20 cmake ninja glslang molten-vk vulkan-loader
   cmake --preset macos-arm64-debug
   cmake --build --preset macos-arm64-debug
   ```

   **Linux x64**, in Docker:

   ```sh
   docker build --platform linux/amd64 -t neon-engine/linux-x64 -f docker/linux-x64.dockerfile docker
   docker run --rm --platform linux/amd64 -v "$PWD":/src -w /src neon-engine/linux-x64 \
     sh -c 'cmake --preset linux-x64-debug && cmake --build --preset linux-x64-debug'
   ```

   **Windows x64** is cross-compiled in Docker, as
   the [development guide](development.md#windows-x64) describes.

3. Run the museum (on Linux, the folder is `linux-x86_64`):

   ```sh
   ./bin/debug/darwin-arm64/museum/NeonRuntime
   ```

| Action | Keyboard and mouse | Gamepad |
|---|---|---|
| Move | W, A, S, D | Left stick |
| Look | Mouse | Right stick |
| Jump | Space | South button |
| Run | Left Shift | Left stick press |
| Press a screen in the world | Left mouse button, aiming with the dot in the middle | South button, aiming with the dot |
| Pause | Escape | Start |

The halls and what each one shows are listed in
[scenes.md](scenes.md#the-museum). For a faster build, use the
`-release` presets in place of `-debug`, and run from `bin/release/` instead.
