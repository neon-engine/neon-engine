# Scripting

The code of a game is written in Lua and lives in the project, under
`scripts/` by convention and anywhere in fact. The engine finds the scripts
itself, registers what they declare, and runs them next to its own systems.
Nothing names an entry script, nothing lists the files, and nothing says
where they are: a script is loaded because it is there, in the order of its
path.

A script declares a **component**, data that an entity carries and a scene
writes, or a **system**, behavior over every entity that carries the
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
  if other:has_component("Player") then door.open = true end
end

function DoorSystem:on_trigger_exit(entity, door, other)
  if other:has_component("Player") then door.open = false end
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
| `integer(3)` | A whole number | `count: 3` |
| `3`, `2.5` | A number | `speed: 2.5` |
| `"text"` | Text, such as a virtual path | `sound: assets://sounds/door.wav` |
| `vec2(1, 2)`, `vec3(1, 2, 3)`, `vec4(1, 2, 3, 4)` | A vector of two, three, or four numbers | `at: [1, 2, 3]` |
| `color(1, 0, 0)` | A color | `tint: [1, 0, 0]` |
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
refused, since behavior belongs in a system.

## Systems

`System:extend(...)` declares a system over the components named, which are
given as text, `System:extend("Door", "Transform")`, or as the class in
hand, `System:extend(Door)`. A system may run over the engine's components
alone: `System:extend "Transform"` adds behavior to every entity that has
one. The components have to exist once every file is read, from the engine
or from a script; a name nothing declares is reported with what the scripts
do declare.

