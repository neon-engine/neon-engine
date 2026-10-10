#ifndef LUA_LIBRARIES_HPP
#define LUA_LIBRARIES_HPP

struct lua_State;

namespace neon
{
  /// `world`: the entities, named as `EntityStore` names them.
  /// `world.each(name, ...)` iterates the entities that carry every named
  /// component, handing over the entity and the components;
  /// `world.find_entity(path)` finds one by its names from the top;
  /// `world.create_entity(name, parent)` makes one;
  /// `world.destroy_entity(entity)` ends one, with its children;
  /// `world.load_scene(path)` asks for another scene.
  void open_world_library(lua_State *lua);

  /// `input`: the actions of the input map, named as `InputContext` names
  /// them. `input.is_action_down(name)`, `input.was_action_pressed(name)`,
  /// `input.action_axis2(name)`, `input.action_axis2(name)` as two numbers,
  /// and `input.action_axis3(name)` as three. Without an input, every
  /// action is up.
  void open_input_library(lua_State *lua);

  /// `ui`: the values of the user interface, named as `UiContext` names
  /// them. `ui.set_text(name, text)`, `ui.set_number(name, number)`, and
  /// `ui.set_flag(name, flag)` set a value that files refer to as `{name}`,
  /// and `ui.set_text_of(interface, name, text)`, `ui.set_number_of`, and
  /// `ui.set_flag_of` set it for one user interface. Without a user
  /// interface a value that is set is dropped. A click or a change is not
  /// asked for here: an element names the function it calls with
  /// `on_click` or `on_change`.
  void open_ui_library(lua_State *lua);

  /// `log`: the engine's log. `log.debug`, `log.info`, `log.warn`, and
  /// `log.error`, each taking any values, written with a space between.
  /// `print` is `log.info`.
  void open_log_library(lua_State *lua);

  /// `scene`: `scene.load_scene(path)`, the same as `world.load_scene`.
  void open_scene_library(lua_State *lua);

  /// What `math` gets on top: `math.move_toward(from, to, by)` and
  /// `math.clamp(value, low, high)`.
  void open_math_extras(lua_State *lua);
} // neon

#endif //LUA_LIBRARIES_HPP
