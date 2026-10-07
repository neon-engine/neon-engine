# The sandbox

The project the engine is tried out with: one scene, a box on a floor under
one light, built by the engine with nothing read from a file. NeonRuntime
ships no game of its own, so this is what to run after building the engine,
and the place for a quick try of something new before it gets an exhibit in
the museum.

It has what every game brings for itself: `assets/project.yml` and its
settings. It names no input map, so it is played with the engine's,
`engine://input/default.input.yml`. Its settings name the pause and settings menus the engine
brings, `engine://ui/pause.ui.yml` and `engine://ui/settings.ui.yml`.

## Building it

By the engine's build, next to the applications: every build preset builds
it with NeonRuntime, the museum, and the bench.

```sh
cmake --build --preset macos-arm64-debug
bin/debug/darwin-arm64/sandbox/NeonRuntime
```

Or on its own, against a NeonRuntime that is built already:

```sh
cmake -S projects/sandbox -B build/sandbox -G Ninja
cmake --build build/sandbox
build/sandbox/sandbox/NeonRuntime
```
