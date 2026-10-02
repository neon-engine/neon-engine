#ifndef LUA_LIBRARIES_HPP
#define LUA_LIBRARIES_HPP

struct lua_State;

namespace neon
{
  /// `world`: the entities. `world.each(name, ...)` iterates the entities
  /// that carry every named component, handing over the entity and the
  /// components; `world.find(path)` finds an entity by its names from the
  /// top; `world.create(name, parent)` makes one; `world.destroy(entity)`
  /// ends one, with its children.
  void open_world_library(lua_State *lua);

  /// `input`: the actions of the input map. `input.is_down(name)`,
  /// `input.pressed(name)`, `input.amount(name)`, `input.axis(name)` as
  /// two numbers, and `input.axis3(name)` as three. Without an input, every
  /// action is up.
  void open_input_library(lua_State *lua);

  /// `log`: the engine's log. `log.debug`, `log.info`, `log.warn`, and
  /// `log.error`, each taking any values, written with a space between.
  /// `print` is `log.info`.
  void open_log_library(lua_State *lua);

  /// `scene`: `scene.load(path)` asks the world for another scene, read at
  /// the start of the next frame. Without a world it says so.
  void open_scene_library(lua_State *lua);

  /// What `math` gets on top: `math.move_toward(from, to, by)` and
  /// `math.clamp(value, low, high)`.
  void open_math_extras(lua_State *lua);
} // neon

#endif //LUA_LIBRARIES_HPP
