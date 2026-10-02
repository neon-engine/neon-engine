# Scenes

This note records how a scene is kept in a file, why it is kept that way, and
what is still open.

**Current decision:** a scene is a recipe, a YAML file that lists entities
and their components, see [recipes.md](recipes.md). It is meant to be read
and changed by hand, under the rules every recipe shares. NeonEditor will
write the same file. A binary form is not needed yet, and has a place to go
when it is.

## The file

```yaml
scene: demo
version: 1

entities:
  - name: floor
    components:
      Transform:
        position: [0, -0.55, 0]
        scale: [100, 0.1, 100]
      Renderable:
        model: assets://models/cube.obj
        shader: assets://shaders/basic-lit
        textures:
          - assets://textures/concrete.png
        material:
          color: [0.5, 0.5, 0.5]

  - name: player
    components:
      Transform:
        position: [0, 0, 2]
      Spectator: Default
    children:
      # the camera sits on the player, and so moves and turns with it
      - name: camera
        components:
          Transform: Default
          Camera: Default
```

The runtime starts with the entry scene of its project, see
[projects.md](projects.md), which is
[demo.scene.yml](../app/NeonRuntime/assets/scenes/demo.scene.yml). Another one
is chosen with `--scene`:

```
NeonRuntime --scene assets://scenes/other.scene.yml
```

### At the top

| Name | Holds | When it is left out |
|---|---|---|
| `scene` | What the scene is called | It has no name |
| `version` | The version of this layout, which is 1 | It counts as 1 |
| `entities` | A list of entities | The scene is empty |

### An entity

| Name | Holds | When it is left out |
|---|---|---|
| `name` | What the entity is called. Unique among the entities next to it, and without a `/` | It has no name and cannot be found by one |
| `components` | The components, each under the name it was registered with | It carries none |
| `children` | A list of entities below it | It has none |

A child moves, turns, and grows with its parent. It is found by the names from
the top, such as `player/camera`.

### Components

What a component leaves out keeps its default. A component that keeps all of
its defaults is written as `Default`, as in `Spectator: Default`. `{}` means
the same.

**Transform**

| Name | Holds | Default |
|---|---|---|
| `position` | `[x, y, z]`, relative to the parent | `[0, 0, 0]` |
| `rotation` | `[pitch, yaw, roll]` in degrees | `[0, 0, 0]` |
| `scale` | `[x, y, z]`, or one number for all three | `1` |

**Renderable**

| Name | Holds | Default |
|---|---|---|
| `model` | Virtual path of the model | None. It has to be written |
| `shader` | Virtual path of the shader, without an extension | None. It has to be written |
| `textures` | A list of virtual paths | None |
| `scale_textures` | Whether textures repeat as the entity grows | `false` |
| `material` | `color` as `[red, green, blue]` or with alpha as a fourth, `metallic` and `roughness` from 0 to 1 (the `pbr` shader), `shininess` (the `basic-lit` shader), `use_textures`, `alpha_mode`, and `double_sided` | white, `0`, `0.5`, `0`, `true`, `opaque`, `false` |

`color` is written as a screen shows it, in sRGB, like the colours of an
image. The first texture holds colours, the second how much each part of a
surface shines. `alpha_mode` is `opaque`, which covers what is behind, or
`blend`, where the alpha of the colour, or of the texture, lets what is
behind show through. See-through surfaces are drawn after the opaque ones,
from the farthest to the nearest.

`model` is an `.obj` or a `.glb`. What the file says about its look fills in
what the `Renderable` leaves out: the textures of the model are shown when
`textures` is not written, and the base colour factor of a glTF material is
multiplied into `color`. Every model is scaled so that its longest side is 1,
so `scale` gives a piece of a kit its size back. See [models.md](models.md).

The back of every triangle is left out, since most surfaces are seen from
one side only: the outside of a closed model. The front is the side whose
corners go round anticlockwise, as models are made. `double_sided: true`
draws the back as well, for a leaf, a flag, or a pane of glass that is seen
from both sides. As `doubleSided` in the materials of glTF. An entity whose
`scale` mirrors it, with one or three of its axes below 0, is still drawn
from the front.

