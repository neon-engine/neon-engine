# Physics

This note records how the physics of Neon Engine works, how it is used, why
it is built the way it is, and what is still open.

**Current decision:** physics is components and a system. An entity is given
a `RigidBody`, a `Trigger`, or a `CharacterBody`, and a `Collider` for its
shape. The engine has its own interface for the physics, with one
implementation built on [Jolt Physics](https://github.com/jrouwe/JoltPhysics)
5.2.0. The world advances in steps of one length, whatever the frame rate is.

## What a body can be

| Behaviour | Component | In Godot | Meaning |
|---|---|---|---|
| Does not move | `RigidBody` with `kind: static` | StaticBody3D | Floors and walls. Everything else stops at it |
| Collide and react | `RigidBody` with `kind: dynamic` | RigidBody3D | Moved by the simulation: gravity, forces, impulses, and what it hits |
| Moved by code, pushes | `RigidBody` with `kind: kinematic` | AnimatableBody3D | It is where its `Transform` puts it. What is in its way is pushed, and nothing holds it back. Lifts, doors, platforms |
| Collide and stop | `CharacterBody` | CharacterBody3D with `move_and_slide` | Moved by a velocity. It stops at what is in its way and slides along it. It pushes dynamic bodies and is not pushed by them |
| Overlap only | `Trigger` | Area3D | It reports what enters and leaves it. Nothing is stopped or pushed |

An entity carries one of `RigidBody`, `Trigger`, and `CharacterBody`. One
that carries more is refused, which is said in the log.

```yaml
- name: crate
  components:
    Transform:
      position: [0, 5, 0]
    Renderable: { ... }
    RigidBody:
      mass: 10
    Collider:
      shape: box

- name: door-trigger
  components:
    Transform:
      position: [0, 1, -5]
    Trigger: {}
    Collider:
      shape: sphere
      radius: 2
```

The scene
[physics.scene.yml](../app/NeonRuntime/assets/scenes/physics.scene.yml) shows
all of them:

```
NeonRuntime --scene assets://scenes/physics.scene.yml
```

## Components

What a component leaves out keeps its default. A value that is wrong is
reported with the file and the line, as for every component of a
[scene](scenes.md).

Each component is described next to itself, in its header, and how it is
read and written follows from the description, as for every component of the
engine. So the editor and a script know its fields by name as well. The
fields of a `Collider` that belong to some shapes alone say so with
`OnlyWhen()`, and what the fields have to be together, such as a capsule that
is at least as high as it is wide, is a rule of the description. See
[reflection.md](reflection.md).

**RigidBody**

| Name | Holds | Default |
|---|---|---|
| `kind` | `static`, `kinematic`, or `dynamic` | `dynamic` |
| `mass` | Of a dynamic body, above 0 | `1` |
| `friction` | 0 slides without end, 1 grips. 0 or above | `0.5` |
| `bounce` | How much of its speed the body keeps when it bounces, from 0 to 1 | `0` |
| `linear_damping`, `angular_damping` | How fast motion and turning die down by themselves. 0 or above | `0.05` |
| `gravity_scale` | 0 floats, 1 falls as everything does | `1` |
| `linear_velocity` | `[x, y, z]` in units per second | `[0, 0, 0]` |
| `angular_velocity` | `[x, y, z]` in degrees per second around each axis | `[0, 0, 0]` |
| `continuous` | Looks for hits along the whole way of a step. For what is small and fast, which would otherwise pass through a wall between two steps | `false` |
| `can_sleep` | A body that came to rest stops being simulated until something touches it | `true` |
| `layers`, `mask` | See [layers](#layers-and-masks) | `1` |

In code it has two more, which the engine fills in and a scene recipe never
holds: `body`, the id the physics knows the body by, and `failed`, which is
set when the body could not be created.

The values are read when the body is created. After that the physics follows
two things that a game writes: the `Transform`, and the two velocities.

| What a game writes | Dynamic | Kinematic | Static |
|---|---|---|---|
| `position` and `rotation` of the `Transform` | The body is put there at once | The body moves there in the step that follows, and pushes what is in its way | The body is put there at once |
| `linear_velocity`, `angular_velocity` | What the body moves with from the next step on | What the body moves with by itself, when nothing moves its `Transform` | Nothing |

What the engine writes in every step:

| | Dynamic | Kinematic | Static |
|---|---|---|---|
| `position` and `rotation` of the `Transform` | Where the simulation put the body | Only when it moved by its velocity. What a game wrote is left as it was written | Never |
| `linear_velocity`, `angular_velocity` | What the body moves with | Never | Never |

Replacing the component of an entity with another creates the body anew, with
the values of the new one.

**Collider**

| Name | Holds | Default | Belongs to |
|---|---|---|---|
| `shape` | One of the [shapes](#shapes) | `box` | |
| `size` | `[x, y, z]`, the lengths of the sides, or one number for all three | `1` | `box` |
| `radius` | Above 0 | `0.5` | `sphere`, `capsule`, `cylinder` |
| `height` | From end to end, above 0. That of a capsule includes its round ends, and is at least twice the radius | `2` | `capsule`, `cylinder`, and the tapered kinds |
| `top_radius`, `bottom_radius` | Above 0. One of the two may be 0 for a `tapered_cylinder`, which makes a cone | `0.25`, `0.5` | The tapered kinds |
| `model` | Virtual path of a model | None. It has to be written | `convex_hull`, `mesh` |
| `offset` | `[x, y, z]`, where the shape sits on its entity | `[0, 0, 0]` | Every shape |
| `rotation` | `[pitch, yaw, roll]` in degrees, how the shape is turned on its entity | `[0, 0, 0]` | Every shape |

A name that does not belong to the shape is an error, as a name that is not
known is. `radius` for a box would otherwise be ignored without a word.

The shape is sized by the `scale` of the `Transform`, as what is drawn is. A
floor that is a cube scaled by `[100, 0.1, 100]` has a `box` with its
defaults as its collider.

**Trigger**

| Name | Holds | Default |
|---|---|---|
| `layers` | The layers the trigger is in, which only queries look at | `1` |
| `mask` | The layers the trigger looks for | `1` |

In code it has `inside`, the number of bodies that are inside, which the
engine keeps, and `body` and `failed` as a `RigidBody` has.

A trigger is where its `Transform` puts it, and follows when that is moved.

**CharacterBody**

| Name | Holds | Default |
|---|---|---|
| `velocity` | `[x, y, z]` in units per second, what the entity is meant to move with | `[0, 0, 0]` |
| `fall_velocity` | `[x, y, z]`, what falling has added. A game writes it to jump | `[0, 0, 0]` |
| `gravity_scale` | 0 for what does not fall, such as something that flies | `1` |
| `max_slope` | Degrees from 0 to 90. Steeper ground is a wall | `45` |
| `step_height` | A step up to this height is walked up as if it were a ramp | `0.25` |
| `mass` | What dynamic bodies feel of the entity, above 0 | `70` |
| `push_strength` | The most force it pushes dynamic bodies with | `100` |
| `layers`, `mask` | See [layers](#layers-and-masks) | `1` |

In code it has what the last step led to, written by the engine: `on_floor`,
`on_wall`, `on_ceiling`, `floor_normal`, `floor` (the entity it stands on),
and `real_velocity`, which is less than what was asked for when something was
in the way. And `character` and `failed`, as the others.

| | Godot | Neon Engine |
|---|---|---|
| Moving | The script calls `move_and_slide()` in `_physics_process` | The engine moves every `CharacterBody` in every step |
| `velocity` | Is changed by `move_and_slide()` to what was left after sliding | Is kept as the game wrote it. An entity that a wall stopped moves on once the wall is gone. What it moved with is `real_velocity` |
| Gravity | The script adds it | The engine adds it to `fall_velocity` while the entity is in the air, and ends it on the ground. `gravity_scale: 0` leaves it to the game |
| Jumping | `velocity.y = 5` | `fall_velocity = {0, 5, 0}` |
| Turning | The script turns the node | The game turns the `Transform`. The shape stays upright |

So a scene recipe alone makes a character walk, which is what the scene of the
runtime does.

## Shapes

| Shape | What it is | Can be on |
|---|---|---|
| `box` | | Everything |
| `sphere` | | Everything |
| `capsule` | A cylinder with half a sphere at each end, along the Y axis | Everything |
| `cylinder` | Along the Y axis | Everything |
| `tapered_capsule` | A capsule whose ends have different radii | Everything |
| `tapered_cylinder` | A cylinder whose ends have different radii. With one of 0 it is a cone | Everything |
| `plane` | Everything below a plane without end. It faces up, and is turned by `rotation` | A static body |
| `convex_hull` | The smallest shape without dents that holds the points of a model | Everything |
| `mesh` | The triangles of a model as they are, dents and holes included | A static body, a kinematic one, and a trigger |

Every shape reaches as far up from where it is as down.

**A mesh cannot be on a dynamic body.** A mesh is a surface without an
inside, so it has no mass, and two meshes cannot collide. The body is not
created, and the log says why:

```
The RigidBody of entity 'level/rock' cannot be created: a dynamic body cannot
have a mesh. A mesh is a surface without an inside, so it has no mass. Make
the body static or kinematic, or give it a convex hull
```

The same holds for a plane on anything but a static body, and for a mesh and
a plane on a `CharacterBody`.

What could not be created is not tried again, since it would fail and say so
in every step. `failed` of the component says that it happened.

**Scale.** A box and a convex hull and a mesh take every scale. A sphere
keeps its form only when all three axes are scaled the same, and a capsule
and a cylinder when X and Z are. A scale that a shape cannot take is refused
with a message. Where what is drawn has a scale that its collider cannot
take, the collider goes on the entity and what is drawn on a child:

```yaml
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
  children:
    - name: looks
      components:
        Transform:
          scale: [0.8, 1.8, 0.8]
        Renderable: { ... }
```

**Models.** The renderer moves a model so that its middle lies at the origin,
and sizes it so that its longest side has length 1. A convex hull and a mesh
are made from the same points, moved and sized the same way, so the shape
lies where the model is drawn. A model is read once, however many colliders
name it.

### Several shapes

A body of several shapes is an entity with children that each carry a
`Collider`, as a body of Godot has CollisionShape3D nodes below it:

```yaml
- name: table
  components:
    Transform:
      position: [0, 1, 0]
    RigidBody: {}
  children:
    - name: top
      components:
        Transform:
          position: [0, 0.9, 0]
        Collider:
          shape: box
          size: [2, 0.2, 2]
    - name: leg
      components:
        Transform:
          position: [-0.9, 0.4, -0.9]
        Collider:
          shape: box
          size: [0.2, 0.8, 0.2]
```

| Rule | |
|---|---|
| Which body a collider belongs to | The `RigidBody`, `Trigger`, or `CharacterBody` of its own entity, or of the nearest entity above it that has one |
| Where the shape sits | Where the `Transform` of its entity puts it relative to the body, over as many levels as lie between them |
| A child that is a body of its own | Its colliders, and those below it, are its own |
| A collider that belongs to nothing | Is said once in the log, as a warning |
| A collider that is added after the body was created | Is not part of it, which is said once in the log. The shapes of a body are read when it is created |

## Layers and masks

There are 32 layers, called by their numbers from 1 to 32. A body is in
layers and has a mask of layers it looks for.

```yaml
RigidBody:
  layers: 2          # one layer
  mask: [1, 2, 5]    # or a list. [] is none
```

| Between | They meet when |
|---|---|
| Two bodies, or a body and a character | One of them looks for a layer the other is in |
| A trigger and a body | The trigger looks for a layer the body is in. What the body looks for does not count |
| Two triggers | Never |
| A query and a body | The query looks for a layer the body is in |

In code both are a number with one bit for each layer, the lowest for layer
1.

## The time step

**The physics does the same whatever the frame rate is.** A game that runs at
30 frames per second and one that runs at 1000 have taken the same steps
after a second, and every body is in the same place, down to the last bit.

| | |
|---|---|
| What a step is | A sixtieth of a second. `steps_per_second` of the settings changes it |
| When steps are taken | The time of every frame is collected. Whenever enough for a step has come together, a step is taken |
| At 30 frames per second | Two steps in every frame |
| At 240 frames per second | A step in every fourth frame |
| What the length of a step depends on | Nothing. It is never the time of a frame, and never a part of it |

### What runs when

A frame of `EntityWorld` runs in this order:

| Order | What | How often | With |
|---|---|---|---|
| 1 | `FixedUpdate` of every system, in the order the systems were added | Once for every step the frame takes, which can be never | The length of a step |
| 2 | `Update` of every system | Once | The time the frame took |
| 3 | Every entity is placed in the world | Once | |
| 4 | `Interpolate` of every system | Once | How far the frame lies between the last two steps |
| 5 | What is visible is handed to the renderer | Once | |

| Runs at the fixed rate | Runs once per frame |
|---|---|
| Creating bodies | Reading the input |
| Moving characters | Moving a camera |
| The step of the simulation | Placing what is drawn between two steps |
| Writing where bodies are into their `Transform` | Drawing |
| The events of a step | The events of a frame |
| `FixedUpdate` of the systems of a game | `Update` of the systems of a game |

### The rule for game code

**What decides how the game goes belongs in `FixedUpdate`. What is only seen
or heard belongs in `Update`.**

| In `FixedUpdate`, with the length of a step | In `Update`, with the time of the frame |
|---|---|
| Forces and impulses | Reading the input, and keeping what was asked for |
| Writing a velocity | Turning the camera with the mouse |
| Moving a kinematic body | Animations, particles, sound |
| Timers that a game is decided by: a cooldown, a fuse | What a menu shows |
| Anything that adds up over time and changes how the game goes | |

```cpp
class Thruster final : public neon::EntitySystem
{
  neon::PhysicsContext *_physics;
  neon::QueryId _query = 0;

public:
  explicit Thruster(neon::PhysicsContext *physics) { _physics = physics; }

  void Initialize(neon::EntityStore &store) override
  {
    _query = store.Query<neon::RigidBody, Engine>();
  }

  // nothing here belongs to a frame
  void Update(neon::EntityStore &store, double delta_time) override {}

  void FixedUpdate(neon::EntityStore &store, double fixed_delta_time) override
  {
    store.Each(_query, [&](const neon::EntityBlock &block)
    {
      const auto *bodies = block.Column<neon::RigidBody>(0);
      const auto *engines = block.Column<Engine>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        // a force acts for the step that follows, so it is asked for in
        // every step
        _physics->AddForce(bodies[i].body, engines[i].thrust);
      }
    });
  }
};
```

A force that is asked for in `Update` is asked for 30 times in a second on one
machine and 300 times on another. That is the bug of old games that ran
faster on faster machines. The engine cannot keep a game from writing it, and
makes the right way the short one: `FixedUpdate` is handed the only time
that is right to multiply with.

**The systems of a game are added before the system of the physics.** What
they ask for in a step is then part of that step.

### What is drawn

A step is taken sixty times in a second, and a frame is drawn as often as the
machine can. Drawn where the last step put it, a body would stand still for
three frames and jump in the fourth at 240 frames per second, and move
unevenly at 50.

So what is drawn lies between the last two steps, as far as the time that
waits for the next step reaches into it:

| | |
|---|---|
| What is placed | Every dynamic body, every kinematic body that moved, every character, and what is below them in the hierarchy |
| Where | Between where the step before the last put it and where the last put it. The position along a line, the rotation along the shortest turn |
| What is changed | `world_coordinates` of the `Transform`, which is what is drawn |
| What is not changed | `position` and `rotation` of the `Transform`, the velocities, and the body itself. A game and the next step see where the simulation put it |
| What it costs | What is drawn is up to one step behind where the body is |
| A body that was put somewhere at once | Is drawn there at once, and nowhere on the way |
| A body that rests | Is where its `Transform` says, and is left alone |

A system of a game places something of its own between two steps by
overriding `Interpolate`.

`Forward()` and `Right()` of a `Transform` read `world_coordinates`, and so
follow what is drawn.

### A frame that took long

A frame that took half a second asks for thirty steps. Taking them takes
long, so the next frame asks for more, and the one after for more again.

| | |
|---|---|
| The most steps a frame takes | 8. `most_steps_per_frame` of the settings changes it |
| What happens to the time above them | It is given up |
| What that means | The world runs slower than the clock on the wall for that frame, and goes on from where it is. The length of a step stays what it is, and the simulation stays right |
| When it happens | Below 7.5 frames per second, and after the machine was asleep or the application was held in a debugger |

## Events

The physics reports when two bodies begin and end to touch.

```cpp
for (const auto &event : physics->GetStepEvents())
{
  if (event.trigger && event.kind == neon::PhysicsEventKind::Began)
  {
    // event.second entered event.first
  }
}
```

| Of an event | Holds |
|---|---|
| `kind` | `Began` or `Ended` |
| `trigger` | Whether one of the two is a trigger |
| `first`, `second` | The two entities. With a trigger, the first is the trigger and the second what entered or left. Without one, the first is the body that was created first |
| `first_body`, `second_body` | The ids of their bodies |
| `point`, `normal` | Where they touch, and the direction from the first to the second. Both are 0 when the touch ended |

| Read | In | Holds |
|---|---|---|
| `GetStepEvents()` | `FixedUpdate` | What the last step reported |
| `GetFrameEvents()` | `Update` | What every step of this frame reported. A frame without a step has none |

Each event is handed to `Update` in one frame, and to `FixedUpdate` in one
step, whatever the frame rate is.

| Decision | Reason |
|---|---|
| Events are a list on the interface, not a function that is called | Jolt finds contacts on several threads while it takes a step. A function of a game that is called from there would have to be safe to call from any thread, and could not change the world. A list is read on the thread of the world, when the step is done |
| The events of a step are put in order | The threads find them in an order that differs from run to run. The same run hands them over in the same order |
| What ended comes before what began | A body that left one trigger and entered another in the same step is never in both |
| A body of several shapes is reported once | What counts is that the bodies touch, not how many of their parts do |
| Bodies that come to rest and are put to sleep still touch | Jolt says that their contact is gone, since it stops looking at them. Nothing `Ended` is reported, and nothing `Began` when they wake up. It `Ended` when one of them is moved away or destroyed |
| A `Trigger` counts what is inside | `inside` of the component is enough for a door that is open while something stands in it, without reading events |

A trigger reports dynamic bodies, kinematic bodies, characters, and static
bodies. It reports a body that rests inside it once.

## Queries

```cpp
neon::RayHit hit;
const neon::Ray ray{.origin = position, .direction = forward, .distance = 50.0f};

if (physics->CastRay(ray, neon::QueryFilter{.ignore = player}, hit))
{
  // hit.entity, hit.point, hit.normal, hit.distance
}

neon::ShapeInfo blast;
blast.kind = neon::ShapeKind::Sphere;
blast.radius = 5.0f;

std::vector<neon::OverlapHit> hits;
physics->Overlap(blast, position, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}, neon::QueryFilter{}, hits);
```

| Query | Finds |
|---|---|
| `CastRay` | The first body along a ray: the entity, the point, the direction away from what was hit, and the distance |
| `Overlap` | Every body that overlaps a shape that is held somewhere. Each body once, ordered by its id. The shape is any but a mesh and a plane |

| Of a `QueryFilter` | Holds | Default |
|---|---|---|
| `mask` | A body is looked at when it is in one of these layers | Every layer |
| `triggers` | Whether triggers are looked at | `false` |
| `ignore` | An entity whose bodies are skipped, such as the one that asks | None |

A character is found as a body is.

## The interface

| Piece | Location | Role |
|---|---|---|
| `PhysicsContext` | neon-core | What the rest of the engine sees: bodies, characters, the step, events, and queries |
| `PhysicsSystem` | neon-core | Base class of backends, with `Initialize` and `CleanUp`. It keeps the events |
| `Jolt_PhysicsSystem` | neon-jolt | The implementation |
| `RigidBody`, `Collider`, `Trigger`, `CharacterBody` | neon-core | The components |
| `PhysicsSimulation` | neon-core | The system that brings entities and the physics together |
| `FixedClock` | neon-core | Turns the time of frames into steps. It belongs to `EntityWorld` |
| `LoadModelGeometry` | neon-core | Reads the points and triangles of a model through the file system |
| `FakePhysicsContext` | neon-core, `neon/testing` | A physics in which nothing collides, for tests |

Nothing outside neon-jolt includes a header of Jolt. The library is linked
privately, so the compiler refuses it anywhere else. The header of
`Jolt_PhysicsSystem` names no type of Jolt. What Jolt keeps is behind a
pointer to a type that only the source file knows.

An application puts it together as the file system is:

```cpp
neon::Jolt_PhysicsSystem physics_system(settings_config, logging_system.CreateLogger("Jolt_PhysicsSystem"));
physics_system.Initialize();

world.AddSystem(std::make_unique<neon::PhysicsSimulation>(
  &physics_system,
  &file_system,
  logging_system.CreateLogger("PhysicsSimulation")));

// ... the application runs ...

app.CleanUp();
physics_system.CleanUp();
file_system.CleanUp();
```

What a game does with a body, through `PhysicsContext` and the `body` of its
`RigidBody`:

| Function | Does |
|---|---|
| `AddForce`, `AddTorque` | Acts for the step that follows. Asked for in every step for as long as it is meant to act |
| `AddImpulse`, `AddImpulseAt` | Changes the velocity at once, as a kick does |
| `SetLinearVelocity`, `SetAngularVelocity` | The same as writing the velocities of the component. Turning is in radians per second here, and in degrees in the component |
| `SetBodyPlace`, `MoveBody` | The same as writing the `Transform` |
| `GetBodyState` | Where the body is, how it moves, and whether it sleeps |
| `SetGravity`, `GetGravity` | `[0, -9.81, 0]` unless it is set |

### Decisions

| Decision | Reason |
|---|---|
| A body is released when its component leaves its entity | The store tells the system, through `on_remove`. That includes the entity being destroyed and the store being cleaned up. Nothing has to watch for it |
| The shapes and the values of a body are read once | Changing the shape of a body while it collides is costly, and rare. Replacing the component creates the body anew |
| A collider is a component of its own | One shape can be on a body, a trigger, and a character, and a body can have several |
| A kinematic body is moved by its `Transform` | It is what a game, an animation, and an editor write already |
| A character is a component of its own, not a kind of `RigidBody` | It has other values, and is moved another way: by a sweep of its shape and not by the simulation |
| The physics is handed the file system, and the backend is not | Reading a model is the same for every backend. The backend is handed points |
| Nothing of Jolt is made with `new` | The factory Jolt shares is one static object, whose address Jolt is given while a physics is in use. A character is built in place in a map, whose nodes do not move, and marked as embedded so that Jolt, which counts references to it, never deletes it. What Jolt makes itself, such as shapes, is held by its own references |
| The formats of the components are part of those of the engine | A scene with a `RigidBody` loads in every application. One without physics says so: `RigidBody of entity 'crate' needs the physics, which is not part of this world` |

## Entities with a parent

The physics works in the space of the world. A `Transform` is relative to the
parent of its entity.

| | |
|---|---|
| A body on an entity with a parent | Is created where the parents put the entity in the world, turned and sized as they turn and size it |
| A dynamic body | Is not moved by its parent. In every step, where the physics put it is turned into what is relative to the parent, as the parent is at that moment |
| A static body, a kinematic body, and a trigger | Are carried by their parent |
| A character | Is not moved by its parent |
| Parents before children | A body below another body finds its parent where this step put it |

**Limit:** a parent that is sized differently along its axes, with a child
that is turned, shears the child. A shape cannot be sheared. The shape is then
sized by the lengths of the axes, and what is drawn is not.

## Rotation

`Transform` holds its rotation as pitch, yaw, and roll in degrees. The physics
works with quaternions, and a body that tumbles passes through every
orientation.

**Decision:** the quaternion of a body is kept by the physics, and the three
angles are written from it in every step, with `Rotation::FromQuaternion`.
`Rotation` stays what it was.

| Question | Answer |
|---|---|
| Can three angles hold every orientation? | Yes. What they cannot do is change smoothly. Where the pitch is 90 degrees, yaw and roll turn around the same axis, and the angles of two orientations that are close can be far apart |
| Does that harm a body that tumbles? | No. The angles are never added to, and nothing is ever placed between two sets of them. The physics advances its quaternion, and what is drawn between two steps is placed between two quaternions. The angles are only what the orientation is written down as |
| Is what is written exact? | `FromQuaternion(q).GetQuaternion()` is `q` or its negative, which is the same orientation, within what a `float` can hold. That is tested for every 15 degrees around each axis, close to straight up, and along a tumble of 1000 steps |
| What about close to straight up? | The pitch is found from a length and not from an arc sine, which loses half of its digits there. The roll is found from what is left once yaw and pitch are taken away, which makes up for what the yaw is off by |
| What does a game see there? | Angles that jump from one step to the next, for an orientation that does not. A game that needs the orientation asks for `GetQuaternion()` |

**Why `Rotation` does not store a quaternion.** Its three angles are public
fields that are written one at a time, as in `rotation.yaw -= 5`, in the
engine, in its tests, and in games. A quaternion as the stored value would
turn each of them into a function, and change every line that writes one. It
would be needed for something that reads the angles, changes them, and writes
them back, such as an editor that shows them. Nothing does so today.

## How it is built

Jolt Physics is a submodule in `external/jolt-physics`. Its options are set in
[cmake/JoltPhysics.cmake](../cmake/JoltPhysics.cmake).

| Option | Set to | Reason |
|---|---|---|
| `ENABLE_ALL_WARNINGS` | Off | Jolt turns its warnings into errors, which breaks the build whenever a compiler learns a new warning |
| `OVERRIDE_CXX_FLAGS` | Off | Jolt replaces the flags of a build type with its own otherwise |
| `CROSS_PLATFORM_DETERMINISTIC` | On | The same input gives the same result on every platform and compiler. It costs a few percent of speed |
| `CPP_RTTI_ENABLED`, `CPP_EXCEPTIONS_ENABLED` | On | neon-jolt derives from classes of Jolt, and is compiled as the engine is. Compiled otherwise, the type information of the classes of Jolt is missing when linking |
| `USE_SSE4_1`, `USE_SSE4_2` | On | What every 64-bit processor of the last fifteen years has |
| `USE_AVX`, `USE_AVX2`, `USE_AVX512`, `USE_LZCNT`, `USE_TZCNT`, `USE_F16C`, `USE_FMADD` | Off | Jolt asks for AVX2 by default, and what is built with it does not start on a processor without. ARM ignores all of them |
| `INTERPROCEDURAL_OPTIMIZATION` | Off | It ties a static library to the linker of the compiler that built it |
| `DEBUG_RENDERER_IN_DEBUG_AND_RELEASE`, `PROFILER_IN_DEBUG_AND_RELEASE`, `ENABLE_OBJECT_STREAM`, `ENABLE_INSTALL` | Off | Parts of Jolt that the engine does not use |

| | |
|---|---|
| Samples, unit tests, and viewer of Jolt | Not built. Jolt declares them when it is the top of a build only |
| What neon-jolt is compiled with | What Jolt declares on its target: the instruction sets and `JPH_CROSS_PLATFORM_DETERMINISTIC`. A private link hands them to neon-jolt and no further. And `-ffp-contract=off`, which Jolt sets for itself and does not declare |
| Threads | One less than the machine has, 16 at most. The result does not depend on how many there are |

## Limits

| Limit | |
|---|---|
| Bodies | 65,536, characters included |
| Different sets of layers and masks | 65,535 |
| Two triggers | Report nothing of each other |
| A character | Reports the bodies it touches through `on_floor`, `on_wall`, `on_ceiling`, and `floor`. It reports a dynamic body and a trigger as events, and a static body and a kinematic one not |
| A character | Stays upright. Its shape is not turned with its entity |
| The shapes of a body | Are read when it is created. A collider that is added, changed, or removed later changes nothing |
| The scale of an entity | Is read when its body is created |
| Joints, such as a hinge | Not there |
| Height fields, soft bodies, vehicles | Jolt has them. The interface does not |
| Locking an axis of a body | Not there |
| A shape that is cast along a way | Not there. A ray and an overlap are |

## How it was checked

| Check | Result |
|---|---|
| 89 checks of `Jolt_PhysicsSystem` through the interface | Pass. A box falls and comes to rest, a sphere rolls down a slope, a character stops at a wall and slides along it and is not pushed by a body of 500 kg, a trigger reports enter and leave once each and changes nothing of what passes, layers and masks, rays and overlaps, a mesh is refused on a dynamic body, bodies are released |
| The same world twice, with 21 bodies, a mesh, a trigger, and a character, for 300 steps | The same state down to the last bit, and the same events in the same order |
| 73 checks of `PhysicsSimulation` with a physics that is a fake | Pass |
| 43 checks of the formats of the components | Pass. Reading, writing, reading what was written, and every message |
| 21 checks of `Rotation` | Pass |
| 19 checks of `FixedClock` | Pass |
| 21 checks of the world with Flecs, Jolt, and a scene recipe | Pass |
| The same scene at 30, 60, 144, and 1000 frames per second, for four seconds, with a force that a system of a game asks for | 240 steps each. Every body is in the same state after every step, down to the last bit |
| The same scene with frames between 0.1 and 120 milliseconds | The steps of the time that passed, and the same state after every one of them |
| A frame of ten seconds | 8 steps, and 592 given up. The state is that of 8 steps of any other run |
| A crate that falls, drawn at 1000 frames per second | It is drawn lower in every frame, not only in the 30 that took a step |
| The scene of the runtime, rendered twice without a window | The same images, byte for byte |
| The scene of the runtime at 30, 60, 120, and 240 frames per second | The same images one and two seconds in, byte for byte |
| The demo scene | The same image as before the physics, byte for byte |
| Every test program started by itself, which runs its tests in one process | Pass |

The checks are part of the repository, and `ctest` runs them. See the
[development guide](development.md#tests).

It was built and run on macOS. **It was not built on Linux and Windows.**

## Open questions

- Whether `Rotation` stores a quaternion. It is needed once something reads
  the angles of a body, changes them, and writes them back.
- How a script reads events. A list fits what a script can walk through.
- Whether a collider without a body should count as a static body, as in
  Unity. It is a warning today.
- Shapes that change while a body lives: a collider that is added or removed,
  and a scale that changes.
- Joints, locked axes, and casting a shape along a way.
- Whether a character reports every body it touches as events.
- Drawing the shapes of the physics, to see what collides. Jolt has a
  renderer for it, which is turned off.
- Whether the steps per second become an option of the command line.
- The lighting of the scene of the runtime. The bunny is black, since its
  model has no normals.
- Several worlds of physics at once, such as one for each scene that is open
  in an editor. One `Jolt_PhysicsSystem` is one world, and several can exist.
