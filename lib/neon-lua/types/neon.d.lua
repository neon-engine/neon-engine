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
---@field FirstPersonController Component|nil
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
function Entity:has_component(name) end

---The component, or nil when the entity has none.
---@param name string
---@return Component|nil
function Entity:get_component(name) end

---Takes the component away. It takes effect when the hook's query is done.
---@param name string
function Entity:remove_component(name) end

---@return Entity|nil
function Entity:get_parent() end

---@return Entity[]
function Entity:get_children() end

---@return boolean
function Entity:is_alive() end

---Ends the entity with its children.
function Entity:destroy() end

---A component of an entity. A field is read and changed in place; a name
---with an underscore first is private state the engine keeps for the
---entity and never reads from a recipe.
---@class Component
Component = {}

---Declares a component: every entry of the table is a field with its
---default, and the default says what the field holds: a bool, a whole
---number, a number, text, a vec2, a vec3, a vec4, a color, a quat, a mat3, or a mat4. The name is that of the file,
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
---@field handlers table<string, fun(self: Entity, ...: any)> The functions an element of the user interface calls by name: a button with `on_click`, and a slider, a toggle, a select, an input, or a textarea with `on_change`. `self` is the entity that shows the user interface, and the parameters are what the file wrote as arguments
System = {}

---Declares a system over the components named, as text or as the class in
---hand.
---@param ... string|Component
---@return System
function System:extend(...) end

---Once, when an entity with the components is first seen.
---@param entity Entity
---@param ... any The components the system named, in that order, then what the hook adds
function System:ready(entity, ...) end

---Every frame. The last argument is the seconds of the frame.
---@param entity Entity
---@param ... any The components the system named, in that order, then what the hook adds
function System:update(entity, ...) end

---Every step of the world. The last argument is the seconds of a step.
---@param entity Entity
---@param ... any The components the system named, in that order, then what the hook adds
function System:fixed_update(entity, ...) end

---Once, when an entity the system saw no longer has the components.
---@param entity Entity
function System:removed(entity) end

---A body entered this entity's Trigger. The last argument is the other
---entity.
---@param entity Entity
---@param ... any The components the system named, in that order, then what the hook adds
function System:on_trigger_enter(entity, ...) end

---A body left this entity's Trigger.
---@param entity Entity
---@param ... any The components the system named, in that order, then what the hook adds
function System:on_trigger_exit(entity, ...) end

---This entity's body began to touch another. The last three arguments are
---the other entity, the point, and the normal from this entity to the
---other.
---@param entity Entity
---@param ... any The components the system named, in that order, then what the hook adds
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

---Marks the default of a field as a whole number, for `Component:extend`:
---`count = integer(3)` declares an Integer field, where `count = 3` would
---declare a Float, since LuaJIT has one kind of number.
---@param n integer
---@return integer
function integer(n) end

---A list of a component, reached in place. Indexes count from 1.
---@class list
local list_class = {}

---At the end, or before the element i with two arguments.
---@overload fun(self: list, value: any)
---@param i integer
---@param value any
function list_class:insert(i, value) end

---@param i integer
function list_class:remove(i) end

function list_class:clear() end

---A vector of two numbers, as vec3 is.
---@class vec2
---@field x number
---@field y number
---@operator add(vec2): vec2
---@operator sub(vec2): vec2
---@operator mul(number|vec2): vec2
---@operator div(number): vec2
---@operator unm: vec2
local vec2_class = {}

---@return number
function vec2_class:length() end

---@return vec2
function vec2_class:normalized() end

---@param other vec2
---@return number
function vec2_class:dot(other) end

---@param other vec2
---@return number
function vec2_class:distance(other) end

---@return vec2
function vec2_class:copy() end

---@overload fun(): vec2
---@overload fun(n: number): vec2
---@param x number
---@param y number
---@return vec2
function vec2(x, y) end

---A vector of four numbers, as vec3 is.
---@class vec4
---@field x number
---@field y number
---@field z number
---@field w number
---@operator add(vec4): vec4
---@operator sub(vec4): vec4
---@operator mul(number|vec4): vec4
---@operator div(number): vec4
---@operator unm: vec4
local vec4_class = {}

---@return number
function vec4_class:length() end

---@return vec4
function vec4_class:normalized() end

---@param other vec4
---@return number
function vec4_class:dot(other) end

---@param other vec4
---@return number
function vec4_class:distance(other) end

---@return vec4
function vec4_class:copy() end

---@overload fun(): vec4
---@overload fun(n: number): vec4
---@param x number
---@param y number
---@param z number
---@param w number
---@return vec4
function vec4(x, y, z, w) end