**Camera**

| Name | Holds | Default |
|---|---|---|
| `target` | `window` or `texture` | `window` |
| `fov` | Vertical field of view in degrees | `45` |
| `near`, `far` | The distances between which things are visible | `0.1`, `1000` |
| `up` | The direction that is up | `[0, 1, 0]` |

**Light**

| Name | Holds | Default |
|---|---|---|
| `type` | `direction`, `point`, or `spot` | `direction` |
| `direction` | `[x, y, z]` | `[0, 0, 0]` |
| `ambient`, `diffuse`, `specular` | `[red, green, blue]`, amounts of light: 0.5 is half the light of 1. For the `pbr` shader `diffuse` is the light's colour and `specular` is not read, see [vulkan-renderer.md](vulkan-renderer.md#the-shaders-that-ship) | `[0, 0, 0]` |
| `constant`, `linear`, `quadratic` | How the light fades with distance | `0` |
| `cutoff`, `outer_cutoff` | The cone of a spot light | `0` |

The position of a light is that of its `Transform`. The renderer knows a
light by the name of its entity.

**Spectator**

| Name | Holds | Default |
|---|---|---|
| `move_speed` | Units per second | `2.5` |
| `look_speed` | Degrees per unit the mouse moved | `0.1` |

**SoundSource** and **SoundListener** are listed in [audio.md](audio.md).

