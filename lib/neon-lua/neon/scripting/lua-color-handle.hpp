#ifndef LUA_COLOR_HANDLE_HPP
#define LUA_COLOR_HANDLE_HPP

#include <neon/common/color.hpp>
#include <neon/reflection/type-info.hpp>

#include "lua-component-handle.hpp"

struct lua_State;

namespace neon
{
  /// A color a script holds, bound to a field of a component when it was
  /// read from one, as a vector is. See LuaVec3Handle.
  struct LuaColorHandle
  {
    Color value;

    LuaComponentHandle target;
    const FieldInfo *field = nullptr;

    [[nodiscard]] bool IsBound() const
    {
      return field != nullptr;
    }
  };

  /// Adds the metatable of colors and the `color` function to the state,
  /// once.
  void open_color_handles(lua_State *lua);

  void push_color(lua_State *lua, const Color &value);

  void push_bound_color(lua_State *lua, const Color &value, const LuaComponentHandle &target, const FieldInfo &field);

  /// The color at the index, or a Lua error when there is none.
  [[nodiscard]] Color check_color(lua_State *lua, int index);

  [[nodiscard]] LuaColorHandle *test_color(lua_State *lua, int index);
} // neon

#endif //LUA_COLOR_HANDLE_HPP
