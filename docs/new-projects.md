# New projects

How a project of its own is made outside the engine: what is generated,
where it goes, how it is built, and how it finds the engine. It is the
library half of the editor's "New project" (#573, under #82); the dialog
comes with the editor's window (#568). Until then the generator is a
command line, `new-project`, built with the tests.

**Where this stands.** `ProjectTemplate` of `app/NeonEditor` writes the
folder, the command line calls it, and a test builds what was written on
its own and runs it without a window. The dialog of the editor, the editor
file that remembers the parent folder, and opening the new project in the
editor at once are still to come.

## What is made

```sh
build/macos-arm64-debug/tests/runtime-new-project/new-project hello-neon
```

makes `~/Dev/hello-neon/`:

```
hello-neon/
  CMakeLists.txt               NEON_ENGINE_DIRECTORY, NeonSdk.cmake, neon_add_project
  CMakePresets.json            the presets every project shares
  README.md                    how to build and run it
  .gitignore                   build/, user-editor.yml, what a desktop leaves
  assets/
    project.yml                the name, the organization, the scene, the input map
    settings.yml               the engine's pause and settings menus
    scenes/start.scene.yml     a camera, a light, a floor, and a box, from Geometry
    input/game.input.yml       the engine's default map, as the project's own
  extensions/
    hello-neon/
      extension.yml            the recipe, with a library for every platform
      hello-neon.cpp           an extension in C++: Spinner, and Spinning that turns the box
```

