# Entity component system

This note records how the world of Neon Engine is built, why, and what is
still open.

**Current decision:** the world is made of entities, components, and systems.
The engine has its own interface for it, with one implementation built on
[Flecs](https://github.com/SanderMertens/flecs). The node system that came
before is removed.

## The idea

| Piece | What it is |
|---|---|
| Entity | Names one thing in the world. It is a number and holds nothing |
| Component | Data that an entity carries, such as a `Transform`. It has no behaviour |
| System | Behaviour. It works on every entity that carries the components it asks for, once per frame or once per step of the world |

What an entity is follows from what it carries. A box of the start scene is
an entity with a `Transform` and a `Renderable`. The player is one with a
`Transform` and a `Spectator`.

### Against the node system

The node system built things from nodes, in the way Godot does. A node held
its data and updated itself.

| | Nodes | Entities and components |
|---|---|---|
| A part of a thing | An object with data and behaviour | Data only |
| Behaviour | In each node, called while the tree is walked | In systems |
| Hierarchy | The tree is both what a thing is made of and what it moves with | An entity has a parent. What it is made of is its components |
| A scene | Objects that are constructed in code | A list of entities with components and their values |

The last row is the reason for the change. A world that is plain data can be
written to a file and read from one, shown in an editor, changed by a script,
and inspected by an agent.

## What exists today

| Piece | Location | Role |
|---|---|---|
| `EntityStore` | neon-core | The interface. Entities, components, hierarchy, and queries |
| `EntitySystem` | neon-core | The interface of behaviour |
| `Scene` | neon-core | The interface of what fills a store |
| `EntityWorld` | neon-core | Runs the systems every frame, changes the scene when one is asked for, and spawns a prefab through the scene when a game asks, see [prefabs.md](prefabs.md#spawning-at-run-time). It is the `WorldSystem` of the runtime |
| Components and systems of the engine | neon-core | Listed below |
| `Flecs_EntityStore` | neon-flecs | The implementation |
| `SceneFile` | neon-core | The scene that is read from a file. See [scenes.md](scenes.md) |

Nothing outside neon-flecs includes a header of Flecs. The library is linked
privately, so the compiler refuses it anywhere else.

### Components

| Component | Holds | In |
|---|---|---|
| `Transform` | Position, rotation, and scale relative to the parent, and where that puts the entity in the world | `neon/common/transform.hpp` |
| `Renderable` | The model, shader, textures, and material to draw | `neon/world-system/ecs/components/` |
| `Camera` | Field of view and the distances between which things are visible | same |
| `Light` | A light source | same |
| `Spectator` | The speeds the input moves the entity with | same |
| `Player` | The speeds the input walks, runs, jumps, and turns the entity with, how much it steers in the air, and where its eyes are, with their offset and how they glide over a step. See [physics.md](physics.md#the-player) | same |
| `RigidBody`, `Collider`, `Trigger`, `CharacterBody` | What the physics needs to know of an entity. See [physics.md](physics.md) | same |
| `Persistent` | The entity stays when the world changes scene. See [scenes.md](scenes.md#changing-the-scene) | same |
| `SceneExit` | The scene the world changes to when a body enters the entity's `Trigger` | same |

### Systems

A frame runs them in this order.

| Order | System | Does |
|---|---|---|
| 1 | `FixedUpdate` of every system | Once for every step of the world that the time of the frame asks for, which can be never |
| 2 | `SpectatorMovement` | Moves and turns entities with a `Spectator` by the input |
| 3 | Systems a game added | In the order they were added. `ScriptRunning` is one of them: it runs the systems the scripts declare, see [scripting.md](scripting.md) |
| 4 | `TransformPropagation` | Places every entity in the world, parents before children |
| 5 | `Interpolate` of every system | Places what is drawn between the last two steps |
| 6 | `GeometryBuilding`, then `RenderSubmission` | Builds the mesh of every entity whose `Geometry` has none yet, see [geometry.md](geometry.md), so that what a system spawned is drawn in the same frame; then hands the camera, the lights, and what is visible to the render pipeline |

A system that a game adds sees the input of the frame, and what it moves is
drawn where it was moved to.

A system has three functions that the world calls. Only `Update` has to be
written.

| Function | Called | With | For |
|---|---|---|---|
| `Update` | Once per frame | The time the frame took | What belongs to a frame: the input, a camera, what is shown |
| `FixedUpdate` | Once per step of the world | The length of a step, which is always the same | What the game is decided by: forces, velocities, timers |
| `Interpolate` | Once per frame, after every entity was placed | How far the frame lies between the last two steps | Placing what is drawn between two steps |

The world takes 60 steps in a second, whatever the frame rate is. Why, and
what belongs where, is in [physics.md](physics.md#the-time-step).

`PhysicsSimulation` is a system that an application adds, as `main.cpp` of
the runtime does, and so is `PlayerMovement`, which the runtime adds before
it, so that the velocity it sets is taken in the step that follows.

## Using it

Creating an entity:

```cpp
const auto player = store.CreateEntity("player");
store.Set(player, neon::Transform{.position = {0.f, 0.f, 2.f}});
store.Set(player, neon::Spectator{});

// the camera sits on the player, and so moves and turns with it
const auto camera = store.CreateEntity("camera", player);
store.Set(camera, neon::Transform{});
store.Set(camera, neon::Camera{});
```

A component of a game:

```cpp
struct Health
{
  int points = 100;
};
```

A system, which registers the component it brings in `Register` and makes
its queries in `Initialize`. The world calls `Register` of every system
before `Initialize` of any, so that a system finds the components of another
whichever comes first:

```cpp
class Regeneration final : public neon::EntitySystem
{
  neon::QueryId _query = 0;

public:
  void Register(neon::EntityStore &store) override
  {
    store.Register<Health>("Health");
  }

  void Initialize(neon::EntityStore &store) override
  {
    _query = store.Query<Health>();
  }

  void Update(neon::EntityStore &store, const double delta_time) override
  {
    store.Each(_query, [](const neon::EntityBlock &block)
    {
      auto *health = block.Column<Health>(0);
      for (std::size_t i = 0; i < block.count; i++)
      {
        health[i].points = std::min(health[i].points + 1, 100);
      }
    });
  }
};
```

It is added to the world before the world is initialized:

```cpp
world.AddSystem(std::make_unique<Regeneration>());
```

### Turning a component off

A component of an entity is turned off and on without being removed:

```cpp
store.SetEnabled<neon::Renderable>(nail, false);   // not drawn, and everything about it is kept
store.SetEnabled<neon::Collider>(nail, false);     // walked through
store.SetEnabled<neon::Renderable>(nail, true);    // as it was
```

| While it is off | |
|---|---|
| Queries | Pass the entity over, as they pass over one that has no such component |
| `Has<T>()`, `Get<T>()` | `false`, and `nullptr`: it is not there for whoever asks |
| `IsEnabled<T>()` | `false`. It is `true` only for a component the entity has and that is on |
| What it holds | Kept. `GetComponentData()` reaches it, and so do a script, an extension, and a document, which read and write its fields as before |
| What it keeps outside the store | Handled by the component, with `on_toggle`, see the table below |

It is data of the component, and not of the entity: an entity is hidden and
still collides, or collides and is not seen, by which of its components is
on. For something that comes and goes often, a projectile, a spark, turning
its components off and on again costs next to nothing, where destroying it
and making another gives the renderer and the physics work each time.

Destroying an entity while a game runs is still what it was:

```cpp
store.DestroyEntity(nail);   // gone, with its children, its render object, and its body
```

It is right for what is gone for good, a level that is left, an enemy that
will not be seen again. For what is used over and over it is better to turn
it off and keep it, which is what a pool does: see [pools.md](pools.md).

What each component of the engine does when it is turned off:

| Component | Off | On again |
|---|---|---|
| `Renderable` | Not drawn. It keeps its render object | Drawn |
| `RigidBody` | Its body is taken out of what the physics simulates, and kept with its shapes: nothing touches it, moves it, or finds it with a ray | Put back where its entity is by then, with the velocity its component says |
| `Collider` | No shape of its body, which is given its other shapes anew. A body whose colliders are all off is taken out of the physics and kept, and put back when one is on. While its body is off as well nothing is done, so what waits in a pool keeps its shapes | A shape of its body again |
| `Trigger`, `CharacterBody` | Taken out of what is simulated and kept, as a body is | Put back where its entity is by then |
| `Joint` | Holds nothing, and is kept | Holds again |
| `SoundSource` | Silent: its sound is stopped, and kept | Plays from the start when it says `playing` |
| `Ui`, `UiSurface` | Hidden, and kept as it is | Shown again, with nothing read anew |
| `Script` | Its hooks are not called. It is not told that it was turned off: a script that is off does nothing, and that includes hearing about it | Its hooks are called again |
| `Camera`, `Light`, `Sky`, `Geometry`, `SoundListener`, `Player`, `Spectator`, and the rest | Passed over by the systems that read them: a camera draws nothing, a light lights nothing | Read again |

Nothing is destroyed when a component is turned off, and nothing is made
anew when it is turned on: what the component keeps in another system is
kept there, out of use. A scene the engine writes says `enabled: false` for
a component that is off, with everything it holds, and nothing for one that
is on.

## How the interface is shaped

An entity component system is harder to put behind an interface than a window
or a renderer. The decisions below follow from that.

| Decision | Reason |
|---|---|
| A component is known by an id, a size, and four functions | A C++ type cannot pass through a virtual function. It also means that a component can be declared by something that is not C++, such as a script |
| A query hands over blocks, not entities | One call per block. A call per entity would cost more than the library saves |
| A block holds plain arrays | A system loops over memory that lies together, which is where the speed of an entity component system comes from |
| The templates `Set<T>`, `Get<T>`, and `Query<T...>` are part of the interface class | They turn a type into an id and call the virtual functions. A backend does not implement them |
| Changes during a query are held back | Entities must not move in memory while they are walked. Creating, destroying, setting, and removing take effect when the query is done. Writing to the arrays of a block takes effect at once |
| Hierarchy is part of the interface | Placing entities needs parents before children, and the store knows that order |
| A component can ask to be told when it is removed | `Renderable` refers to what the renderer holds. That is released when the entity is destroyed, without a system having to watch for it |
| A name with a dot is one name | Paths are written with forward slashes, as everywhere else in the engine: `player/camera` |

### Limits

There is no limit on how many kinds of component a store knows, or on how
many components one entity carries. A query names between 1 and 8 components,
which is `EntityBlock::Max_Components`. Flecs itself allows 32.

Flecs looks up the first 256 ids of a world faster than the ones after them.
The components of `Flecs_EntityStore` are given ordinary ids today, so they do
not use that faster lookup.

## Why Flecs

[ecs_benchmark](https://github.com/abeimler/ecs_benchmark) measured Flecs
4.0.1 and EnTT 3.13.2, among others. At about 16,000 entities:

| Operation | EnTT | Flecs |
|---|---|---|
| Run 7 systems over all entities | 569 µs | 120 µs |
| Create the entities | 623 µs | 6,365 µs |
| Remove and add a component | 408 µs | 3,890 µs |
| Look up one component by entity | 51 µs | 210 µs |

Flecs is faster at what happens every frame and slower at changes. The numbers
did not decide it, because both are fast enough for a game. What decided it:

| | Flecs | EnTT |
|---|---|---|
| Components that are declared at run time | What its C interface is made for | Possible, but the slow path. 1,455 µs in the first row above |
| Hierarchy | Built in, with queries that order by it | Left to the application |

An interface that hides the library has to name components at run time, so
Flecs works the way the interface has to work.

**What follows for games:** giving a component to an entity or taking one away
moves the entity in memory. A state that changes often, such as being stunned,
is better kept as a value in a component than as a component that comes and
goes.

## How it was checked

| Check | Result |
|---|---|
| The demo scene, as entities and as nodes | The same image, byte for byte |
| 35 checks of the store through its interface | Pass. They cover components with strings, hierarchy order, changes during a query, an exception during a query, destroying children, and cleaning up |
| Memory after the checks | Nothing leaked |

The checks are not part of the repository. It has no test framework yet, which
is the first step of the [roadmap](roadmap.md#order).

## Open questions

- The order of systems. There are four fixed places today. A game with many
  systems will want to say what runs before what.
- Scripts. A component that is declared in Lua fits the interface. How a
  script reads a block has to be decided.
- Several cameras. The last one that is handed over wins.
- Threads. Every system runs on the main thread.
- Whether systems of the engine, such as `SpectatorMovement`, belong in the
  engine or in a game.
- A rotation that is wanted relative to the world and not the parent.
