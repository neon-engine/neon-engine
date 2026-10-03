# Scripting

The code of a game is written in Lua 5.4 and lives in the project, under
`scripts/` by convention and anywhere in fact. The engine finds the scripts
itself, registers what they declare, and runs them next to its own systems.
Nothing names an entry script, nothing lists the files, and nothing says
where they are: a script is loaded because it is there, in the order of its
path.

A script declares a **component**, data that an entity carries and a scene
writes, or a **system**, behaviour over every entity that carries the
components it names, or both. That is the same split the engine's own code
has, see [entity-component-system.md](entity-component-system.md): a
component holds, a system does. Both extend a class the engine provides,
and the engine checks the contract when the file loads, so that a mistake
is reported at start-up with the file and the line, and never found by a
player.

## A door

```lua
-- scripts/door.lua declares Door

local Door = Component:extend {
  open_height = 2.5,
  speed       = 2.0,
  open        = false,
  sound       = "assets://sounds/door.wav",
}

local DoorSystem = System:extend(Door)   -- a system over every entity with a Door

function DoorSystem:ready(entity, door)
  door._closed_y = entity.Transform.position.y
end

function DoorSystem:update(entity, door, dt)
  local target = door.open and door._closed_y + door.open_height or door._closed_y
  entity.Transform.position.y = math.move_toward(entity.Transform.position.y, target, door.speed * dt)
end

function DoorSystem:on_trigger_enter(entity, door, other)
  if other:has("Player") then door.open = true end
end

function DoorSystem:on_trigger_exit(entity, door, other)
  if other:has("Player") then door.open = false end
end

return Door, DoorSystem
```

A scene places it like any component of the engine, and writes only what
differs from the script's defaults:

```yaml
- name: lab-door
  components:
    Transform:
      position: [4, 0, -2]
    Trigger: Default
    Collider:
      shape: box
      size: [1.2, 2.5, 1.2]
    Door:
      speed: 3.5
```

## Files

Every `*.lua` anywhere under `assets://` is run once when the world starts,
before the scene is read, in the order of the paths: the files of a folder
in the order of their names, then the folders below it in turn. The order
is the same on every platform. `scripts/` is where the
[project layout](project-layout.md) suggests keeping them, and a level may
as well keep its scripts next to its scene; the engine does not mind.

| File | Returns | Declares |
|---|---|---|
| `door.lua` | A component, systems, or both; or a table, which makes it a module | `Door` from the file's name |
| `door.component.lua` | One component and nothing else | `Door` |
| `door.system.lua` | One or more systems and nothing else | Systems over components declared anywhere |
| `scripts/lib/tween.lua` | A table, a function, or nothing | A module for `require "scripts.lib.tween"` |
| `neon.d.lua`, `*.d.lua` | Not run | Definitions for an editor, see below |

The name of a component is the name of its file, in PascalCase: `door.lua`
and `door.component.lua` declare `Door`, `turret_gun.lua` and
`turret-gun.lua` declare `TurretGun`. A file that wants another name gives
it as text in front of the fields, `Component:extend("Door", { ... })`. A
component's name is letters and digits with a capital first, as the
engine's are, and no file may declare a component the engine or another
file has.

A folder may hold one component per file, or one component and its system,
or a feature's files side by side, `scripts/door/door.component.lua` and
`scripts/door/door.system.lua`. The engine is not particular about the
folders; it is particular about the contracts.

## Components

`Component:extend { fields }` declares a component. Every entry of the table
is a field with its default, and the default says what the field holds:

| Default | The field holds | In a scene |
|---|---|---|
| `true`, `false` | A bool | `open: true` |
| `3` | A whole number | `count: 3` |
| `2.5` | A number | `speed: 2.5` |
| `"text"` | Text, such as a virtual path | `sound: assets://sounds/door.wav` |
| `vec2(1, 2)`, `vec3(1, 2, 3)`, `vec4(1, 2, 3, 4)` | A vector of two, three, or four numbers | `at: [1, 2, 3]` |
| `color(1, 0, 0)` | A colour | `tint: [1, 0, 0]` |
| `quat()`, `quat(x, y, z, w)`, `quat.from_euler(pitch, yaw, roll)` | A quaternion | `turn: [0, 0, 0, 1]` |
| `mat3()`, `mat4()` | A matrix, the identity to start with | `basis: [[1, 0, 0], [0, 1, 0], [0, 0, 1]]` |

The fields are laid out in memory in the order of their names, each at the
alignment of what it holds, so a component a script declares is one of the
engine's: the store keeps it in its arrays, a scene reads and writes it
through [reflection](reflection.md) with the same checks and the same
messages, a prefab carries it, and `Save` writes it back. Nothing is copied
between the store and the script: a field is read and changed in place.

A component with no fields, `Component:extend {}`, marks an entity, as
`Persistent` does.

