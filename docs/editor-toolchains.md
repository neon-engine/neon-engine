# Editor toolchains

This note records how the editor builds and runs a project: the toolchains
it is told about, where they are declared, and what it does with them. The
objects are in `app/NeonEditor/neon/editor/toolchains/`, the library
`neon-editor` of the editor's application; the window that
shows them is the editor's (#572, #568).

**Current decision:** two files declare the toolchains. `editor.yml` at the
root of a project is the team's, committed with the project. `user-editor.yml`
is one machine's: it is read from the editor's own folder, `editor://`, and
then from the root of the project, where git ignores it, each laid over
what came before. A toolchain is native or Docker, says which platform it
builds for, which programs it needs, and how the project is configured,
built, and run, as three command lines with placeholders. The editor checks
at start which toolchains are usable on the machine, builds with one in a
process of its own, and starts the result, which is named after the project.

The owner asked for `Editor.yml` and `UserEditor.yml`. The lowercase names
with a dash follow [project-layout.md](project-layout.md#names), which every
file of a project follows: a name has to be the same file on macOS, Linux,
and Windows, and lowercase cannot be miscased.

## The files

| File | Where | Whose | Holds |
|---|---|---|---|
| `editor.yml` | The root of the project, next to `CMakeLists.txt` | The team's, committed | The toolchains everyone builds with, and the defaults for the editors |
| `user-editor.yml` | The editor's own folder, `editor://` | One machine's, for every project | What this machine has: where its compilers and editors are, a toolchain of its own |
| `user-editor.yml` | The root of the project, ignored by git | One machine's, for this project | The same, for one project |

The three are read in that order, each on top of the one before, so that
the file nearest to the project wins. A toolchain of the same name replaces
the whole entry underneath, not field by field, so that an entry is read in
one place. A name set to `off` drops the toolchain from what is underneath.
An editor of the same name replaces the path. A file that is not there is
skipped; a file with a mistake adds nothing, and every problem in it is
reported with its line, as with every recipe (see
[recipes.md](recipes.md#made-to-be-changed-by-hand)).

The editor's folder, `editor://`, is machine-wide: organization
`neon-engine`, application `neon-editor`, under the folder the platform keeps
for an application, apart from the project's `user://`, which is the game's.
See [editor.md](editor.md#the-editors-folder) and
[file-systems.md](file-systems.md).

## The file

```yaml
version: 1

toolchains:
  macos-arm64:
    kind: native            # native or docker
    platform: darwin-arm64  # what it builds for, the engine's <os>-<arch> naming
    requires: [cmake, ninja, /opt/homebrew/opt/llvm@20/bin/clang++]
    configure: cmake -S {project} -B {build} --preset macos-arm64-release
    build: cmake --build {build}
    run: "{build}/{name}/{name}"

  linux-x64:
    kind: docker
    platform: linux-x86_64
    image: neon-engine/linux-x64
    requires: [docker]
    configure: cmake -S {project} -B {build} --preset linux-x64-release
    build: cmake --build {build}
    run: off                # cannot run here

editors:
  vscode: /Applications/Visual Studio Code.app/Contents/Resources/app/bin/code
```

| Name | Holds | When it is left out |
|---|---|---|
| `version` | The version of this layout, which is 1, as in every recipe | An error. A file of a later layout is refused with its version |
| `toolchains` | The toolchains, each under its name: a plain name, lowercase letters, digits, and dashes, as a project is named. `off` in place of the map drops the toolchain of that name from the file underneath | No toolchains, as in a user file that only names editors |
| `editors` | Where the editors the editor opens are, by name, as a path or a name on `PATH`. `vscode` is read by Open in VS Code (#574) | None |

A toolchain:

| Name | Holds | When it is left out |
|---|---|---|
| `kind` | `native`, which runs the commands on this machine, or `docker`, which runs them in a container of `image` | An error |
| `platform` | What it builds for, in the engine's naming of a platform: `darwin-arm64`, `linux-x86_64`, `windows-x86_64` | An error |
| `image` | The Docker image of a `docker` toolchain, as `docker build -t` named it | An error for `docker`; not taken for `native` |
| `requires` | The programs that have to be installed: a name that is looked for on `PATH`, or a path that has to be there | Nothing is required |
| `configure`, `build` | The commands, as one would type them | An error |
| `run` | How the result is started, or `off` when it cannot be started on this machine, as a Windows build on a Mac | `off` |

A name that is not known is an error, so that a name that was misspelled
does not go unnoticed. The file is a configuration file, read by the same
reader as every recipe. One thing YAML asks of it: a command that starts
with a placeholder is written in quotes, `run: "{build}/{name}/{name}"`,
since a value that starts with `{` is a map to YAML.

### Placeholders

A command names its paths through placeholders, so that one `editor.yml`
serves every machine of the team and every project of a machine:

| Placeholder | Native toolchain | Docker toolchain |
|---|---|---|
| `{project}` | The folder of the project | `/src`, where the project is mounted |
| `{build}` | `{project}/build/<toolchain>`, the build folder of the project for this toolchain | `/src/build/<toolchain>` |
| `{name}` | The name of the project, from its `project.yml`, which is what the result is called | The same |
| `{engine}` | The checkout of the engine the project is built against, with `cmake/NeonSdk.cmake` and the runtime that was built | `/engine`, where it is mounted |
| `{platform}` | The `platform` of the toolchain | The same |

A brace that is not one of these is an error of the file, reported with
its line. The build folder is `build/<toolchain>` so that it lines up with
`build/<preset>` of the engine's presets when a toolchain is named after
the preset it uses.

### Native and Docker

A native toolchain starts the first word of the command as the program, in
the folder of the project, with the rest as its arguments. The line is
taken apart as a shell would: spaces separate the words, and single or
double quotes hold a word together. No shell runs it, so nothing is
expanded, and the same line means the same on every platform. A shell is
started on purpose by naming it, as `sh -c '...'`.

A Docker toolchain starts the whole line in a shell inside a container of
its image, with the project and the engine mounted:

```sh
docker run --rm -v <project>:/src -v <engine>:/engine -w /src <image> sh -c '<command>'
```

The container is the one of [development.md](development.md#linux-x64),
built once with `docker build`. The Docker toolchains serve the export (#84)
as well: a Mac exports to Linux and Windows through them, and an export to
macOS happens only on macOS. Decided 2026-10-11.

### Detection

When the editor starts it checks every toolchain and shows which are usable:
each name of `requires` has to be found on `PATH` and each path has to be
there, as `ProcessContext::Find` looks for them. A Docker toolchain needs
`docker` whether or not it lists it, and its image is asked of
`docker image inspect <image>`, which ends with 0 when the image is built.
What is missing is shown by the toolchain, one line each, such as `ninja is
not on PATH` or `the Docker image neon-engine/linux-x64 is not built`.

### Building and running

The three commands are independent: configure once, build as often as the
code changes, run what was built last. A build runs in a process of its
own, so the editor stays usable while it runs, and every line the build
prints is handed on as it comes, errors among the rest in the order they
were printed, for the editor's log window.

The result is the game `neon_add_project` puts together, with the runtime
renamed after the project: `{build}/<name>/<name>`, with `engine/`,
`assets/`, and `extensions/` next to it, as [projects.md](projects.md)
describes. The runtime is built once, by the engine; a project's build
compiles its extension alone against it through `cmake/NeonSdk.cmake`.

## The objects

| Type | Does |
|---|---|
| `ToolchainFile` | Reads one file into a `Toolchains`, with every problem and its line |
| `Toolchains` | The toolchains and editors of one file, or of several laid over each other with `Apply()` |
| `Toolchain` | One entry |
| `ToolchainPlaceholders` | What the placeholders stand for on a target, and the filling in |
| `ToolchainCheck` | Whether a toolchain is usable on this machine, and what is missing |
| `ToolchainRun` | Runs one step, configure, build, or run, as a process of its own |
| `BuildTarget` | The project the editor has open, its name, and the engine it is built against |
| `ProcessContext` | The interface of neon-core that starts a program and reads what it prints, with `OS_ProcessRunner` of neon-platform behind it on `posix_spawn` and `CreateProcess` |

The files are read through `FileSystemContext` by virtual path, and the
commands hold native paths, which are the machine's own and go to the
`ProcessContext` and nowhere else.

## Open questions

- **A toolchain that builds the engine itself**, for someone who works on
  the engine: the commands can say so already, with `{engine}` as the
  source, but nothing marks such a toolchain apart.
- **`--platform linux/amd64`** for the Docker images on an Apple Silicon
  Mac, see [development.md](development.md#linux-x64): an image built with
  it runs with it by itself; whether a toolchain should say it is open.
