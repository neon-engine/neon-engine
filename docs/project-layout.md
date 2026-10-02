# The layout of a project

How a Neon project is organised: where files go, how they are named, and why.
It is to a project what the [style guide](style-guide.md) is to the engine's
code, and it is what the editor (#82), the exporter (#84), and agents (#85)
rely on. The project of the runtime,
[app/NeonRuntime/assets](../app/NeonRuntime/assets), follows it and is the
example.

## The folders

A project is a folder with a `project.yml` at its root, see
[projects.md](projects.md). Everything else is sorted by what kind of thing it
is, not by where in the game it is used, so that a tool knows where to look
and a file is found by its name alone.

| Folder | Holds | Kind of file |
|---|---|---|
| `project.yml` | What the project is: name, organization, scenes, entry scene | [projects.md](projects.md) |
| `settings.yml` | What the project chooses for itself and a player may change | [settings.md](settings.md) |
| `CREDITS.md` | Where every third-party file came from, and its licence | See [credits](#credits-and-licences) |
| `scenes/` | The scenes, `*.scene.yml` | [scenes.md](scenes.md) |
| `prefabs/` | Entities described once and placed in scenes, `*.prefab.yml` (#146) | Not there yet |
| `ui/` | User interfaces, `*.ui.yml`, their style sheets `*.css`, their atlases `*.atlas.yml`, and the images they show | [user-interface.md](user-interface.md) |
| `models/` | Meshes, `*.obj` today, glTF once models are prepared ahead of time (#98) | |
| `textures/` | Images that materials show, `*.png` and `*.jpg` | |
| `sounds/` | Sounds and music, `*.wav`, in folders by their use: `music/`, `ambience/`, and the sounds of things next to the top | [audio.md](audio.md) |
| `fonts/` | Fonts, `*.ttf`, one folder per family with its licence | |
| `shaders/` | Compiled shaders, `*.spv`, that materials name without an extension | Made by the build, see below |
| `scripts/` | Game code, once Lua is there (#57) | Not there yet |
| `input/` | Input maps (#117) and input scripts, `*.input` | Not there yet |

A folder that has nothing in it does not exist; there are no placeholder
files. A folder is added when its first file is.

### Where the compiled shaders come from

`shaders/` is not written by hand. The build compiles the GLSL sources of the
engine (`app/NeonRuntime/shaders/vulkan/`) to SPIR-V and puts the result next
to the other assets, so that `assets://shaders/basic-lit` names a shader the
same way `assets://models/cube.obj` names a model. A project that writes
shaders of its own will keep the sources in a folder of the project and the
build puts them in the same place; where that folder is comes with the first
such shader. What ships is the compiled form only.

## Names

| Rule | Example | Why |
|---|---|---|
| Lowercase letters, digits, and `-` between words | `settings-demo.scene.yml`, `reactor-room.wav` | A name has to be the same file on macOS, Linux, and Windows. Lowercase cannot be miscased; `-` is one separator for everything, as in the engine's code |
| The kind of file is a second extension before the format | `.scene.yml`, `.prefab.yml`, `.ui.yml`, `.atlas.yml` | The format says how to parse it, the kind says what it is. A tool filters by the kind, and an editor opens the right one |
| Virtual paths with forward slashes, from a scheme | `assets://scenes/demo.scene.yml` | The [path rules](development.md#path-rules) refuse anything else, on every platform |
| Images of several densities carry the density | `gem-1x.png`, `gem-2x.png`, `gem-4x.png` | The user interface picks the one for the display, see [density](user-interface.md#points-pixels-and-density) |
| A third-party file keeps the name it was published under | `Inter-Regular.ttf`, `NotoSansArabic-Regular.ttf` | It ties the file to its source and its licence. Everything else about it, the folder it is in, follows the rules |
| No spaces, parentheses, or other punctuation | | They need quoting in every shell and build script, and broke the build once (#155) |

The letter case of a name has to match the file on disk, on every platform.
The file system checks it, so that a scene written on macOS, which does not
care, does not break on Linux, which does.

## What is written by hand, and what a tool makes

| Written by hand | Made by a tool |
|---|---|
| `project.yml`, `settings.yml`, `CREDITS.md` | `shaders/*.spv`, by the build |
| `*.scene.yml`, `*.prefab.yml`, `*.ui.yml`, `*.css`, `*.atlas.yml` | Models prepared from what assimp reads (#98), by the editor |
| The sources of images, models, and sounds, as the artist saved them | Packed assets (#81) and baked light maps (#63), by the exporter |

A file a tool makes is not edited by hand and is not committed when the build
makes it from something that is. The compiled shaders are such a file; the
project of the runtime has them only in its build folder. What the editor
makes once from a source that is not kept, such as a model converted from a
format the runtime does not read, is committed, since it cannot be made
again.

## Version control

| In version control | Not in version control |
|---|---|
| Everything written by hand, and every asset the game needs | What the build makes: `shaders/*.spv`, the build folder |
| Binary assets through Git LFS, by kind: images, models, sounds, fonts (see [.gitattributes](../.gitattributes)) | `.DS_Store`, `Icon\r`, and the other files a desktop leaves behind (see [.gitignore](../.gitignore)) |
| The licence of each third-party pack, next to it | What a licence forbids to pass on, once the project is shared beyond who holds the licence |

## Credits and licences

`CREDITS.md` at the root names every third-party file or pack: where it came
from, who made it, and under which licence, with a link. A pack that comes
with its own licence file keeps it in its folder (`fonts/inter/LICENSE.txt`),
and `CREDITS.md` points at it. Files made for the project are listed too, as
under the project's own licence, so that nothing in the folder is of unknown
origin.

A file whose licence does not allow passing it on, such as music licensed to
one person, is marked so in `CREDITS.md`, with what may be done instead. See
the runtime's [CREDITS.md](../app/NeonRuntime/assets/CREDITS.md).

## When a project grows

The folders above sort by kind. A large project sorts inside them by area of
the game, one level down: `scenes/forest/clearing.scene.yml`,
`sounds/music/forest-day.wav`, `textures/forest/bark.png`. The kind stays at
the top, so that a tool still knows where to look, and the area is a folder
inside, so that what belongs together is together. Not the other way round:
`forest/scenes/` would spread one kind over the whole project.

## Open questions

- **Prefabs** (#146) and **scripts** (#57) get their folders with their first
  files; their names follow this guide.
- **Shaders of a project**: where their sources go, and whether a project
  without an engine build can compile them, is decided with the compiler
  tools of the editor (#83).
- **One folder per scene** for what only that scene uses is a pattern other
  engines offer. It is not adopted: it spreads kinds over the project, and a
  scene that grows into two shares its files anyway.