Everything in it works as it is: the first build compiles the extension,
and the first run shows an orange box on a gray floor turning under one
light. Nothing is an empty file; a folder that has nothing in it is not
made, as [project-layout.md](project-layout.md) asks. The layout is the one
of the sandbox, [projects/sandbox](../projects/sandbox), and of the bench,
[projects/bench](../projects/bench), which is the first game in C++ on
extensions, see [projects.md](projects.md#a-project-with-code-in-c).

| Decision | Reason |
|---|---|
| The project is made outside the engine, under `~/Dev` by default | A game is not part of the engine's tree, see [projects.md](projects.md). neon-quake lives next to the engine the same way. The folder is the owner's choice (2026-10-11) |
| `project.yml` is in `assets/` | It is where the runtime reads it today. It moves to the root with #368, which is one line of `ProjectTemplate` |
| The extension does something, a box that turns, in place of a hello in the log | A first run that shows the extension at work is what tells a person their build works, and the system is the one the header of extensions shows as its example |
| The input map is the engine's own, copied into the project | A project that names no map is played with the engine's, but a game changes its map first, so it is there to change |
| One scene, no prefabs, no user interface of its own | What a game adds comes with the game. The menus are the engine's, `engine://ui/pause.ui.yml` and `engine://ui/settings.ui.yml`, as the sandbox names them |
| The names are plain, as `ProjectFile` asks: lowercase letters, digits, and dashes, starting with a letter | They become folder names on every platform, and the name of the extension, its library, and the CMake target. Anything else is refused with why |
| A folder that is there already is never written into | A project is only made where nothing is; a person's work is never overwritten |

## How it is built

Every project made this way is built with the same commands, which is what
the toolchains of the editor rely on (#572):

```sh
cmake --preset macos-arm64-debug
cmake --build --preset macos-arm64-debug
build/macos-arm64-debug/hello-neon/hello-neon
```

Nothing of the engine is compiled. The project includes
[cmake/NeonSdk.cmake](../cmake/NeonSdk.cmake) of the engine, compiles its
extension, and puts the game together in the build folder with a
NeonRuntime that is built already: the engine's release build, else its
debug build, or the folder `-DNEON_RUNTIME_DIRECTORY=` names, see
[projects.md](projects.md#a-project-with-code-in-c).

The presets are `macos-arm64-debug`, `linux-x64-debug`,
`windows-x64-debug`, and the same with `-release`. They mirror the engine's
own, [CMakePresets.json](../CMakePresets.json), for what a project needs:
the compiler of each platform, Ninja, `build/<preset>/` as the build
folder, and the release flags. They are the same in every project and
carry no path.

| Decision | Reason |
|---|---|
| The presets name no path of the machine | Two projects are the same but for their names, and the file is committed. What is of this machine, the engine, is in `CMakeLists.txt`, which is written once for it |
| The Windows presets set `NEON_TOOLCHAIN=windows-x64-llvm-mingw`, and `CMakeLists.txt` turns it into `CMAKE_TOOLCHAIN_FILE` under the engine's folder before `project()` | A preset's `toolchainFile` cannot be under a cache variable, and the toolchain files are the engine's, not copied into every project |
| No test presets | A new project has no tests. neon-quake shows how a project adds them, with GoogleTest from the engine's checkout |

## How it finds the engine

A project is built against a checkout of the engine, always, for now
(decided by the owner on 2026-10-11): what it uses of the checkout is
[cmake/NeonSdk.cmake](../cmake/NeonSdk.cmake), the two headers of
extensions in `lib/neon-extension-api`, and a NeonRuntime the checkout
built already under its `bin/`. Nothing else of the engine is read, and
nothing of it is compiled.

`CMakeLists.txt` names the checkout as the default of
`NEON_ENGINE_DIRECTORY`, the folder with `cmake/NeonSdk.cmake`, as
neon-quake, the game that lives next to the engine, does:

```cmake
set(NEON_ENGINE_DIRECTORY "/Users/someone/Dev/neon-engine" CACHE PATH
        "The checkout of Neon Engine this is built against")
```

It is the checkout the generator was built from, or the folder it was told
with `--engine`. Another is given with `-DNEON_ENGINE_DIRECTORY=<folder>`;
a folder without `cmake/NeonSdk.cmake` stops the configure with what to do.
The path is written with forward slashes on every platform, which is how
CMake reads one.

## The generator

`neon::editor::ProjectTemplate`
([app/NeonEditor/neon/editor/templates](../app/NeonEditor/neon/editor/templates))
takes a `NewProject`: the name, the organization, the parent folder, and
the engine folder. An empty parent folder stands for `Dev` under the home
folder, which the platform says. `Create` checks the names, the engine, and
that the folder is new, then writes every file, and answers with the folder
it made or one message saying what was wrong, for the dialog and the log.
`ProjectTemplateText` holds the text of each file, with the name of the
project where `@name@` stands.

It writes through `NativeFileContext`
([lib/neon-core/neon/filesystem](../lib/neon-core/neon/filesystem/native-file-context.hpp)),
what a tool of the editor sees of the files of the machine by native path:
the home folder, joining paths with the separator of the platform, making
folders, writing a file. `FileSystemContext` cannot do this: its schemes
keep a game inside its own folders, and a new project goes where the person
says, outside every one of them. Nothing a game runs takes it. The backend
is `OS_NativeFiles` of `lib/neon-platform`, on the standard library and
`HOME` or `USERPROFILE`; the tests of the generator use
`FakeNativeFileContext`, which keeps the files in memory.

`neon-editor`, a library of `app/NeonEditor`, is the editor's own objects,
free of any user interface library: what a person asks the editor to do,
done on the interfaces of neon-core, so that the window, a command line, a
test, and a service call the same thing. It lives with the editor's
executable, not under `lib/`, since the editor is an application (decided
by the owner on 2026-10-11).

## The command line

```
new-project <name> [--organization NAME] [--parent FOLDER] [--engine FOLDER]
```

The organization is the name unless given, the parent folder `~/Dev`, and
the engine the checkout the tool was built from. The folder that was made
is the last line it prints. It is built with the tests
(`cmake --build --preset macos-arm64-debug --target new-project`) and will
be `NeonEditor --new-project` once the editor exists.

## The test

`runtime-new-project.a-new-project-builds-and-runs`
([tests/runtime-new-project](../tests/runtime-new-project)) makes a project
with the command line into the folder of the test, configures and builds
it on its own against the runtime the engine's build made, as a person
builds one, and runs it without a window for a few frames with a
screenshot. It checks the exit code, that the extension said it is up, and
that there were no errors. The unit tests of `ProjectTemplate` and
`ProjectTemplateText` check the files without touching the disk.

```sh
ctest --preset macos-arm64-debug -E runtime- -R 'ProjectTemplate|OSNativeFiles'
ctest --preset macos-arm64-debug -j 1 -R runtime-new-project
```

## Open questions

- **An editor without a checkout**: a project builds against a checkout
  of the engine for now. Where `NeonSdk.cmake`, the headers of extensions,
  and a prebuilt runtime come from for an installed editor is later.
- **The parent folder** is remembered in the user's editor file, with the
  editor (#572, #568).
- **`project.yml` at the root** (#368), and with it whether `settings.yml`
  moves too.
- **A project without code**, Lua only, as the sandbox: whether the dialog
  offers it, or the extension is removed by hand.