What a component cannot hold yet: a list, a choice of words, a reference to
another entity (#211), and the narrower whole numbers (a script's whole
number is an `Integer`; a byte or a short is reached on the engine's
components, not declared). A field named with an underscore first is
refused, since that is for private state (below), and a function is
refused, since behaviour belongs in a system.

## Systems

`System:extend(...)` declares a system over the components named, which are
given as text, `System:extend("Door", "Transform")`, or as the class in
hand, `System:extend(Door)`. A system may run over the engine's components
alone: `System:extend "Transform"` adds behaviour to every entity that has
one. The components have to exist once every file is read, from the engine
or from a script; a name nothing declares is reported with what the scripts
do declare.

A system is its hooks, and nothing else. A key that is not a hook, and a
hook with the wrong number of parameters, are refused with the list of
hooks:

| Hook | Called | With |
|---|---|---|
| `ready(entity, components...)` | Once, when an entity with the components is first seen | |
| `update(entity, components..., dt)` | Every frame | The seconds of the frame |
| `step(entity, components..., dt)` | Every step of the world, see [the fixed clock](entity-component-system.md#systems) | The seconds of a step, always the same |
| `removed(entity)` | Once, when an entity the system saw no longer has the components | The entity alone: the components are gone |
| `on_trigger_enter(entity, components..., other)` | A body entered this entity's `Trigger` | The other entity |
| `on_trigger_exit(entity, components..., other)` | A body left it | |
| `on_collision(entity, components..., other, point, normal)` | This entity's body began to touch another | The other entity, where, and the direction from this one to the other |

`self` is the system's class, so a system keeps what is its own on it. What
is an entity's own goes on the component, in a field the scene can write,
or in a private field: a name with an underscore first, `door._closed_y`,
which the engine keeps for that entity and that component, never reads from
a recipe, and forgets when the entity goes.

The systems of the scripts run in the place the engine gives them: after
the engine's systems that react to input, before the physics takes its
steps, so that what a script asks for in a step is part of it. Within that
place they run in the order their files were loaded. What the physics
reported in a frame, a body entering a trigger or two bodies touching,
reaches the hooks before `update` of that frame.

A hook that fails is reported once, with the system, the hook, the entity,
and Lua's message with a traceback, and is switched off until the scripts
are loaded again. The game goes on; the runtime's exit code says that
something went wrong, as for every error.

## Entities and components in a script

A hook is handed the entity and one handle per component. Both are small
and stay valid: a handle kept in a variable across frames still works, and
says when the entity lost the component.

| On an entity | Gives |
|---|---|
| `entity.id`, `entity.name`, `entity.path` | What the entity is called, and its names from the top with slashes |
| `entity.Transform`, `entity.Door` | The component by its name, or `nil` when the entity has none. A name no component has is an error |
| `entity:has("Door")`, `entity:get("Door")` | The same, said in full |
| `entity:remove("Door")` | Takes a component away. It takes effect when the hook's query is done |
| `entity:parent()`, `entity:children()` | What is above and below |
| `entity:alive()`, `entity:destroy()` | Whether it is still there, and the end of it with its children |

| On a component | Does |
|---|---|
| `door.speed` | Reads a field. A name the component lacks is an error that lists the fields |
| `door.speed = 3` | Changes a field, in place. A value of another kind, or outside what the field allows, is an error |
| `door._timer` | A private field, see above |

A vector or a colour read from a component is bound to it: `p = t.position;
p.y = 1` changes the Transform. One made in the script, `vec3(1, 2, 3)` or
`a + b`, is a value of its own until it is assigned to a field. Vectors of
two, three, and four numbers add, subtract, scale, and have `length`,
`normalized`, `dot`, `distance`, and `copy`; `vec3` has `cross` too. A
colour has `r`, `g`, `b`, `a`. A quaternion has `x`, `y`, `z`, `w`,
`normalized`, `inverse`, `to_euler`, and `*` with a quaternion or a vector;
`quat.from_euler(pitch, yaw, roll)` makes one from degrees as a Transform
reads them. A matrix is reached by `m:get(row, column)` and `m:set(row,
column, value)`, rows and columns from 1, with `m:row(i)` as a vector and
`*` with a matrix or a vector. A whole number of any width of the engine's
components is a Lua integer, refused outside the range of its kind; a
character is a string of one; a whole vector is read as a `vec2` or `vec3`
and written back whole. A list is a table, and the table is a copy: reading
`gauge.steps` copies the list out, and a change to the table reaches the
component when the table is assigned back, `gauge.steps = steps`. Every
other value above reaches the component in place. A handle that reaches a
list in place is a follow-up (#99); until then a list a hook walks every
frame is the one cost to know of.

## What a script reaches of the engine

| Library | Function | Does |
|---|---|---|
| `world` | `world.each("Player", "Transform")` | Iterates the entities that carry every named component, `for entity, player, transform in ...`. The entities are taken at the start of the loop |
| | `world.find("player/camera")` | An entity by its path, or `nil` |
| | `world.create(name, parent)`, `world.destroy(entity)` | Makes and ends entities |
| `input` | `input.is_down(action)`, `input.pressed(action)` | The actions of the [input map](input.md) |
| | `input.amount(action)`, `input.axis(action)`, `input.axis3(action)` | A trigger, a stick as two numbers, a sensor as three |
| `scene` | `scene.load(path)` | Asks for another scene, read at the start of the next frame, see [scenes.md](scenes.md#changing-the-scene) |
| `log` | `log.debug(...)`, `log.info(...)`, `log.warn(...)`, `log.error(...)` | The engine's log, with a space between the values. `print` is `log.info` |
| `math` | `math.move_toward(from, to, by)`, `math.clamp(value, low, high)` | On top of Lua's `math` |
| | `vec3(x, y, z)`, `vec3(n)`, `vec3()`, `color(r, g, b, a)` | Values |
| | `require "scripts.lib.tween"` | A module, by its path below `assets://`, loaded once |

`require` takes the path below `assets://` with dots for the slashes and
resolves through the engine's file system, inside the assets alone: a name
that climbs out, or one of a file that declares a component or a system, is
refused. A module is loaded once; a file the engine already ran is handed
over without running it again.

### The sandbox

A script has Lua's base functions, `string`, `table`, `math`, `utf8`, and
`coroutine`, and nothing else: no `io`, no `os`, no `debug`, no `package`,
and no `load`, `loadstring`, `dofile`, or `loadfile`. Files, time, random
numbers, and the log go through the engine. A shipped game loads its
scripts from its own assets, as it loads its scenes.

## For an editor

`lib/neon-lua/types/neon.d.lua` describes `Component`, `System`, every hook,
the libraries, and the engine's components for the
[Lua language server](https://luals.github.io), which VS Code and others
use. A project's `.luarc.json` names it:

```json
{ "workspace.library": ["../../lib/neon-lua/types"] }
```

With it, `door.opne` is underlined before the game runs, and `entity.`
completes to the engine's components. A script may add `---@class Door :
Component` above its own component for the same on its fields.

## How it is built

| Part | Where | Holds |
|---|---|---|
| `ScriptContext`, `ScriptSystem` | neon-core, `neon/scripting/` | What the engine sees of the scripts, in no language: load, start, run the hooks, hand the physics events over. The base of the backends |
| `ScriptField`, `ScriptComponentLayout` | neon-core, `neon/scripting/` | A field a script declares, and the layout that makes the fields a component of the store, with its `TypeInfo` for reflection and its `ComponentFormat` for scenes |
| `ScriptRunning` | neon-core, `neon/world-system/ecs/systems/` | The system of the world that loads the scripts when components are registered and runs their hooks in its place |
| `FileSystemContext::ListFiles` | neon-core | How the scripts are found |
| `Lua_ScriptSystem` | neon-lua, `neon/scripting/` | The Lua backend: the state, the files, the contracts, the hooks |
| `lua-classes`, `lua-sandbox`, `lua-libraries` | neon-lua | `Component` and `System`, what of Lua a script gets, and `world`, `input`, `log`, `scene` |
| `lua-entity-handle`, `lua-component-handle`, `lua-vec3-handle`, `lua-color-handle` | neon-lua | What a script holds of an entity, a component, a vector, a colour |
| `lua` | `external/lua`, built by `lib/neon-lua/CMakeLists.txt` | Lua 5.4, as C, private to neon-lua |

The language is a backend behind `ScriptContext`, as the physics is behind
`PhysicsContext`: nothing outside neon-lua includes Lua. A second language
is a second backend over the same `ScriptComponentLayout` and the same
handles, which is how #93 left the door open.

A component a script declares is registered with the store through
`ComponentInfo` whose functions hold the layout, which is why those
functions are `std::function` and not pointers. The store never sees a C++
type, and never did; now nothing else has to either.

## What is open

- **Components from Lua in the editor (#100).** The description is there;
  what an editor shows for a field is the default's kind and a sentence the
  script does not give yet.
- **Lists, choices, entity references, precise numbers** as fields.
- **Spawning**: `world.spawn(path, parent, overrides)` once #229 lands, with
  `overrides` in the form of a scene's `components:` block.
- **Audio** from a script, `audio.play(path, entity)`, through the groups
  of [audio.md](audio.md).
- **Hot reload** while the game runs (#104): the scripts are read once at
  start.
- **Bytecode and embedding** when a game is exported (#84, #93): a shipped
  game loads the same files, compiled or not.
- **Speed.** A hook is one call per entity per frame, with a handle per
  component filled in and no allocation. A system over thousands of
  entities is better written over the block, which the interface does not
  offer yet (#99).
