# The bench

Scenes that measure the engine: the same work done by a script in Lua, by a
system in C++, and by nothing. It is a project of its own with code of its
own, and the first game in C++ on [extensions](../../docs/extensions.md): a
project with an extension. `assets/project.yml` is the project, and
`extensions/bench/` is everything else: its code, a library that NeonRuntime
loads, and under `assets/` its scenes, prefabs, scripts, and counter, which
the project names as `extensions://bench/assets/…`. Nothing of it is linked
into the runtime, and nothing of the engine is compiled to build it.

It is run by hand, never by the tests.

## Building it

On its own, against a NeonRuntime that is built already. This compiles
`bench.cpp` and nothing else, in a second or two, and is how it is worked on:

```sh
cmake -S projects/bench -B build/bench -G Ninja
cmake --build build/bench
build/bench/bench/NeonRuntime
```

The runtime is the engine's release build when there is one, else its debug
build, or the folder `-DNEON_RUNTIME_DIRECTORY=<folder>` names. A number
should come from a release build of the runtime, and of the bench:
`-DCMAKE_BUILD_TYPE=Release`.

Or by the engine's build, on request, next to the applications:

```sh
cmake --preset macos-arm64-debug
cmake --build build/macos-arm64-debug --target bench
bin/debug/darwin-arm64/bench/NeonRuntime
```

Either way the game is put together as:

| There | Is |
|---|---|
| `NeonRuntime` | The runtime, as it was built |
| `assets/` | The runtime's assets, with this project's `project.yml` on top |
| `extensions/bench/` | `bench-<platform>.dylib`, `.so`, or `.dll`, its `extension.yml`, and its `assets/` |

## What it measures

Every behaviour exists twice with the same work: as a script under
`extensions/bench/assets/scripts`, and as a system next to it in C++.

A scene is named for how its crates arrive, the language, and what each
crate does: `rain-lua-heavy-copy`.

| Part of the name | Means |
|---|---|
| `rain` | A spawner drops crates for ever, sixty a second with no cap |
| `falling`, `2000` | 2000 crates are placed at once, in layers of a grid so that every run starts the same, and fall onto a floor |
| `lua`, `cpp` | A script does it, or a system of the extension |
| `light-copy` | Each crate adds the time of the frame to one field: `LightCopy` in Lua, `NativeLightCopy` in C++. The least a behaviour can do, so the cost of a hook alone |
| `heavy-copy` | Each crate copies its position out of the engine and back sixty-four times a frame: `HeavyCopy`, `NativeHeavyCopy`. The worst case, for the cost of the crossing between a script and the store rather than of arithmetic |
| `no-copy` after a language | Each crate reads its height from the engine's `Transform` in place once a frame, and counts its time and the frames it was below a line: `NoCopy`, `NativeNoCopy` |
| `rain-no-copy`, `falling-none-2000` | No script and no system: each crate is a dynamic body and nothing more. What the others are measured against |

| Scenes | |
|---|---|
| `rain-no-copy` | The rain alone |
| `rain-cpp-light-copy`, `rain-lua-light-copy` | The rain, with the cost of a hook |
| `rain-cpp-heavy-copy`, `rain-lua-heavy-copy` | The rain, with the worst case |
| `falling-none-2000`, `falling-cpp-no-copy-2000`, `falling-lua-no-copy-2000` | The 2000, alone and with a read in place |

The HUD names the scene and says what its crates do, and a counter under it
and a line in the log once a second say how many crates are on the scene. The spawner and the counter are in C++ in every scene, so
that what differs between two scenes is the behaviour of the crates alone.

For a number, run without a window and time it:

```sh
time build/bench/bench/NeonRuntime --headless-renderer --window-size 320x180 \
  --time-step 0.016667 --frames 300 --scene extensions://bench/assets/scenes/falling-lua-no-copy-2000.scene.yml
```

For watching, with the Metal performance HUD on macOS:

```sh
MTL_HUD_ENABLED=1 build/bench/bench/NeonRuntime --window-size 1600x900 \
  --scene extensions://bench/assets/scenes/rain-lua-heavy-copy.scene.yml
```

## How the code is laid out

Everything is under `extensions/bench/`.

| File | Is |
|---|---|
| `bench.cpp` | The extension: it registers the components and adds the systems |
| `crate.hpp`, `native-light-copy.hpp`, `native-no-copy.hpp`, `native-heavy-copy.hpp`, `native-spawner.hpp` | The components, plain structs with their defaults |
| `light-copying.hpp`, `no-copying.hpp`, `heavy-copying.hpp` | The twins of the scripts. `NoCopying` and `HeavyCopying` read the position of the engine's `Transform` in place, in the columns of a query |
| `spawning.hpp` | Places the crates, from a prefab chosen by `behaviour` |
| `counting.hpp` | Counts the crates of every kind, those of the scripts too, and names the scene on the HUD |
| `assets/scripts/*.lua` | `LightCopy`, `NoCopy`, `HeavyCopy` |
| `assets/prefabs/crate-*.prefab.yml` | A crate for each behaviour |
| `assets/scenes/*.scene.yml`, `assets/ui/counter.ui.yml` | The scenes, and the counter |

For Windows, from the image of `docker/windows-x64.dockerfile`, with the
toolchain file by its whole path:

```sh
cmake -S projects/bench -B build/bench-windows -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/windows-x64-llvm-mingw.cmake"
cmake --build build/bench-windows
```

The bench that was closed with #314 was linked into the runtime. Its numbers
are not carried over, and none have been taken with this one yet.
