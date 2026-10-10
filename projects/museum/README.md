# The museum

One scene that shows what the engine does, to walk through from the first
person: a corridor with halls, one for each part of the engine, each with a
sign that says what it shows. It is described in
[docs/scenes.md](../../docs/scenes.md#the-museum).

It is a project of its own with no code of its own: `assets/project.yml` is
the project, and next to it are its scene, its prefabs, its signs, its
scripts, its sky, its hum, and its sounds. Nothing in it comes from a model
file: every piece is a `Geometry` in a plain color.

## What is heard

A quiet piano piece plays everywhere, and a bed of wind and birds under it.
Birds are heard by the entrance and a stream by the far wall, so that walking
the corridor moves from the one to the other. The sound hall adds its hum, a
tone the engine's tools wrote. The music and the ambience are public domain
(CC0) recordings from [OpenGameArt](https://opengameart.org), encoded as Ogg
Vorbis; who made them is in [CREDITS.md](../../CREDITS.md).

| File | What | By |
|---|---|---|
| `sounds/music/core-theme.ogg` | Piano with pads and Japanese-inspired instruments | Yoiyami |
| `sounds/ambience/birds-and-wind.ogg` | Synth pads with bird calls | Spring Spring |
| `sounds/ambience/park-birds.ogg` | Birds before rain, a park in Adelaide | Thimras |
| `sounds/ambience/park-river.ogg` | A small river over a step, with frogs | Thimras |

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
