# The museum

One scene that shows what the engine does, to walk through from the first
person: a corridor with halls, one for each part of the engine, each with a
sign that says what it shows. It is described in
[docs/scenes.md](../../docs/scenes.md#the-museum).

It is a project of its own with no code of its own: `assets/project.yml` is
the project, and next to it are its scene, its prefabs, its signs, its
scripts, its sky, and its hum. Nothing in it comes from a model file: every
piece is a `Geometry` in a plain color.

## Building it

By the engine's build, next to the applications: every build preset builds
it with NeonRuntime and the bench.

```sh
cmake --build --preset macos-arm64-debug
bin/debug/darwin-arm64/museum/NeonRuntime
```

Or on its own, against a NeonRuntime that is built already:

```sh
cmake -S projects/museum -B build/museum -G Ninja
cmake --build build/museum
build/museum/museum/NeonRuntime
```

The game is put together in that folder: the runtime, its assets with this
project's on top, `project.yml` above all.
