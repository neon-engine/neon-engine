#ifndef LUA_CLASSES_HPP
#define LUA_CLASSES_HPP

#include <string>

#include "lua-component-declaration.hpp"
#include "lua-system-declaration.hpp"

struct lua_State;

namespace neon
{
  /// Adds `Component` and `System` to the state, the two classes a script
  /// extends. `Component:extend { fields }` makes a component class, with
  /// an optional name in front of the table. `System:extend("Door", ...)`
  /// makes a system class over the named components, which may be given as
  /// their class tables too.
  void open_classes(lua_State *lua);

  /// Whether the value at the index is a class made by `Component:extend`.
  [[nodiscard]] bool is_component_class(lua_State *lua, int index);

  /// Whether the value at the index is a class made by `System:extend`.
  [[nodiscard]] bool is_system_class(lua_State *lua, int index);

  /// Reads a component class into a declaration. `name` is what the file
  /// says the component is called, used unless the class names itself.
  /// Returns false with what is wrong when the class breaks the contract.
  bool read_component_class(
    lua_State *lua,
    int index,
    const std::string &name,
    LuaComponentDeclaration &declaration,
    std::string &problem);

  /// Reads a system class into a declaration, and keeps a reference to the
  /// class in the registry. Returns false with what is wrong when the class
  /// breaks the contract: a key that is not a hook, a hook that is not a
  /// function.
  bool read_system_class(
    lua_State *lua,
    int index,
    const std::string &name,
    LuaSystemDeclaration &declaration,
    std::string &problem);
} // neon

#endif //LUA_CLASSES_HPP
