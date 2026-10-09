# Recipes

This note records what a recipe is, what every recipe has in common, and what
is still open. What one kind of recipe holds is in the note of that kind, see
[the table below](#which-doc-describes-which-recipe).

**Current decision:** a YAML file whose name says its kind, as
`demo.scene.yml` and `hud.ui.yml` do, is a **recipe**: a scene, a user
interface, an atlas, an input map, a prefab, later geometry, entities,
anything the engine turns into something live. A YAML file without a kind,
`project.yml` and `settings.yml`, is a **configuration file**: it says what
the project is and how it is set up. The name decides, nothing else: one
reader reads both with one set of rules, so the rules below hold for the
configuration files as well. A recipe is made to be read and changed by hand
and by agents, and the editor writes the same file. The classes in code keep
their names, `SceneFile`, `ProjectFile`, `UiDocument`; the words are for the
documentation and for talking about the files.

## The kinds

| Kind | File | What it describes | Note |
|---|---|---|---|
| Scene recipe | `*.scene.yml` | Entities and their components | [scenes.md](scenes.md) |
| UI recipe | `*.ui.yml` | The elements of a menu or of what is shown during play | [user-interface.md](user-interface.md) |
| Atlas recipe | `*.atlas.yml` | An image that holds several, and where each one is | [user-interface.md](user-interface.md#atlases) |
| Input recipe | `*.input.yml` | The actions a game reads, what is bound to each, and the states they are live in | Coming with #117 |
| Prefab recipe | `*.prefab.yml` | An entity, or a tree of them, described once and placed in scenes | [prefabs.md](prefabs.md) |

A recipe is named `<name>.<kind>.yml`: lowercase letters, digits, and dashes,
the kind as a second extension before the format, as in
`reactor-room.scene.yml`. The format says how to parse it, the kind says what
it is, and a tool filters by the kind. The folder it goes in is by its kind,
`scenes/`, `ui/`, `prefabs/`, `input/`. See
[project-layout.md](project-layout.md).

The configuration files, which are not recipes:

| File | What it holds | Note |
|---|---|---|
| `project.yml` | What the project is: its name, who makes it, its scenes | [projects.md](projects.md) |
| `settings.yml` | What the project chooses for itself and a player may change | [settings.md](settings.md) |

## Made to be changed by hand

These rules hold for every recipe, and for the two configuration files. A
note of one kind lists only what is its own on top of them.

| Decision | Reason |
|---|---|
| A name that is not known is an error | `postion` would otherwise be ignored, and the entity would sit at the origin without a word. The message is `'postion' is not known to Transform of entity 'a'. Known are: position, rotation, scale` |
| A problem does not end the game | What could be read is used, the log says what could not with its line, and the run's exit code says that something was wrong (see the [development guide](development.md#command-line)). A game is not ended by a file (#179) |
| A message names the file, the line, and what was expected | `demo.scene.yml:5: 'position' of Transform of entity 'a' is text, where a list of 3 numbers was expected` |
| Every problem of a file is reported, not only the first | A file is corrected in one pass |
| What is left out keeps its default | A file holds what was decided and nothing else |
| Paths need no quotes | `assets://models/cube.obj` is plain text to YAML |
| Comments are allowed | YAML has them. They are lost when the engine writes the file, see the open questions |
| Anchors, aliases, and tags are refused | They make a file harder to follow, and harder for a tool to write back as it was. The messages are `anchors and aliases are not supported` and `tags are not supported` |
| Names are written the way they are in code | `Transform` in a file is `Transform` in code. A property of a user interface is as in CSS, with an underscore where CSS has a hyphen |
| `version: 1` at the top of every file | The version of the layout of the file, not of its content. See below |

### The version

Every recipe, and `project.yml` and `settings.yml` too, carries `version`.
It is the version of the layout: the names the file may hold and what they
mean. It is 1 today. When a layout changes in a way that an older engine
cannot read, the number goes up, and an engine that reads a file of a later
layout refuses it with a message that names both: `the scene has version 2,
and this engine reads up to version 1`. A file of an older layout is read as
far as the engine still knows it. The number is what lets a tool convert a
file from one layout to the next.

| In | When `version` is left out |
|---|---|
| A scene, a UI, an atlas | It counts as 1 |
| `project.yml`, `settings.yml` | An error: `'version' is missing. It holds the version of the layout, which is 1` |

## Numbers

A number is written as a person writes it: digits, a sign, a dot, an
exponent. `29`, `-0.5`, `1e3`. The field decides the type: the engine knows
through reflection whether a field is an `int`, a `float`, or a `double`,
reads a plain number in that type, and the file never decides how a number
is stored.

| Written | Read as |
|---|---|
| `29`, `-0.5`, `1e3` | A number, in the type of the field |
| `0.5f` | A number with a fraction that says it is a float, for the person reading the file |
| `2.0d` | A number with a fraction that says it is a double, for the person reading the file |
| `2d`, `2f` | Text. A whole number takes no suffix |
| `0x1d`, `0o17` | Text. Hexadecimal and octal are the computer's forms and are never a number by surprise. A field that wants such a form, such as a color, reads the text |
| `"1.3"` | Text, since it is quoted. `rendering.vulkan_version` wants it so, since `1.10` as a number is `1.1` |

A suffix is for the reader and changes nothing about how the engine stores
the number, except in one case: `2.0d` into a `float` field is refused, since
it would lose the precision that was asked for. `0.5f` into a `double` field
is fine. The engine never writes a suffix; it knows the type of what it
writes, and writes a number with the digits that type needs, `0.31` and not
`0.3100000023841858`.

| What went wrong | Message |
|---|---|
| Text, a list, or a map where a number was wanted | `'fov' of Camera of entity 'camera' is text, where a number was expected` |
| A fraction, or a number too large, where an `int` field reads | `'points' of Health of entity 'a' is 1.5, where a whole number was expected` |
| `2.0d` into a `float` field | `'fov' of Camera of entity 'camera' is written as a double, with d, where a float is read and would lose that precision. Write it without the suffix, or with f` |
| `2.0d` in a list of floats, such as a `position` | `'position' of Transform of entity 'a' holds a number written as a double, with d, where floats are read and would lose that precision. Write it without the suffix, or with f` |

## Which doc describes which recipe

| Recipe | What its note holds |
|---|---|
| [scenes.md](scenes.md) | The scene recipe: entities, children, and every component with its names and defaults; what the engine writes |
| [user-interface.md](user-interface.md) | The UI recipe: elements, properties, states, style sheets, and the atlas recipe under [atlases](user-interface.md#atlases) |
| [projects.md](projects.md) | The configuration file `project.yml` |
| [settings.md](settings.md) | The configuration file `settings.yml` |
| Input | Coming with #117, `docs/input.md` |
| [prefabs.md](prefabs.md) | The prefab recipe: an entity described once, how a scene places it, and what it may write on top |

## How it is built

| Piece | Location | Role |
|---|---|---|
| `DataValue` | neon-core, `neon/data/data-value.hpp` | A value of a document: a bool, a number, text, a list, a map, or nothing. It knows the line it was read from, and whether a number was written with `f` or `d` |
| `DocumentFormat` | neon-core | The interface that turns text into values and back |
| `RYML_DocumentFormat` | neon-ryml, `neon/data/ryml-document-format.cpp` | The implementation for YAML, with rapidyaml. It is where a scalar becomes a number or text, and where anchors, aliases, and tags are refused |
| `DataReader` | neon-core, `neon/data/data-reader.cpp` | Reads the values of a map into variables, in the type of each, and collects what is wrong, the names that are not known included |

Every reader of a recipe, and `ProjectFile` and `SettingsFile`, is built on
these and knows nothing of YAML. A second format, binary or otherwise, is a
second `DocumentFormat`, see [scenes.md](scenes.md#a-binary-form).

## Open questions

- **A binary form** (#96): a recipe stays YAML while a game is made, and is
  turned into a binary form when the game is exported, as a second
  `DocumentFormat`. Not needed yet; where it goes is in
  [scenes.md](scenes.md#a-binary-form).
- **Whether `project.yml` and `settings.yml` get a word of their own.** They
  follow every rule above and are read by the same reader, but describe the
  project and not the game. "Recipe" is kept for what is in the game.
- Comments and the order of names are lost when the engine writes a file that
  was changed by hand. Keeping them needs the writer to work on the text that
  is there and not on values.
