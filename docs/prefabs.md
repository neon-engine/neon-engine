# Prefabs

This note records how an entity that a game needs many times is described
once, how a scene places it and changes what differs, why it works that way,
and what is still open.

**Current decision:** a prefab is a recipe, a YAML file named
`<name>.prefab.yml` in `prefabs/`, that describes one entity with its
components and the entities below it, see [recipes.md](recipes.md). A scene
places it with `prefab:` next to the name of an entity, and writes next to
it only what differs. The loader reads the prefab's entity first and the
scene's values on top: a component changes only the values the scene
writes, a child of the same name is changed and a new name is added, and
`~` takes a component away. Every entity that was placed from a prefab
carries a `Prefab` component that names the file. A prefab may place
prefabs; a loop is refused.

## The file

```yaml
version: 1
prefab: wall

entity:
  components:
    Renderable:
      model: assets://external/kenney/prototype-kit/wall.glb
      shader: assets://shaders/basic-lit
    RigidBody:
      kind: static
    Collider:
      shape: box
      size: [0.2, 1, 1]
      offset: [0, 0.5, 0]
```

| Name | Holds | When it is left out |
|---|---|---|
| `version` | The version of this layout, which is 1 | It counts as 1 |
| `prefab` | What the prefab is called, for the people reading the file | It has no name |
| `entity` | The entity the prefab describes: its `prefab`, `components`, and `children`, as [an entity of a scene](scenes.md#an-entity) has them | An error: the file describes nothing |

The entity has no `name` of its own. It is called what the scene calls it
where it is placed, so one prefab is `north-wall` here and `south-wall`
there. `name` under `entity` is reported as a name that is not known. The
children keep their names, as they are what a scene changes them by.

## Placing

```yaml
entities:
  - name: north-wall
    prefab: assets://prefabs/wall.prefab.yml
    components:
      Transform:
        position: [0, 0, -3]
        rotation: [0, 90, 0]
        scale: [1, 2.5, 6]

  - name: south-wall
    prefab: assets://prefabs/wall.prefab.yml
    components:
      Transform:
        position: [0, 0, 3]
        rotation: [0, 90, 0]
        scale: [1, 2.5, 6]
```

| Name | Holds | When it is left out |
|---|---|---|
| `prefab` | The virtual path of the prefab recipe | The entity is described in the scene alone |

The entity starts as the entity of the prefab: the prefab's own `prefab`
first, then its components, then its children. What is written next to
`prefab:` in the scene goes on top, in the same order. An entity that has a
`prefab` carries a `Prefab` component afterwards, with the path, so that
the editor knows which file a change belongs to and a script can tell a
wall from a wall. The component is not written under `components`;
`prefab:` is what sets it.

The prototype of the runtime,
[prototype.scene.yml](../app/NeonRuntime/assets/scenes/prototype.scene.yml),
places the walls, the floors, the columns, and the targets of its level from
[assets/prefabs](../app/NeonRuntime/assets/prefabs), each with only its
`Transform` written on top. The image it draws is the one it drew before, to
the byte.

## What differs

| Written on top | What happens | Reason |
|---|---|---|
| A component the prefab has, with some of its values | The values that are written change, the rest stay as the prefab says | A scene writes what is its own, as it leaves out what is the default |
| A group of a component, such as `material` | The same, value by value | A group is a part of the component, not a value of its own |
| A list, such as `textures` | The scene's list replaces the prefab's whole | A list is one value. There is no way to say which item to change, and writing the whole list is what a reader expects |
| `Default` or `{}` | Nothing changes | They say that nothing is written. To reset a value, write it |
| `Component: ~` | The component is taken away | Nothing, written as `~`, is what the entity has of it. Taking away what is not there is fine |
| A component the prefab lacks | It is added, with its defaults and what is written | As it would be on any entity |
| A child with the name of one of the prefab's | That child is changed by the same rules, all the way down | The name is what a child is found by, in a path such as `room/north-wall` |
| A child with a new name | It is added after the prefab's children | A prefab is a start, not a limit |
| A name twice in one list of children | The second is reported, as everywhere | It would change the first, which is never what was meant |
| A name that is not known, anywhere | Reported with the file and the line it is in, the prefab's or the scene's | The rule of every recipe. A prefab's problems are said the first time it is placed, not once for every wall of a room |

A child of a prefab cannot be taken away yet, see the open questions.

## Prefabs within prefabs

A prefab's entity, and every child of it, may carry `prefab:` as an entity
of a scene does. A `room.prefab.yml` places four walls from
`wall.prefab.yml`, and a `tall-wall.prefab.yml` starts from
`wall.prefab.yml` and writes a taller scale on top. The entity it is placed
as carries the outermost prefab as its `Prefab`, and each child placed from
a prefab carries its own.

A prefab that places itself, through others or not, is a loop. It is
reported where it closes, with every file and line of the chain:

```
assets://prefabs/q.prefab.yml:6: 'prefab' of entity 'p' places assets://prefabs/p.prefab.yml within itself: assets://scenes/demo.scene.yml:3 places assets://prefabs/p.prefab.yml, assets://prefabs/p.prefab.yml:4 places assets://prefabs/q.prefab.yml, assets://prefabs/q.prefab.yml:6 places assets://prefabs/p.prefab.yml again
```

What is above the loop is placed; the entity that would close it stays
empty.

## What is wrong with a file

| What went wrong | Message |
|---|---|
| The prefab is not there, or is not YAML | `demo.scene.yml:5: 'prefab' of entity 'a' is assets://prefabs/nope.prefab.yml, which cannot be read`, after the reason with its own line where there is one |
| `entity` is left out | `wall.prefab.yml:1: 'entity' is missing. It holds the entity the prefab describes` |
| A name at the top is not known | `wall.prefab.yml:2: 'entities' is not known to the prefab. Known are: prefab, version, entity` |
| A name inside the prefab is not known | `assets://prefabs/wall.prefab.yml:7: 'postion' is not known to Transform of entity 'north-wall'. Known are: position, rotation, scale`, the entity named as the scene names it |
| A version this engine does not read | `wall.prefab.yml:1: the prefab has version 2, and this engine reads up to version 1` |

The scene goes on with what could be read, and the exit code says that not
everything could, as with every recipe.

## When the engine writes

`SceneFile::Save` writes an entity that was placed from a prefab with its
`prefab:` next to its name, and with every component it has, not only what
differs from the prefab. Loading the file gives the same world: the
components land on top of the prefab's and say the same. A component that
was taken away with `~` is not written, so the prefab gives it back on the
next load; writing back only what differs is open, see below.

## How it is built

| Piece | Location | Role |
|---|---|---|
| `PrefabFile` | neon-core, `neon/world-system/ecs/scene-file/prefab-file.cpp` | Reads one prefab recipe into a tree of values, through the file system and the `DocumentFormat`, and keeps it |
| `PrefabFiles` | neon-core, `neon/world-system/ecs/scene-file/prefab-files.cpp` | The prefabs of one scene load, read once each, and which are being placed, for the loop check |
| `Prefab` | neon-core, `neon/world-system/ecs/components/prefab.hpp` | The component that names the file an entity was placed from. The loader registers it |
| `SceneFile::ReadOnto` | neon-core, `scene-file.cpp` | Reads what is written for an entity onto it: its prefab, its components, its children. The prefab's entity goes through the same reader, which is what lets a prefab place prefabs |
| `ComponentFormat::StartFrom` | neon-core, `component-format.hpp` | The component a read starts from: the one the entity has, or the defaults. It is what makes the scene's values land on top |

The prefab's entity is kept as values and read onto every entity it is
placed as, through the formats of the components, so that a prefab is read
exactly as a scene is and reports its problems the same way. Nothing is
copied between entities. The prefabs are kept for one load alone, so that a
file that changed is read anew the next time the scene loads.

## How it was checked

| Check | Result |
|---|---|
| The prototype of the runtime, with its walls, floors, columns, and targets placed from prefabs | The same two frames as before, byte for byte |
| 28 checks of reading, placing, changing, nesting, saving, and what is reported for a wrong file, in `tests/prefab-files` | Pass |
| 3 checks that a component is read into the one the entity has, next to the formats | Pass |

## Open questions

- **Taking a child away.** `~` takes a component away; there is no way yet
  to say that a child of the prefab is not wanted. A child written as
  `- name: shade` with nothing else is the child unchanged. Likely a
  `remove: true` on the child, or a name with a mark, once a scene needs
  it.
- **Spawning at run time.** A prefab is placed when the scene loads. A game
  that fires a bullet or drops a crate needs the same from code and from a
  script, without a scene: `PrefabFiles` and `ReadOnto` are the pieces, and
  what calls them is decided with scripting (#57).
- **Saving back only what differs.** The editor will write the scene with
  the prefab's values left out, as it leaves out the defaults, so that a
  change to the prefab reaches every scene. It needs the prefab's entity at
  hand when the scene is written, and a word for a component that was taken
  away.
- **Flecs `IsA`.** Flecs has prefabs of its own, where an instance shares
  the components of its prefab until it writes to one. Every instance here
  carries its own copy, which is simple and enough for a level. Sharing is
  a change to `EntityStore` behind the same files, when a scene has
  thousands of one thing.
- **A name for the prefab's entity.** The scene names it; the prefab's
  `prefab:` is for the people reading the file. Whether a scene may leave
  the name out and take the prefab's is open.