A system is its hooks and its [handlers](#what-the-user-interface-calls),
and nothing else. A key that is not a hook, and a hook with the wrong number
of parameters, are refused with the list of hooks:

| Hook | Called | With |
|---|---|---|
| `ready(entity, components...)` | Once, when an entity with the components is first seen | |
| `update(entity, components..., dt)` | Every frame | The seconds of the frame |
| `fixed_update(entity, components..., dt)` | Every step of the world, see [the fixed clock](entity-component-system.md#systems) | The seconds of a step, always the same |
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

`update` and `fixed_update` are one call per entity. The engine makes that cheap
underneath: one protected call per run of entities the store keeps
together, inside which the hook is called plainly for each, with the same
handles refilled, so a thousand entities cost a thousand function calls and
nothing else. A handle a hook is given is bound to the entity of that call
and no other, so it is not kept across calls.

A hook that fails is reported once, with the system, the hook, the entity,
and Lua's message with a traceback, and is switched off until the scripts
are loaded again. The game goes on; the runtime's exit code says that
something went wrong, as for every error.

## What the user interface calls

A button of a [user interface](user-interface.md) names the function it
calls when it is chosen, as a template of a web page does, and writes what
the function is handed:

```yaml
values:
  blinks_per_second: 2

root:
  type: panel
  children:
    - type: button
      text: Unlock
      on_click: unlock

    - type: button
      text: Alarm
      on_click: alarm(blinks_per_second)

    - type: button
      text: Paint it red
      on_click: paint('red', $event)
```

The function is a handler of a system, `handlers` being a table every
system has. In a handler `self` is the entity that shows the user
interface, and the parameters are exactly what the file wrote, in that
order:

```lua
local TerminalSystem = System:extend(Terminal)

-- on_click: unlock
function TerminalSystem.handlers:unlock()
  local terminal = self.Terminal
  terminal._unlocked = not terminal._unlocked
  ui.set_text_of(terminal.ui, "door", terminal._unlocked and "unlocked" or "locked")
end

-- on_click: alarm(blinks_per_second)
function TerminalSystem.handlers:alarm(blinks_per_second)
  self.Terminal._blinks_per_second = blinks_per_second
end

-- on_click: paint('red', $event)
function TerminalSystem.handlers:paint(color, event)
  local lamp = world.find_entity(self.Terminal.lamp)
  local player = event.instigator -- who clicked, or nil
end
```

A slider, a checkbox, a toggle, a select, an input, and a textarea name a
handler the same way with `on_change`, which the player changing them
calls, with what the element holds now in `$event.value`:

```yaml
- type: slider
  name: brightness
  min: 0
  max: 1
  step: 0.1
  on_change: dim($event)
```

```lua
-- on_change: dim($event)
function TerminalSystem.handlers:dim(event)
  self.Terminal._brightness = event.value -- a number, from 0 to 1
end
```

It is called once a frame for what the player changed in it, and never for
what the game sets: a value the element follows that a script sets with
`ui.set_number` and the rest, or a field set from C++ with `SetField()`.
See [user-interface.md](user-interface.md#what-the-player-changes).

| | |
|---|---|
| Who is called | The entity that shows the user interface, with a `Ui` or a `UiSurface`: every system that runs over that entity and has a handler of the name. Two terminals in a scene each get their own clicks |
| `self` | The entity that shows the user interface. Its components are reached through it, `self.Terminal`, and so is what a system keeps for itself on one, `self.Terminal._unlocked`. Any other entity is found by its name, `world.find_entity("vault/door")`. The system's class is reached by its name, `TerminalSystem`. A hook still has the class as `self` and the entity as its first parameter, which #442 brings in line |
| The parameters | The arguments, as many as the file wrote and in its order |
| A number, `'a text'` or `"a text"`, `true`, `false` | As written |
| A name, as `blinks_per_second` | A value of the user interface, which says what a number is for where a `2` would not, as what it holds when the button is chosen or the control changed, after a value the control follows took what the player chose: that of the file before the one every user interface shares. `nil` when there is none |
| `$event` | A table: `kind` (`"click"`, or `"change"` for `on_change`), `element` (the `name` of the element), `interface` (what the file writes as `ui`), `surface`, `value` for a change, and `instigator`, the entity the click or the change comes from: for a screen in the world the player who pointed at it, which is the nearest entity from the window's camera up that carries a `Player`, or the camera's own entity. `nil` for a click or a change on the window, which no entity made |
| `$event.value` | For a change alone: what the element holds after it, a number for a slider, `true` or `false` for a checkbox and a toggle, and a text for a select (the `value` of the option), an input, and a textarea |
| Any other entity | Its name as a text, `on_click: open('vault/door')`, and `world.find_entity(name)` in the handler |
| When | Before `update` of the same frame, where the world is the handler's to change |
| A handler that fails | Reported with the element that called it, and called again at the next click or change |
| Nothing to call | A warning: no handler of that name over the entity, or no entity shows the user interface, which is so for a file the runtime loaded itself, the pause menu or `--ui` |

A click or a change is not asked for in every frame. `ui` only sets values,
see [below](#what-a-script-reaches-of-the-engine). Only a button has
`on_click`, and `on_change` is on the controls named above; a radio has
none yet. A click and a change are the only events so far; listening from
code, and the elements themselves, are #436.

## Entities and components in a script

A hook is handed the entity and one handle per component. Both are small
and stay valid: a handle kept in a variable across frames still works, and
says when the entity lost the component.

| On an entity | Gives |
|---|---|
| `entity.id`, `entity.name`, `entity.path` | What the entity is called, and its names from the top with slashes |
| `entity.Transform`, `entity.Door` | The component by its name, or `nil` when the entity has none. A name no component has is an error |
| `entity:has_component("Door")`, `entity:get_component("Door")` | The same, said in full, named as `EntityStore` names them |
| `entity:remove_component("Door")` | Takes a component away. It takes effect when the hook's query is done |
| `entity:set_enabled("Renderable", false)`, `entity:is_enabled("Renderable")` | Turns a component off, and on again with `true`: it keeps what it holds and is not there while it is off, so an entity is hidden without being destroyed. See [entity-component-system.md](entity-component-system.md#turning-a-component-off) |
| `pool:pool_acquire(kind)`, `instance:pool_release()`, `pool:pool_free_count(kind)`, `pool:pool_kinds()` | Takes an instance from a pool, an entity with a `PoolManager`, or `nil` when all are out; gives one back; and what a pool holds. See [pools.md](pools.md) |
| `entity:get_parent()`, `entity:get_children()` | What is above and below |
| `entity:is_alive()`, `entity:destroy()` | Whether it is still there, and the end of it with its children |

| On a component | Does |
|---|---|
| `door.speed` | Reads a field. A name the component lacks is an error that lists the fields |
| `door.speed = 3` | Changes a field, in place. A value of another kind, or outside what the field allows, is an error |
| `door._timer` | A private field, see above |

A vector or a color read from a component is bound to it: `p = t.position;
p.y = 1` changes the Transform. One made in the script, `vec3(1, 2, 3)` or
`a + b`, is a value of its own until it is assigned to a field. Vectors of
two, three, and four numbers add, subtract, scale, and have `length`,
`normalized`, `dot`, `distance`, and `copy`; `vec3` has `cross` too. A
color has `r`, `g`, `b`, `a`. A quaternion has `x`, `y`, `z`, `w`,
`normalized`, `inverse`, `to_euler`, and `*` with a quaternion or a vector;
`quat.from_euler(pitch, yaw, roll)` makes one from degrees as a Transform
reads them. A matrix is reached by `m:get(row, column)` and `m:set(row,
column, value)`, rows and columns from 1, with `m:row(i)` as a vector and
`*` with a matrix or a vector. A whole number of any width of the engine's
components is a Lua integer, refused outside the range of its kind; a
character is a string of one; a whole vector is read as a `vec2` or `vec3`
and written back whole. A list of a component is reached in place too:
`#steps`, `steps[i]`, `steps[i] = v`, `steps:insert(v)`, `steps:insert(i,
v)`, `steps:remove(i)`, and `steps:clear()` touch the list where it lives,
and an index past the end is an error. An element that is a vector is a
copy of that element, written back with `steps[i] = v`. A list of a field
that is computed, with a get and a set of its own, has no address and comes
as a table, a copy, which is assigned back whole; none of the engine's
lists is such a field. Assigning a table to a list field replaces it.

What a script reads from a component is kept: `transform.position` is the
same handle every frame, bound again, and so is `entity.Transform`, so a
hook that reads them makes nothing Lua has to collect. What a script makes,
`vec3(1, 2, 3)` or `a + b`, is new each time; a hook over many entities
keeps such values to a few per turn.

## What a script reaches of the engine

The functions are the engine's own, named as the interfaces name them, in
the snake case Lua reads: `EntityStore::CreateEntity` is
`world.create_entity`, `InputContext::IsActionDown` is
`input.is_action_down`, `WorldSystem::LoadScene` is `world.load_scene`,
and a method of an entity is the store's function on it,
`entity:has_component("Door")`. What a name means in C++ it means in Lua.

| Library | Function | Does |
|---|---|---|
| `world` | `world.each("Player", "Transform")` | Iterates the entities that carry every named component, `for entity, player, transform in ...`. The entities are taken at the start of the loop |
| | `world.find_entity("player/camera")` | An entity by its path, or `nil` |
| | `world.create_entity(name, parent)`, `world.destroy_entity(entity)` | Makes and ends entities |
| | `world.load_scene(path)` | Asks for another scene, read at the start of the next frame, see [scenes.md](scenes.md#changing-the-scene). `scene.load_scene` is the same |
| `input` | `input.is_action_down(action)`, `input.was_action_pressed(action)` | The actions of the [input map](input.md) |
| | `input.action_axis2(action)`, `input.action_axis2(action)`, `input.action_axis3(action)` | A trigger, a stick as two numbers, a sensor as three |
| `ui` | `ui.set_text(name, text)`, `ui.set_number(name, number)`, `ui.set_flag(name, flag)` | Sets a value that files refer to as `{name}` |
| | `ui.set_text_of(interface, name, text)`, `ui.set_number_of`, `ui.set_flag_of` | The same for one user interface, where it wins over the shared value |
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

A script has Lua's base functions, `string`, `table`, `math`, `bit`, and
`coroutine`, and nothing else: no `io`, no `os`, no `debug`, no `package`,
no `ffi`, no `jit`, and no `load`, `loadstring`, `dofile`, or `loadfile`.
Files, time, random numbers, and the log go through the engine. A shipped
game loads its scripts from its own assets, as it loads its scenes.

## For an editor

`lib/neon-lua/types/neon.d.lua` describes `Component`, `System`, every hook,
the libraries, and the engine's components for the
[Lua language server](https://luals.github.io), which VS Code and others
use; in VS Code it is the extension `sumneko.lua`, which the repository
recommends. The `.luarc.json` at the root of the repository names the
folder of the definitions and the Lua the scripts run on:

```json
{
  "runtime.version": "LuaJIT",
  "workspace.library": ["lib/neon-lua/types"]
}
```

The path is from the folder of the `.luarc.json`; a project kept elsewhere
has one of its own with the path from there. With it, `Component:` and
`entity.` complete, a function of `world` that is not there is underlined
before the game runs, and so is `//`, `<const>`, or a library that LuaJIT
does not have. The server lets `&`, `|`, and `~` through; the engine
refuses them when the file loads.

The components a hook receives are not checked unless the script says what
they are, since the definitions cannot know which components a system
named:

```lua
---@class Door : Component
---@field speed number
---@field open boolean
local Door = Component:extend { speed = 2.0, open = false }

local Doors = System:extend(Door)

---@param door Door
function Doors:update(entity, door, dt)
  door.opne = true -- underlined
end
```

## Which Lua

The scripts run on [LuaJIT](https://luajit.org/) 2.1, and on nothing else.
It runs on x86 and x64, on 32-bit ARM, and on ARM64, which is Apple
silicon, the ARM Macs, iPhones and iPads, Linux and Windows on ARM, and
the Raspberry Pi; it also has ports to PowerPC and MIPS, which the build
does not turn on, and a processor outside those is refused when the build
is configured. Its compiler runs on the desktops, macOS, Linux, and
Windows, on both x64 and ARM64. On a platform that forbids writing code at
run time, iOS and iPadOS and the consoles, LuaJIT turns its compiler off
itself and is an interpreter there; the SDK flags of those platforms are
not wired into the build yet. The Windows build cross-compiles LuaJIT with
llvm-mingw like the rest; the two tools of LuaJIT's build that run on the
building machine are compiled by the same clang, which the Windows image
gives the C headers and runtime files of that machine for.

`scripting.jit: false` in [settings.yml](settings.md), or `--jit off` on
the command line for one run, keeps LuaJIT from compiling and runs the
scripts in its interpreter, which is how the two are compared. The log
says which started.

LuaJIT speaks Lua 5.1 with the parts of 5.2 it chose to add: `goto`,
`table.pack`, `table.unpack`, `%q`, and `__pairs` and `__len` on tables.
What a script written for a later Lua has to know:

| | In a script |
|---|---|
| Numbers | One kind, a double; `3` and `3.0` are the same value and print the same. Beyond 2^53 a number loses precision, in `Long` and `UnsignedLong` fields too |
| `//` | Not in the language; `math.floor(a / b)` |
| `&`, `\|`, `~`, `<<`, `>>` | Not in the language; `bit.band`, `bit.bor`, `bit.bxor`, `bit.bnot`, `bit.lshift`, `bit.rshift`, on 32 bits with a sign |
| `local x <const>`, `local x <close>` | Not in the language; a file with one does not load |
| `utf8`, `string.pack`, `table.move`, `math.type`, `math.tointeger` | Absent |
| `unpack` | Present, with `table.unpack` |
| A whole value written to an `Integer` field | Any number without a fraction, `2.0` as well |

This is why a number in a declaration is a `Float`, and a whole field is
declared with `integer(3)`: LuaJIT cannot tell `3` from `3.0`.

The bindings are written to the API of Lua 5.4; what LuaJIT lacks of it is
in `lua-compat.hpp`, where a userdata's single user value, the cache of
handles, is its environment table.

What the compiler buys: a hook that works on its own values, numbers,
tables, and the vectors it makes, is compiled to machine code. A hook that
reads and writes components crosses into C at every field, and LuaJIT does
not compile across such a call, so those run in its interpreter. Reaching
a component as a C struct through LuaJIT's FFI, which compiles to a load
and a store, is #326.

## How it is built

| Part | Where | Holds |
|---|---|---|
| `ScriptContext`, `ScriptSystem` | neon-core, `neon/scripting/` | What the engine sees of the scripts, in no language: load, start, run the hooks, hand the physics events over. The base of the backends |
| `ScriptField`, `ScriptComponentLayout` | neon-core, `neon/scripting/` | A field a script declares, and the layout that makes the fields a component of the store, with its `TypeInfo` for reflection and its `ComponentFormat` for scenes |
| `ScriptRunning` | neon-core, `neon/world-system/ecs/systems/` | The system of the world that loads the scripts when components are registered and runs their hooks in its place |
| `FileSystemContext::ListFiles` | neon-core | How the scripts are found |
| `Lua_ScriptSystem` | neon-lua, `neon/scripting/` | The Lua backend: the state, the files, the contracts, the hooks |
| `lua-classes`, `lua-sandbox`, `lua-libraries` | neon-lua | `Component` and `System`, what of Lua a script gets, and `world`, `input`, `log`, `scene` |
| `lua-entity-handle`, `lua-component-handle`, `lua-vec3-handle`, `lua-color-handle` | neon-lua | What a script holds of an entity, a component, a vector, a color |
| `lua` | `external/luajit`, built by its own Makefile from `lib/neon-lua/CMakeLists.txt` | LuaJIT, as C, private to neon-lua |
| `lua-compat` | neon-lua | What of the Lua 5.4 API the bindings use and LuaJIT lacks, and how many parameters a function takes |

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
