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
        shader: engine://shaders/basic-lit
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
[projects.md](projects.md). Another one is chosen with `--scene`:

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
| `prefab` | The virtual path of a prefab recipe the entity starts as, see [prefabs.md](prefabs.md). What is written next to it goes on top | The entity is described here alone |
| `components` | The components, each under the name it was registered with | It carries none |
| `children` | A list of entities below it. Below a prefab, `- shade: ~` takes the child `shade` of the prefab away | It has none |

A child moves, turns, and grows with its parent. It is found by the names from
the top, such as `player/camera`.

```yaml
  - name: north-wall
    prefab: assets://prefabs/wall.prefab.yml
    components:
      Transform:
        position: [0, 0, -3]
```

An entity with a `prefab` has every component and child of the prefab, and
what is written next to it changes only what it names: a component is read
into the one the prefab gave, `~` takes a component or a child away, and a
child of the same name is changed. The rules are in
[prefabs.md](prefabs.md#what-differs). A game places a prefab while it
plays with `WorldSystem::Spawn`, see
[prefabs.md](prefabs.md#spawning-at-run-time).

### Components

What a component leaves out keeps its default. A component that keeps all of
its defaults is written as `Default`, as in `Spectator: Default`. The reader
takes `{}` as well, but `Default` is the spelling of the recipes, since it
reads as a sentence. On an entity placed from a prefab, what is left out
keeps what the prefab says instead.

Any component takes `enabled`, besides its own values. `enabled: false`
turns the component off: it keeps everything that is written for it, and is
not there until it is turned on. An entity whose `Renderable` is off is not
drawn, one whose `Collider` is off is walked through, and one whose
`RigidBody` is off is not in the physics. It is how a recipe writes what
waits to be used, as the instances of a pool do. A script, an extension, and
C++ turn it on, see
[entity-component-system.md](entity-component-system.md#turning-a-component-off).

```yaml
- name: nail
  components:
    Transform: Default
    Renderable:
      enabled: false
      model: assets://models/nail.glb
      shader: engine://shaders/pbr
```

**PoolManager**

Holds instances that are made ahead and handed out, so that what comes and
goes often is never made or destroyed while the game runs: see
[pools.md](pools.md).

| Name | Holds | Default |
|---|---|---|
| `_entries` | A list of what the pool holds, each with a `source`, a prefab as `assets://prefabs/nail.prefab.yml` or an entity that is there as `instance://lamp`, and a `count` | None |

**Transform**

| Name | Holds | Default |
|---|---|---|
| `position` | `[x, y, z]`, relative to the parent | `[0, 0, 0]` |
| `rotation` | `[pitch, yaw, roll]` in degrees | `[0, 0, 0]` |
| `scale` | `[x, y, z]`, or one number for all three | `1` |

**Renderable**

| Name | Holds | Default |
|---|---|---|
| `model` | A virtual path. Left out when the entity has a `Geometry`, which is drawn instead, see [geometry.md](geometry.md) | None |
| `fit` | How the model is sized: `none` draws it at its own size and origin, as the file says; `unit` moves its middle to the origin and scales it so that its longest side is 1 | `none` |
| `shader` | Virtual path of the shader, without an extension | None. It has to be written |
| `textures` | A list of virtual paths | None |
| `preload` | A list of virtual paths of textures the entity will show later in place of its first one: the faces of a character, the skins of what a player holds. Each is loaded when the entity is first drawn and stays loaded for as long as the entity is there, so that showing it the first time costs nothing while the game runs. It changes nothing that is drawn | None |
| `scale_textures` | Whether textures repeat as the entity grows | `false` |
| `material` | `color` as `[red, green, blue]` or with alpha as a fourth, `metallic` and `roughness` from 0 to 1 (the `pbr` shader), `shininess` (the `basic-lit` shader), `use_textures`, `alpha_mode`, `double_sided` as `model`, `always`, or `never`, what the surface gives off: `emissive` as a color, `emissive_strength` from 0 up, and `emissive_texture` as a virtual path, see below; and light that was worked out ahead: `lightmap` as a virtual path and `lightmap_strength` from 0 up, see [vulkan-renderer.md](vulkan-renderer.md#lightmaps) | white, the model file's or `0`, the model file's or `0.5`, `0`, `true`, `opaque`, `model`, black, `1`, none, none, `1` |

`color` is written as a screen shows it, in sRGB, like the colors of an
image. The first texture holds colors, the second how much each part of a
surface shines. `alpha_mode` is `opaque`, which covers what is behind, or
`blend`, where the alpha of the color, or of the texture, lets what is
behind show through. See-through surfaces are drawn after the opaque ones,
from the farthest to the nearest, by where the middle of each thing is.

A surface gives off light of its own with `emissive`: a color, written in
sRGB as `color` is, that the `pbr` and `basic-lit` shaders add after the
lighting, so that it shows in a dark room. `emissive_strength` multiplies it
in linear light, and is how a surface gets brighter than white: a strength
of 4 gives four times the light of its color, which a tonemapper in the
settings then rolls off, see
[vulkan-renderer.md](vulkan-renderer.md#tonemapping). `emissive_texture` is
an image, or `surface://<name>` for a render target such as a screen that
shows a user interface, which the color multiplies; written without an
`emissive`, the texture is shown as it is. `unlit` and `color` ignore all
three. A surface that gives off light does not light anything around it,
and does not glow around its edges: that is #132 and #130.

`model` is an `.obj` or a `.glb`. What the file says about its look fills in
what the `Renderable` leaves out: the textures of the model are shown when
`textures` is not written, the base color factor of a glTF material is
multiplied into `color`, a color painted on the vertices of the model
is multiplied in as well, the metallic and roughness factors of the file
are multiplied into `metallic` and `roughness`, which count as 1 when they
are left out, so that the file's are taken then, and what the file's material gives off is shown
when `emissive` is left black and `emissive_texture` is not written, with
`emissive_strength` multiplied into the file's, so that `0` turns a file's
glow off. A model whose meshes use different materials is drawn with each of
them, and the `Renderable` is the override of the whole: `color`, the
shader, and what else `material` holds apply to every material of the file,
and `textures` replace those of the first material alone, see
[several materials](models.md#several-materials). A model is drawn at its
own size, as the file says:
a piece of a kit is in meters and stands on its origin. A model that is not
in meters is drawn with `fit: unit`, scaled so that its longest side is 1,
and `scale` then gives it a size; the `.obj` models of the demo scenes are
drawn so. See [models.md](models.md).

The back of every triangle is left out, since most surfaces are seen from
one side only: the outside of a closed model. The front is the side whose
corners go round counterclockwise, as models are made. `double_sided: always`
draws the back as well, for a leaf, a flag, or a pane of glass that is seen
from both sides, and `never` leaves it out. `model`, the default, takes
what the model file says, the `doubleSided` of a glTF material, and one
side when the file says nothing or the entity has a `Geometry`. See
[models.md](models.md#doublesided). An entity whose `scale` mirrors it,
with one or three of its axes below 0, is still drawn from the front.

A `Renderable` can be written while the game runs: a script, an extension,
or the editor sets `textures`, `shader`, `model`, or anything of `material`,
and the entity is drawn with what it says from the next frame on. That is
how a face blinks and a button lights up: the entity is told another
texture. The renderer is told once for each write, and an entity that is
never written costs nothing for it, see
[vulkan-renderer.md](vulkan-renderer.md#what-is-shared). A texture that was
shown stays loaded until the scene is over, so going back and forth between
two reads no file and sends nothing to the graphics card. `preload` loads
the ones that will be shown before they are.

**Geometry**

A shape the engine builds in place of a model, in meters: a box, a plane, a
ramp, a prism from an outline, a sphere, a cylinder, an upright quad, or a
tube along a curve, textured once a meter. The entity's
`Renderable` draws it, and a `Collider` without a `model` takes the same
shape. The fields are in [geometry.md](geometry.md).

**Rope**

A rope drawn between two ends that move: straight while they are as far
apart as it is long, hanging in a curve while they are nearer. The entity's
`Renderable` draws it, with its shader and material. It draws the rope of a
`Joint` of type `rope` by naming the entity of that joint, or hangs between
two places of its own, see [physics.md](physics.md#ropes).

| Name | Holds | Default |
|---|---|---|
| `joint` | The path of the entity whose `Joint` of type `rope` is drawn. The ends and the length are then those of the joint | Empty |
| `from`, `to` | The paths of the entities the two ends are on. Empty for the world. Not read with `joint` | Empty |
| `from_anchor`, `to_anchor` | `[x, y, z]`, where each end is on its entity, sized by its `scale`, or in the world | `[0, 0, 0]` |
| `length` | How long the rope is, in meters. `0` for a rope that is always straight. Not read with `joint` | `0` |
| `thickness` | How thick the rope is, across, in meters | `0.03` |
| `sides` | How many faces go round it | `8` |
| `segments` | How many straight pieces it is drawn as from end to end | `16` |
| `texels_per_meter` | How often a texture repeats over one meter of it | `1` |

**Camera**

| Name | Holds | Default |
|---|---|---|
| `target` | `window` or `texture` | `window` |
| `fov` | Vertical field of view in degrees | `45` |
| `near`, `far` | The distances between which things are visible | `0.1`, `1000` |
| `up` | The direction that is up | `[0, 1, 0]` |
| `effects` | A list of shaders that are run over the whole picture the camera drew, one after the other, on the light of its scene before the tonemapper: a view that waves under water, a vignette. Each is the virtual path of a fragment shader a game brings, without an extension, see [vulkan-renderer.md](vulkan-renderer.md#the-effects-of-a-camera) | none |
| `screen_effects` | As `effects`, but run on the colors a screen is given, after the tonemapper and before the user interface is drawn: the rows of an old monitor, the palette of an old game | none |
| `texture`, `size` | For `target: texture`: what the texture is called, and its width and height in pixels, see [user-interface.md](user-interface.md#what-a-camera-sees) | none, `[512, 512]` |
| `mipmaps` | For `target: texture`: the most levels of smaller copies it has, 1 for none, 0 for as many as its size allows | `0` |
| `rolls_with_entity` | Whether the view rolls with the rotation of its entity: `up` then turns with the entity, so leaning the entity leans the view. For a view from the eyes. A camera that looks at something from outside leaves it off and stays level whatever it hangs from | `false` |

**Light**

| Name | Holds | Default |
|---|---|---|
| `type` | `direction`, `point`, or `spot` | `direction` |
| `direction` | `[x, y, z]` | `[0, 0, 0]` |
| `ambient`, `diffuse`, `specular` | `[red, green, blue]`, amounts of light: 0.5 is half the light of 1. For the `pbr` shader `diffuse` is the light's color and `specular` is not read, see [vulkan-renderer.md](vulkan-renderer.md#the-shaders-that-ship) | `[0, 0, 0]` |
| `constant`, `linear`, `quadratic` | How a point or a spot light fades with distance `d`: its light is divided by `constant + linear * d + quadratic * d * d`, so a light writes a `constant` of 1 | `0` |
| `cutoff`, `outer_cutoff` | The cone of a spot light, as the cosines of two angles from its `direction`: the light is full within the first and fades out to the second. 20 and 30 degrees are `0.9397` and `0.866`. Degrees are #342 | `0` |
| `casts_shadows` | Whether what stands in the light shadows what is behind it. A `direction` light draws a shadow map, see [vulkan-renderer.md](vulkan-renderer.md#shadows); a `point` or `spot` light casts nothing yet, whatever this says | `true` |

The position of a light is that of its `Transform`. The renderer knows a
light by the name of its entity.

**Sky**

What every camera sees behind everything else, in every direction and
endlessly far away: the camera turns in it and never moves through it. A
scene has one, on any entity; it needs no `Transform`. Without one the
frame shows black.

| Name | Holds | Default |
|---|---|---|
| `type` | `box`, six images that are the faces of a cube, or `sphere`, one panorama around the camera | `box` |
| `faces` | For a box: `right`, `left`, `top`, `bottom`, `front`, and `back`, the virtual path of an image each | none |
| `texture` | For a sphere: the virtual path of the panorama | none |
| `rotation` | Degrees the sky is turned around the direction that is up, against the clock seen from above | `0` |
| `brightness` | What the light of the images is multiplied by | `1` |

```yaml
- name: sky
  components:
    Sky:
      type: box
      faces:
        right: assets://textures/sky/day-right.png
        left: assets://textures/sky/day-left.png
        top: assets://textures/sky/day-top.png
        bottom: assets://textures/sky/day-bottom.png
        front: assets://textures/sky/day-front.png
        back: assets://textures/sky/day-back.png
```

```yaml
- name: sky
  components:
    Sky:
      type: sphere
      texture: assets://textures/sky/museum-panorama.png
```

The faces of a box are named after the direction each is seen in: `front`
is what a camera that was not turned looks at, along negative z, `right`
is along positive x, and `top` along positive y. They are laid out as sky
boxes are painted, a cross folded around the camera: the four sides are
upright, the right edge of `front` meets the left edge of `right`, the
lower edge of `top` meets the upper edge of `front`, and the upper edge of
`bottom` meets the lower edge of `front`. All six are squares of one size.

The panorama of a sphere is twice as wide as high and holds every
direction (an equirectangular image, which is how panoramas are published):
its middle is seen along negative z, what is right of the middle towards
positive x, its upper edge straight up.

The images are PNG or JPEG, colors as a screen shows them. The sky is
shown as it is: no light of the scene falls on it, and it lights nothing
itself, so a `Light` is set to match it by hand. The museum has a sky of type sphere. How it is drawn is in
[vulkan-renderer.md](vulkan-renderer.md#the-sky).

**Spectator**

| Name | Holds | Default |
|---|---|---|
| `move_speed` | Units per second | `2.5` |
| `look_speed` | Degrees per unit the mouse moved | `0.1` |

**FirstPersonController**

The entity is the player, seen from the first person and driven by the
input. It carries a `CharacterBody`, and a child with a `Camera` is lifted
to its eyes. How it is driven is in [physics.md](physics.md#the-player).

| Name | Holds | Default |
|---|---|---|
| `walk_speed` | Meters per second along the ground | `4` |
| `run_speed` | Meters per second along the ground while `run` is down | `6` |
| `jump_speed` | Meters per second upward that a jump starts with | `5` |
| `air_control` | How much `move` steers the body in the air, from 0 to 1: 0 keeps the take-off velocity, 1 steers as on the ground | `0.3` |
| `look_speed` | Radians the view turns for every pixel of `look` | `0.0025` |
| `eye_height` | Meters from the feet to the eyes, where the camera is put | `1.6` |
| `camera_offset` | `[right, up, back]`, meters the camera is moved from the eyes in the frame of the entity | `[0, 0, 0]` |
| `step_smoothing` | How quickly the eyes catch up with a step, per second. 0 lifts them with the body | `10` |
| `max_pitch` | Degrees the view can turn up or down, from 0 to 90 | `89` |

**Persistent**

The entity stays when the world [changes scene](#changing-the-scene), with
everything below it. The component holds nothing that matters; being there
is what it says, so it is written as `Persistent: Default`.

| Name | Holds | Default |
|---|---|---|
| `keep` | Whether it stays. Written for completeness | `true` |

**SceneExit**

The `Trigger` of the entity is the way out of the scene: while a body is
inside it, the world changes to the scene named. An entity without a
`Trigger`, or a world without physics, has an exit that never opens.

| Name | Holds | Default |
|---|---|---|
| `scene` | Virtual path of the scene to change to | None, and nothing happens |

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
| `Collider` | The shape of the body of its entity, or of the nearest entity above it that has one | `shape`, `offset`, `rotation`, and what belongs to the shape: `size`, `radius`, `height`, `top_radius`, `bottom_radius`, `model`, `fit`. A `mesh` or `convex_hull` without a `model` takes the entity's `Geometry` |
| `Trigger` | An area that reports what enters and leaves it | `layers`, `mask` |
| `CharacterBody` | Something that is moved by a velocity, stops at what is in its way, and slides along it | `velocity`, `fall_velocity`, `gravity_scale`, `max_slope`, `step_height`, `mass`, `push_strength`, `layers`, `mask` |
| `Joint` | Holds the body of its entity to another body, named by its path, or to the world | `type`, `other`, `anchor`, `collide_with_other`, for a rope: `other_anchor`, `length`, and for a hinge and a slider: `axis`, `limits`, `motor_velocity`, `motor_strength`, `spring` |

## Where the scenes are

NeonRuntime ships no scene: every scene belongs to a project, which the
build puts together next to a copy of the runtime:

| Project | Holds | Built and started with |
|---|---|---|
| [projects/sandbox](../projects/sandbox) | [start.scene.yml](../projects/sandbox/assets/scenes/start.scene.yml), a box on a floor, which the engine is tried out with | `cmake --build --preset macos-arm64-debug`, then `bin/debug/darwin-arm64/sandbox/NeonRuntime` |
| [projects/museum](../projects/museum) | [The museum](#the-museum) | `cmake --build --preset macos-arm64-debug`, then `bin/debug/darwin-arm64/museum/NeonRuntime` |
| [projects/bench](../projects/bench) | Scenes that measure the engine | See its [README.md](../projects/bench/README.md) |
| [tests/game](../tests/game) | Every scene the tests start the runtime with, and what those scenes show | `cmake --build --preset macos-arm64-debug-tests`, then `build/macos-arm64-debug/tests/game/NeonRuntime --scene assets://scenes/physics.scene.yml` |

Where these notes write `NeonRuntime --scene assets://scenes/...` for a scene
of the tests, it is the runtime of `tests/game` that is meant. What the
scenes of the tests show is built by the engine as `Geometry`, or is a small
file that a script of `tools/` writes; nothing in the repository was taken
from elsewhere but the fonts, see
[CREDITS.md](../CREDITS.md).

## The museum

[demo.scene.yml](../projects/museum/assets/scenes/demo.scene.yml), the scene
of the project [projects/museum](../projects/museum), is a museum
of what the engine does, to walk through from the first person: a corridor
with twelve halls, each with a sign that says what it shows. Nothing in it
comes from a model file or an image. Every piece is a `Geometry` in a plain
color, see [geometry.md](geometry.md), until models of our own take their
places. A quiet piano piece and a bed of wind and birds play throughout, with
birds by the entrance and a stream by the far wall: public domain recordings,
named in [CREDITS.md](../CREDITS.md).

| Hall | Shows | Read more |
|---|---|---|
| 01 Shapes | Every shape a `Geometry` builds, flat and smooth, and a pipe on the wall that is a tube along a curve | [geometry.md](geometry.md), [curves.md](curves.md) |
| 02 Materials | The `pbr` shader over roughness, metalness, and color; `basic-lit`, `color`, and an `emissive` material | [vulkan-renderer.md](vulkan-renderer.md#the-shaders-that-ship) |
| 03 Lights | Point lights that mix, a spot light, and a cube that glows, in a room the sun does not reach | [Light](#components) |
| 04 Transparency | `alpha_mode: blend` in order of distance, and `double_sided` | [Renderable](#components) |
| 05 Shadows | The shadows of the sun, of what stands still and of what a script turns | [vulkan-renderer.md](vulkan-renderer.md#shadows) |
| 06 Physics | Static, dynamic, and kinematic bodies, and a collider of every shape | [physics.md](physics.md) |
| 07 Joints | A hinge with a spring, a pendulum on a rope that hangs slack when the ball is lifted, a slider, a motor, a fixed joint with its bar, and a cable that hangs between two pegs | [physics.md](physics.md#joints), [curves.md](curves.md) |
| 08 Character | Steps, slopes, a lift, and a second character that a script walks | [physics.md](physics.md#the-player) |
| 09 Scripts | The `Spinner`, the `Mover`, and the `TriggerLamp` of `assets/scripts` | [scripting.md](scripting.md) |
| 10 Surfaces | A terminal that is pointed at and pressed, whose buttons call handlers of the `Terminal` of `assets/scripts` with `on_click`: Unlock slides the door of a safe, Alarm has a lamp blink. And what a second camera sees | [user-interface.md](user-interface.md#surfaces) |
| 11 Sound | A spatial `SoundSource` that circles the listener, over the music and the ambience of the whole museum | [audio.md](audio.md) |
| 12 Prefabs and scenes | One prefab placed five times with what differs, and a `SceneExit` | [prefabs.md](prefabs.md), [below](#changing-the-scene) |

The player is a `FirstPersonController` with a `CharacterBody` and a camera below it with a
`fov` of 59, which is 90 degrees across a window of 16 by 9: `fov` is the
field of view up and down. The signs are one prefab, `assets/prefabs/museum/sign.prefab.yml`,
whose board shows a user interface of `assets/ui/museum` on a surface of its
own. A part of the engine that is added gets a hall, or a piece in one.

## Changing the scene

A game is more than one scene: a title screen, levels, an end. The world
changes from one to the next in play, which the runtime calls the scene
manager (#118). Three things ask for a change:

- A button with `action: scene` and the `scene` to change to, see
  [user-interface.md](user-interface.md#elements). The runtime hands the
  scene to the world. The file of the button is closed, as it would be with
  `action: close`.
- An entity with a `SceneExit` and a `Trigger`, when a body is inside the
  trigger: the door at the end of a level.
- Code, through `WorldSystem::LoadScene(path)`, and a script once scripts
  come (#57).

```yaml
# title.ui.yml: the Start button of a title screen
- type: button
  name: start
  text: Start
  action: scene
  scene: assets://scenes/prototype.scene.yml

# prototype.scene.yml: the way out of the level. A trigger reports static
# bodies too, so it looks for a layer that only the player is in
- name: exit
  components:
    Transform:
      position: [2.5, 1, -2.5]
    Trigger:
      mask: [2]
    Collider:
      shape: box
      size: [0.8, 2, 0.8]
    SceneExit:
      scene: assets://scenes/end.scene.yml
```

What happens, in this order, at the start of the next frame of the world,
so that a change never falls in the middle of one:

1. The file of the new scene is read. A file that is not there, cannot be
   opened, or holds no document changes nothing: the log says which scene
   and why, and the game stays in the scene it is in, as if it had not
   been asked.
2. Every entity at the top that has no `Persistent` is destroyed, with
   everything below it. What a system held for those entities is released
   as it is when an entity goes away in play: the renderer's objects, the
   bodies of the physics, the sounds, the user interface a `Ui` showed.
3. What was read of the new scene is placed in the store, as the first
   scene was. A file with problems is said in the log, and the world runs
   with what could be read of it.

An entity with `Persistent` stays where it is, with its children: the
player, a score, a sound that goes on. The new scene is read next to it, so
a scene that is entered with a persistent player does not list one. Whether
the player carries over is the game's to decide; the prototype's does not,
each level places its own.

The last scene asked for in a frame is the one taken. A scene asked for
without a path is refused and said in the log.

The prototype game of the tests goes
[title.scene.yml](../tests/game/assets/scenes/title.scene.yml) ->
[prototype.scene.yml](../tests/game/assets/scenes/prototype.scene.yml)
-> [end.scene.yml](../tests/game/assets/scenes/end.scene.yml), from
which the end menu goes back to either.

## Made to be changed by hand

A first scene that cannot be read at all, the entry scene of the project or
the one `--scene` names, stops the application before its first frame with
an exit code of 1, and the log says which scene and why. Once the game
runs, nothing stops it: a scene it changes to and cannot read leaves it
where it is, and a scene with problems in what it holds runs with what
could be read. Both are in the log.

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
| What is drawn without a `Transform` is given one, with a warning | `demo.scene.yml:12: entity 'bear' has a Renderable and no Transform; one was added at the origin`, see below |

### Components that need another

A system works on the entities that carry every component it asks for, so a
component without the one it goes with is passed over, and nothing would
say why: a model without a `Transform` is simply not there. Drawing
something that is placed nowhere makes no sense, so the loader gives a
`Renderable`, a `Camera`, or a `Light` without a `Transform` one of its
defaults, which puts it where its parent is, at the origin at the top of
the scene, and logs a warning with the line where the `Transform` belongs.
What cannot be mended that way is a problem of the scene, logged as an
error and counted with the others, and the game goes on.

| Component | Needs | Without it |
|---|---|---|
| `Renderable` | `Transform` | A `Transform` of its defaults is added, with a warning |
| `Camera` | `Transform` | The same |
| `Light` | `Transform` | The same |
| `Rope` | `Renderable` | A problem of the scene: the rope is drawn nowhere |
| `Geometry` | `Renderable`, or a `Collider` that takes its shape | A problem of the scene: its shape is drawn nowhere |

| Decision | Reason |
|---|---|
| A missing `Transform` is added, and the warning says so | An entity that is drawn is always drawn somewhere. The warning points at the line, so that the recipe is corrected |
| A warning is not a problem of the scene: the `has N problems` line counts errors alone, and the scene was read in full | Nothing was left out. What is counted is what could not be read |
| A missing `Renderable` is not added | A `Renderable` of its defaults has no shader, which the renderer refuses, so one made up would only move the error to the renderer |
| `Save` writes the `Transform` that was added | The world is what is saved. A scene that is saved again has it written, and the warning is gone |
| The entity is asked once it is read in full: its prefab, what the scene writes on top, and its children, those the prefab gave included | A prefab may leave the `Transform` to the scene that places it |
| The message has the line of the entity, or of the nearest entity above it that the scene writes, for a child that only a prefab gave | That is where the missing component is written |
| A component that is turned off still needs what it needs | It is drawn once it is turned on, as an instance of a pool is |
| Every spawn is given its `Transform`, and the first spawn of each prefab says so | A nail gun spawns many nails of one prefab |
| An entity that code makes is not asked | Only a recipe has a line to point to; code that makes an entity gives it what it needs |

## When the engine writes

`SceneFile::Save` writes every entity of the store. It is what NeonEditor will
use.

| Rule | Reason |
|---|---|
| A value that is the default is left out | The file stays short and a change to it shows up as one line |
| An entity placed from a prefab is written with its `prefab:` and every component it has, and a child of the prefab it lacks as `- name: ~` | Loading it gives the same world. Leaving out what the prefab says is open, see [prefabs.md](prefabs.md#open-questions) |
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
- One scene inside another. An entity that is placed a hundred times is a
  prefab now, see [prefabs.md](prefabs.md); a whole scene inside another is
  not.
- An entity that refers to another, such as a door to its switch. Paths such
  as `player/camera` are the likely answer.