---A quaternion, a turn. One read from a component is bound to its field.
---@class quat
---@field x number
---@field y number
---@field z number
---@field w number
---@operator mul(quat): quat
---@operator mul(vec3): vec3
local quat_class = {}

---@return quat
function quat_class:normalized() end

---@return quat
function quat_class:inverse() end

---Pitch, yaw, and roll in degrees, as a Transform writes them.
---@return number, number, number
function quat_class:to_euler() end

---@return quat
function quat_class:copy() end

---@class quatlib
---@overload fun(): quat
---@overload fun(x: number, y: number, z: number, w: number): quat
quat = {}

---From pitch, yaw, and roll in degrees, applied yaw, then pitch, then
---roll, as a Transform does.
---@param pitch number
---@param yaw number
---@param roll number|nil
---@return quat
function quat.from_euler(pitch, yaw, roll) end

---A matrix of three or four rows. Rows and columns count from 1. One read
---from a component is bound to its field and writes through on set.
---@class matrix
---@operator mul(matrix): matrix
---@operator mul(vec3): vec3
---@operator mul(vec4): vec4
local matrix_class = {}

---@param row integer
---@param column integer
---@return number
function matrix_class:get(row, column) end

---@param row integer
---@param column integer
---@param value number
function matrix_class:set(row, column, value) end

---A row as a vec3 or a vec4.
---@param row integer
---@return vec3|vec4
function matrix_class:row(row) end

---@return matrix
function matrix_class:copy() end

---The identity, or three rows as vec3.
---@overload fun(): matrix
---@param a vec3
---@param b vec3
---@param c vec3
---@return matrix
function mat3(a, b, c) end

---The identity, or four rows as vec4.
---@overload fun(): matrix
---@param a vec4
---@param b vec4
---@param c vec4
---@param d vec4
---@return matrix
function mat4(a, b, c, d) end

---A color, as a vector is.
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
function world.find_entity(path) end

---@param name string|nil
---@param parent Entity|nil
---@return Entity
function world.create_entity(name, parent) end

---@param entity Entity
function world.destroy_entity(entity) end

---Asks for another scene, read at the start of the next frame.
---@param path string
function world.load_scene(path) end

---@class inputlib
input = {}

---@param action string
---@return boolean
function input.is_action_down(action) end

---@param action string
---@return boolean
function input.was_action_pressed(action) end

---@param action string
---@return number
function input.action_axis2(action) end

---@param action string
---@return number, number
function input.action_axis2(action) end

---@param action string
---@return number, number, number
function input.action_axis3(action) end

---The values of the user interface, named as `UiContext` names them. A
---user interface is named by what its file writes as `ui`. A click or a
---change is not asked for here: an element names the function of a system
---that it calls, `on_click: unlock` or `on_click: open('safe', door, $event)`
---on a button, and `on_change: tune($event)` on a slider, a toggle, a
---select, an input, or a textarea.
---@class uilib
ui = {}

---Sets a value that files refer to as `{name}`.
---@param name string
---@param text string
function ui.set_text(name, text) end

---@param name string
---@param number number
function ui.set_number(name, number) end

---@param name string
---@param flag boolean
function ui.set_flag(name, flag) end

---Sets a value for one user interface, where it wins over the value every
---user interface shares.
---@param interface string
---@param name string
---@param text string
function ui.set_text_of(interface, name, text) end

---@param interface string
---@param name string
---@param number number
function ui.set_number_of(interface, name, number) end

---@param interface string
---@param name string
---@param flag boolean
function ui.set_flag_of(interface, name, flag) end

---The settings of the game, named as the project's project.yml declares
---them under `settings`, see docs/projects.md and docs/settings.md. A name
---that is not declared is an error that lists the names.
---@class settingslib
settings = {}

---What a setting holds: a boolean for a flag, a number for a number, a
---string for a text or a choice, and nil for an action.
---@param name string
---@return boolean|number|string|nil
function settings.get(name) end

---Sets a setting, which tells whoever hears of it. Returns false for a
---value that does not fit its kind, its range, or its choices, which the
---log says once.
---@param name string
---@param value boolean|number|string
---@return boolean
function settings.set(name, value) end

---Presses an action, which tells whoever hears of it.
---@param name string
---@return boolean
function settings.trigger(name) end

---Calls the function after the setting changes, with what it holds now,
---or with nil for an action, for as long as the scripts run.
---@param name string
---@param handler fun(value: boolean|number|string|nil)
function settings.on_change(name, handler) end

---@class scenelib
scene = {}

---Asks for another scene, read at the start of the next frame.
---@param path string
function scene.load_scene(path) end

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
