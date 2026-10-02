---@meta
-- Definitions of what a script of Neon Engine reaches, for the Lua language
-- server (https://luals.github.io): completion, and a mark under a field or
-- a hook that is not there, before the game runs. Not loaded by the engine.
-- A project's .luarc.json names the folder of this file in
-- "workspace.library". See docs/scripting.md.

---@class Entity
---@field id integer
---@field name string
---@field path string The names from the top, with slashes between
---@field Transform Transform|nil
---@field Camera Component|nil
---@field Light Component|nil
---@field Renderable Component|nil
---@field RigidBody Component|nil
---@field Trigger Component|nil
---@field CharacterBody Component|nil
---@field Collider Component|nil
---@field Joint Component|nil
---@field Player Component|nil
---@field Spectator Component|nil
---@field Persistent Component|nil
---@field SceneExit Component|nil
---@field SoundSource Component|nil
---@field SoundListener Component|nil
---@field Geometry Component|nil
---@field Prefab Component|nil
local Entity = {}

---Whether the entity carries the component.
---@param name string
---@return boolean
function Entity:has(name) end

---The component, or nil when the entity has none.
---@param name string
---@return Component|nil
function Entity:get(name) end

---Takes the component away. It takes effect when the hook's query is done.
---@param name string
function Entity:remove(name) end

---@return Entity|nil
function Entity:parent() end

---@return Entity[]
function Entity:children() end

---@return boolean
function Entity:alive() end

---Ends the entity with its children.
function Entity:destroy() end

---A component of an entity. A field is read and changed in place; a name
---with an underscore first is private state the engine keeps for the
---entity and never reads from a recipe.
---@class Component
local Component = {}

---Declares a component: every entry of the table is a field with its
---default, and the default says what the field holds: a bool, a whole
---number, a number, text, a vec3, or a color. The name is that of the file,
---unless given in front of the table.
---@overload fun(self: Component, fields: table): Component
---@param name string
---@param fields table
---@return Component
function Component:extend(name, fields) end

---@class Transform : Component
---@field position vec3
---@field rotation vec3 Degrees about x, y, and z
---@field scale vec3

---A system: its hooks, over every entity that carries the components it
---named. `self` is the class.
---@class System
local System = {}

---Declares a system over the components named, as text or as the class in
---hand.
---@param ... string|Component
---@return System
function System:extend(...) end

---Once, when an entity with the components is first seen.
---@param entity Entity
---@param ... Component
function System:ready(entity, ...) end

---Every frame. The last argument is the seconds of the frame.
---@param entity Entity
---@param ... Component|number
function System:update(entity, ...) end

---Every step of the world. The last argument is the seconds of a step.
---@param entity Entity
---@param ... Component|number
function System:step(entity, ...) end

---Once, when an entity the system saw no longer has the components.
---@param entity Entity
function System:removed(entity) end

---A body entered this entity's Trigger. The last argument is the other
---entity.
---@param entity Entity
---@param ... Component|Entity
function System:on_trigger_enter(entity, ...) end

---A body left this entity's Trigger.
---@param entity Entity
---@param ... Component|Entity
function System:on_trigger_exit(entity, ...) end

---This entity's body began to touch another. The last three arguments are
---the other entity, the point, and the normal from this entity to the
---other.
---@param entity Entity
---@param ... Component|Entity|vec3
function System:on_collision(entity, ...) end

---A vector. One read from a component is bound to its field and writes
---through; one made with vec3() is a value of its own.
---@class vec3
---@field x number
---@field y number
---@field z number
---@operator add(vec3): vec3
---@operator sub(vec3): vec3
---@operator mul(number|vec3): vec3
---@operator div(number): vec3
---@operator unm: vec3
local vec3_class = {}

---@return number
function vec3_class:length() end

---@return vec3
function vec3_class:normalized() end

---@param other vec3
---@return number
function vec3_class:dot(other) end

---@param other vec3
---@return vec3
function vec3_class:cross(other) end

---@param other vec3
---@return number
function vec3_class:distance(other) end

---A copy of its own, no longer bound to a component.
---@return vec3
function vec3_class:copy() end

---@overload fun(): vec3
---@overload fun(n: number): vec3
---@param x number
---@param y number
---@param z number
---@return vec3
function vec3(x, y, z) end

---A colour, as a vector is.
---@class color
---@field r number
---@field g number
---@field b number
---@field a number
local color_class = {}

---@return color
function color_class:copy() end

---@param r number
---@param g number
---@param b number
---@param a number|nil
---@return color
function color(r, g, b, a) end

---@class worldlib
world = {}

---The entities that carry every named component, with their components.
---@param ... string
---@return fun(): Entity, ...
function world.each(...) end

---An entity by its path from the top, or nil.
---@param path string
---@return Entity|nil
function world.find(path) end

---@param name string|nil
---@param parent Entity|nil
---@return Entity
function world.create(name, parent) end

---@param entity Entity
function world.destroy(entity) end

---@class inputlib
input = {}

---@param action string
---@return boolean
function input.is_down(action) end

---@param action string
---@return boolean
function input.pressed(action) end

---@param action string
---@return number
function input.amount(action) end

---@param action string
---@return number, number
function input.axis(action) end

---@param action string
---@return number, number, number
function input.axis3(action) end

---@class scenelib
scene = {}

---Asks for another scene, read at the start of the next frame.
---@param path string
function scene.load(path) end

---@class loglib
log = {}

---@param ... any
function log.debug(...) end

---@param ... any
function log.info(...) end

---@param ... any
function log.warn(...) end

---@param ... any
function log.error(...) end

---`from` moved by at most `by` towards `to`.
---@param from number
---@param to number
---@param by number
---@return number
function math.move_toward(from, to, by) end

---@param value number
---@param low number
---@param high number
---@return number
function math.clamp(value, low, high) end
