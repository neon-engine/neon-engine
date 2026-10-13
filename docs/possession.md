# Possession

This note records who drives in the world: which entity the input reaches
and the view follows, how a game or the editor moves from one such entity
to another, and what an entity hears of it. The words are Unreal's: a
controller possesses a pawn; the pawn is what moves in the world, the
controller is who drives it, and possessing another pawn moves the input
and the view over to it. Asked for in #569.

**Current decision:** a `Possessable` tag marks an entity that can be
driven, a character, a vehicle, a camera. Whoever drives, the player in a
game or the editor in play mode, possesses **one entity at a time**: the
world marks it with `Possessed`, the systems of the engine that read the
input drive it alone, and the window is seen through its camera. Possessing
another entity, from a script, an extension, or the editor, switches the
input and the view in one step, and both entities are told. This is a
feature of the world the runtime has; the editor adds only the way to pick
which entity to possess and when (#569, play mode).

## The tag

```yaml
- name: cart
  components:
    Transform:
      position: [0, 0.5, -6]
    Possessable: Default
    Cart: Default
    RigidBody:
      kind: kinematic
    Collider:
      shape: box
      size: [1.6, 1, 2.4]
  children:
    - name: camera
      components:
        Transform:
          position: [0, 4, 3]
          rotation: [-55, 0, 0]
        Camera: Default
```

| Component | Holds | Who writes it |
|---|---|---|
| `Possessable` | Nothing; being there is what it says: the entity can be possessed | The recipe |
| `Possessed` | Nothing; being there says the entity is the one that is possessed. On exactly one entity at a time, or none | The world, when something possesses the entity. It is read by the scene file and written by no file: a scene saved while playing does not carry it |

A system that drives an entity by the input queries for `Possessed` next to
the component it drives, as `FirstPersonControllerSystem` and
`SpectatorMovement` do, so that an entity that is not possessed is left
alone. A script does the same: `if not entity:has_component("Possessed")
then return end` at the top of its `update`, as the `Cart` of the sandbox
does. The input itself is not taken from anyone: `input.is_action_down`
answers the same to every script, and a script that drives something it
does not possess is wrong, not refused.

## What possession does

| | |
|---|---|
| The input | `FirstPersonControllerSystem` drives the possessed player alone. A player that is not possessed is driven as if nothing were pressed: it stops on the ground, keeps its velocity in the air as a jump does, and its camera stays at the eyes without turning. `SpectatorMovement` moves the possessed spectator alone |
| The view | The window is seen through the camera of the possessed entity: the one on the entity itself or on an entity below it that draws into the window, so a car carries its camera as a child. When the possessed entity has no such camera, or nothing is possessed, every camera of the window is handed to the renderer as before, and the last one drawn wins. Cameras that draw into a texture are drawn whatever is possessed |
| The hooks | The entity that is possessed and the one that is released are told, see [below](#the-hooks) |
| The log | `Possessed 'cart'` and `Released 'player'`, from the world |

## Who is possessed first

When a scene is placed, the world decides what is possessed, in this order:

1. The entity the scene names at its top with `possess: <path>`, by its path
   from the top, whatever it carries. A path that names no entity is an
   error in the log, and the next rule decides.
2. What was possessed and stayed through the change of scene: a `Persistent`
   player keeps driving in the next level.
3. The first entity that carries a `Possessable`, in the order of the scene,
   children after their parent.
4. The first entity with a `FirstPersonController`, else the first with a
   `Spectator`. This is what a scene written before the tag drove, so every
   such scene plays as it did: its player is what is possessed when nothing
   says otherwise. Mark it `Possessable: Default` to say so.
5. Nothing, said in the log once. A scene of a camera and a box is seen as
   before.

```yaml
scene: garage
version: 1
possess: cart

entities:
  ...
```

`SceneFile::Save` writes `possess` back when the file had it.

## Possessing from code

| From | Call | Does |
|---|---|---|
| C++ | `WorldSystem::Possess(entity)`, `Unpossess()`, `GetPossessed()` | Possesses the entity and returns whether it is possessed; releases it so that nothing is possessed; the entity that is possessed, or `No_Entity` |
| A script | `world.possess(entity)`, `world.unpossess()`, `world.get_possessed()` | The same, see [scripting.md](scripting.md#what-a-script-reaches-of-the-engine). `get_possessed` is `nil` when nothing is |
| An extension | `possess`, `unpossess`, `get_possessed` of `NeonExtensionHost`, since version 21; `World::Possess`, `Unpossess`, `GetPossessed` in C++ | The same, see [extensions.md](extensions.md#the-services-of-the-engine) |

An entity that is gone, or that carries no `Possessable`, is refused with
the reason in the log, and nothing changes: `'rock' cannot be possessed: it
carries no Possessable, which marks what can be`. Possessing the entity
that is possessed changes nothing and is no error. An entity that is
destroyed while it is possessed leaves nothing possessed, and nobody is
told, since there is nobody to tell.

## The hooks

An entity is told when it is possessed and when it is released, so that a
car starts its engine sound and a character lowers its weapon:

| Where | Possessed | Released |
|---|---|---|
| A script | `on_possessed(entity, components...)` of every system that runs over the entity | `on_unpossessed(entity, components...)` |
| An extension in C | `listen` of `listen_to_possession` with `possessed` 1 | with `possessed` 0 |
| An extension in C++ | `System::OnPossessed(world, entity)` | `System::OnUnpossessed(world, entity)` |

Both are called before the `update` of the frame after the possession was
asked for, as the hooks of the physics are, so that a hook sees the frame
it happened in: a script that possesses the cart in `on_trigger_enter`
has the cart's `on_possessed` called in the same frame, before any
`update`. What a hook of possession asks for in turn waits for the next
frame. The entity that is released is told first, then the one that is
possessed.

```lua
local Cart = Component:extend { speed = 6, driver = "" }
local CartSystem = System:extend(Cart, "Transform")

function CartSystem:on_possessed(entity, cart, transform)
  log.info("The cart is possessed: move drives it, jump gets off")
end

function CartSystem:on_unpossessed(entity, cart, transform)
  entity.RigidBody.linear_velocity = vec3(0, 0, 0)
end

function CartSystem:update(entity, cart, transform, dt)
  if not entity:has_component("Possessed") then return end
  local x, y = input.action_axis2("move")
  ...
end
```

## The sandbox

[possession.scene.yml](../projects/sandbox/assets/scenes/possession.scene.yml)
of the sandbox is a character and a cart, both `Possessable`, on a floor
in front of a blue wall. The character is possessed first. Walking onto the
plate in front of the cart has the `CartEntry` of
[cart-entry.lua](../projects/sandbox/assets/scripts/cart-entry.lua) possess
the cart: the view jumps to the camera above the cart, and `move` drives it
through [cart.lua](../projects/sandbox/assets/scripts/cart.lua), which
writes the velocity of its kinematic body. `jump` hands the character back,
where it stands, in the frame after the press, so that the press does not
reach the character as a jump.

```sh
bin/debug/darwin-arm64/NeonEditor/NeonEditor bin/debug/darwin-arm64/sandbox \
  --scene assets://scenes/possession.scene.yml
```

Without a window, as the test `runtime-possession` runs it:

```sh
bin/debug/darwin-arm64/NeonEditor/NeonEditor bin/debug/darwin-arm64/sandbox \
  --headless-renderer --scene assets://scenes/possession.scene.yml \
  --frames 45 --time-step 0.05 --input "1: hold-key w 20\n30: hold jump 1" \
  --log-entity cart,player \
  --output-dir shots --screenshot output://frame.png --screenshot-at 2,25,40
```

The museum's player is possessed as the player of a scene written before
the tag; a piece of the museum that drives something is left to its
redesign (#466).

## How it is built

| Piece | Location | Role |
|---|---|---|
| `Possessable`, `Possessed` | neon-core, `neon/world-system/ecs/components/` | The tags. `Possessable` has a format, so a recipe reads it; `Possessed` has none |
| `Possession` | neon-core, `neon/world-system/ecs/possession.hpp` | Who drives: owned by `EntityWorld`, it possesses, releases, marks, tells the listeners, and decides who is possessed first with `Start()` |
| `PossessionEvent`, `PossessionListener` | neon-core, `neon/world-system/` | What is told, and who hears it: `ScriptRunning` and `ExtensionHost`, which keep it and hand it to their hooks in their next update |
| `WorldSystem::Possess`, `Unpossess`, `GetPossessed`, `AddPossessionListener` | neon-core | The operations, which the scripts and the extensions reach through the world |
| `Scene::GetPossessed`, `SceneFile` | neon-core | `possess` of the file |
| `ScriptContext::DispatchPossessionEvents`, `Lua_ScriptSystem` | neon-core, neon-lua | `on_possessed` and `on_unpossessed` |
| `listen_to_possession`, `possess`, `unpossess`, `get_possessed` | neon-extension-api, version 21 | The boundary |

| Decision | Reason |
|---|---|
| A tag on the possessed entity, read by the systems, rather than an input context per entity | The smallest change: a system that drives by the input adds one component to its query. The actions stay one set, read by whoever asks |
| `Possess` requires the tag, the entry possession does not | What can be driven is said in the recipe, as Unreal says it with the class of a pawn; and a scene written before the tag keeps its player |
| The hooks are called in the next update of the scripts, not inside `Possess` | A hook of Lua runs with handles that are filled for the running hook; calling another hook from inside it would refill them. The physics events go the same way |
| The view follows the possessed entity's own camera, or one below it | A camera that follows a car from outside is a child of the car, or a script that moves it; the world does not guess |

## Open questions

- A camera that follows the possessed entity without being below it, such as
  one a script drives: today it is drawn as every window camera is, and the
  last one wins. A `follows` on the `Camera` could name whom it belongs to.
- Several players at once, each possessing one entity, need a controller per
  player and an input map per device (#116 and the input's open questions).
- Possession from the editor, and the editor's camera as what is possessed
  when nothing is, come with play mode (#569).
