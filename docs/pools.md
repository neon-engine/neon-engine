# Pools

What comes and goes often in a game, a projectile, a spark, an enemy of a
wave, is not made and destroyed each time: making an entity gives the
renderer and the physics work, and so does destroying it. A pool holds a
fixed number of instances that are made ahead, hands one out when it is
asked, and takes it back when it is done with.

A pool is an entity with the component `PoolManager`.

## Contents

| Section | What it covers |
|---|---|
| [In a recipe](#in-a-recipe) | What a pool is told to hold |
| [Asking a pool](#asking-a-pool) | Taking an instance and giving it back, in C++, Lua, and an extension |
| [What waits and what is out](#what-waits-and-what-is-out) | What a pool turns off and on |
| [When a pool runs out](#when-a-pool-runs-out) | It hands out none, and says so |
| [Instances made in code](#instances-made-in-code) | A game that builds its entities itself |
| [Decisions](#decisions) | Why it is shaped this way |

## In a recipe

```yaml
- name: projectiles
  components:
    PoolManager:
      _entries:
        - source: assets://prefabs/nail.prefab.yml
          count: 64
        - source: assets://prefabs/rocket.prefab.yml
          count: 8
```

| Name | Holds |
|---|---|
| `source` | Where the instances of a kind come from, named as a file system names things. `assets://...prefab.yml` is a prefab, which is spawned `count` times below the pool. `instance://` and the path of an entity, as `instance://armory/lamp`, is an entity that is there already, which is taken in as the one instance |
| `count` | How many instances the pool holds of it. 1 for an entity that is there: making more of one takes cloning, which is #406 |

`_entries` is read once, when the pool is filled, in the first frame it is
there. It is not a field of the component, which the line in front of its
name says: a script and an extension ask the pool what it holds, see below.

The instances of a prefab are entities below the pool, named after the
prefab and numbered: `projectiles/nail 1` to `projectiles/nail 64`. A name
is an address in this engine, so two entities below one parent cannot share
it. Nobody has to find one by its name: a pool hands out the entity.

## Asking a pool

A kind is asked for by its `source`.

| | C++ | Lua | An extension |
|---|---|---|---|
| Take one | `PoolSystem::Acquire(store, pool, kind)` | `pool:pool_acquire(kind)` | `world.AcquireFromPool(pool, kind)` |
| Give it back | `PoolSystem::Release(store, instance)` | `instance:pool_release()` | `world.ReleaseToPool(instance)` |
| How many wait | `PoolSystem::GetFreeCount(store, pool, kind)` | `pool:pool_free_count(kind)` | `world.CountFreeInPool(pool, kind)` |
| How many there are | `PoolSystem::GetCount(store, pool, kind)` | | `world.CountInPool(pool, kind)` |
| Which kinds | `PoolSystem::GetKinds(store, pool)` | `pool:pool_kinds()` | |

```lua
local nail = pool:pool_acquire("assets://prefabs/nail.prefab.yml")
if nail then
  nail.Transform.position = muzzle
  nail.RigidBody.linear_velocity = { 0, 0, -30 }
end

-- when it hits
nail:pool_release()
```

A pool is a queue: what was given back first is handed out first. Whoever
takes an instance places it, and gives it back when it is done.

## What waits and what is out

An instance that waits has its `Renderable`, its `Collider`, and its
`RigidBody` turned off, on itself and on everything below it, see
[entity-component-system.md](entity-component-system.md#turning-a-component-off).
It is not drawn, nothing collides with it, and its body is out of what the
physics simulates, and it keeps everything about it: its render object, its
body, its shapes. One that is handed out has them turned on, and its body is
put where its entity is, with the velocity its component says.

Its `Transform` stays on, so that an instance is placed before or after it
is taken. Its other components are left as they are.

## When a pool runs out

A pool never makes more than it was told to hold. When every instance of a
kind is out it hands out nothing: `No_Entity`, `nil`, `0`. The game goes on,
and what would have been shown is not.

It says so in the log, once for a kind until one of it is given back:

```
The pool 'projectiles' ran out of assets://prefabs/nail.prefab.yml: all 64 are out, and it hands out none until
one is given back. It holds as many as it was told to
```

Whoever sets up a pool is expected to know how many it needs, and the log
says when that was too few. A pool that makes more when it runs out is #407.

## Instances made in code

A game that builds its entities itself, and has no prefabs, hands a pool
what it made, under a kind that starts with `instance://`:

```cpp
const Entity pool = world.CreateEntity("projectiles");
world.AddComponent(pool, "PoolManager");

std::vector<Entity> nails;
for (int i = 0; i < 64; i++) { nails.push_back(MakeNail(world, pool, i)); }
world.AddToPool(pool, "instance://nail", nails);

const Entity nail = world.AcquireFromPool(pool, "instance://nail");
```

They keep the names they were made with, are turned off, and wait. More of
a kind the pool has are taken on top of what it holds.

## Decisions

| Decision | Why |
|---|---|
| A pool is a component on an entity | It is data of the world like anything else: a scene writes it, a level has its own, and it goes with its entity |
| It holds a fixed number | What a pool is for is that nothing is made while the game runs. A pool that grows hides that it was set up too small, until the frame in which it grows |
| A kind is named by where it comes from | A prefab has a path already, and `instance://` says as plainly that these are instances that are there, and not a file they are made from |
| Its instances are turned off, not moved away | Nothing is drawn, simulated, or found for them, where an entity that is far away is still all three |
| Running out is said by the pool, not by who asked | A script, an extension, and C++ ask the same way, and none of them has to remember to say it |
| `_entries` is no field | It is what a pool is made from, read once. What a pool holds changes while the game runs, and is asked of the pool |