**RigidBody**, **Collider**, **Trigger**, **CharacterBody**, and **Joint**
are the components of the physics. What they hold is listed in
[physics.md](physics.md#components).

```yaml
- name: crate
  components:
    Transform:
      position: [0, 5, 0]
    RigidBody:
      kind: dynamic        # static, kinematic, or dynamic
      mass: 10
    Collider:
      shape: box           # box, sphere, capsule, cylinder, tapered_capsule,
      size: [1, 1, 1]      # tapered_cylinder, plane, convex_hull, or mesh

- name: door-trigger
  components:
    Transform:
      position: [0, 1, -5]
    Trigger:
      mask: [1, 2]         # the layers it looks for, from 1 to 32
    Collider:
      shape: sphere
      radius: 2

- name: walker
  components:
    Transform:
      position: [-6, 1, 4]
    CharacterBody:
      velocity: [2.5, 0, 0.5]
    Collider:
      shape: capsule
      radius: 0.4
      height: 1.8
```

| Component | Short | Names |
|---|---|---|
| `RigidBody` | A body that does not move, is moved by code, or is moved by the simulation | `kind`, `mass`, `friction`, `bounce`, `linear_damping`, `angular_damping`, `gravity_scale`, `linear_velocity`, `angular_velocity`, `continuous`, `can_sleep`, `layers`, `mask` |
| `Collider` | The shape of the body of its entity, or of the nearest entity above it that has one | `shape`, `offset`, `rotation`, and what belongs to the shape: `size`, `radius`, `height`, `top_radius`, `bottom_radius`, `model` |
| `Trigger` | An area that reports what enters and leaves it | `layers`, `mask` |
| `CharacterBody` | Something that is moved by a velocity, stops at what is in its way, and slides along it | `velocity`, `fall_velocity`, `gravity_scale`, `max_slope`, `step_height`, `mass`, `push_strength`, `layers`, `mask` |
| `Joint` | Holds the body of its entity to another body, named by its path, or to the world | `type`, `other`, `anchor`, and for a hinge and a slider: `axis`, `limits` |

## Made to be changed by hand

The rules every recipe shares, that a name that is not known is an error,
that every problem is reported with its line, that what is left out keeps its
default, how numbers and their precision are written, and that anchors,
aliases, and tags are refused, are in
[recipes.md](recipes.md#made-to-be-changed-by-hand). On top of them:

| Decision | Reason |
|---|---|
| A message names the component and the entity | `demo.scene.yml:5: 'position' of Transform of entity 'a' is text, where a list of 3 numbers was expected` |
| A component with all of its defaults is `Default`, or `{}` | `Spectator: Default` says the component is there without listing anything |
| The name of an entity is unique next to its siblings, and has no `/` | `player/camera` finds a child by the names from the top |
| `scale` takes one number for all three axes | `scale: 2` is `[2, 2, 2]` |
| Components are written under the name they were registered with | `Transform` in a file is `Transform` in code, see [reflection.md](reflection.md) |

## When the engine writes

`SceneFile::Save` writes every entity of the store. It is what NeonEditor will
use.

| Rule | Reason |
|---|---|
| A value that is the default is left out | The file stays short and a change to it shows up as one line |
| Lists of numbers are written on one line | `[0, 0, 2]` |
| Entities are set apart by a blank line | They are found at a glance |
| Entities keep the order they were created in | The file does not reorder itself between two saves |
| A number is written with the digits it needs | `0.31`, not `0.3100000023841858` |

A scene that was saved, loaded, and saved again is the same text.

## How it is built

| Piece | Location | Role |
|---|---|---|
| `DataValue` | neon-core | A value of a document: a bool, a number, text, a list, a map, or nothing. It knows the line it was read from |
| `DocumentFormat` | neon-core | The interface that turns text into values and back |
| `RYML_DocumentFormat` | neon-ryml | The implementation for YAML, with rapidyaml |
| `DataReader` | neon-core | Reads the values of a map into variables and collects what is wrong |
| `ComponentFormat` | neon-core | How one kind of component is read and written. It follows from the description of the component |
| `SceneFile` | neon-core | The `Scene` that reads and writes a file |

Nothing outside neon-ryml includes a header of rapidyaml. The library is
linked privately. neon-core linked it before and no longer does.

`SceneFile` does not know that the file is YAML. It is handed a
`DocumentFormat` and reads the file through the file system.

### A component of a game

A component is described next to itself, and its format follows from the
description. See [reflection.md](reflection.md).

```cpp
struct Health
{
  int points = 100;
};

inline void Describe(neon::TypeBuilder<Health> &type)
{
  type.Named("Health");
  type.Field("points", &Health::points).AtLeast(0);
}

store.Register<Health>("Health");
scene.GetComponentFormats().Add(neon::ComponentFormat::Of<Health>());
```

## A binary form

YAML has no binary form of its own. The formats that are close are separate
formats with the same kinds of values.

| Format | What it is |
|---|---|
| [MessagePack](https://msgpack.org) | Maps, lists, numbers, and text in a compact binary form. Small libraries for every language |
| [CBOR](https://cbor.io) | The same idea as a standard, RFC 8949 |
| [FlatBuffers](https://flatbuffers.dev) | Read without being parsed. Needs a schema that is compiled ahead of time |
| A format of our own | The memory of the components as it is, which is the fastest to load and the most work to keep compatible |

None is needed today. The text of a scene is small next to the models and
textures it names, and those are what loading waits for. How long a large
scene takes to read has not been measured.

When one is needed, for a scene with many thousands of entities or to keep a
shipped game from being changed with a text editor, it is a second
`DocumentFormat`. The scene would be kept as YAML while a game is made, and
turned into the binary form when the game is exported. `SceneFile` and the
formats of the components would not change.

## How it was checked

| Check | Result |
|---|---|
| The demo scene from the file, and as it was written in code | The same image, byte for byte |
| 23 checks of reading and writing YAML through the interface | Pass |
| 30 checks of loading, saving, and what is reported for a wrong file | Pass |

The checks are not part of the repository. They move into the unit tests once
those exist.

## Open questions

- Comments and the order of names are lost when the engine writes a file that
  was changed by hand, see [recipes.md](recipes.md#open-questions).
- One scene inside another, such as a tree that is placed a hundred times.
  Flecs has prefabs for it.
- An entity that refers to another, such as a door to its switch. Paths such
  as `player/camera` are the likely answer.
