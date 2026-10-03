#ifndef LUA_LIST_HANDLE_HPP
#define LUA_LIST_HANDLE_HPP

#include <neon/reflection/type-info.hpp>

#include "lua-component-handle.hpp"

struct lua_State;

namespace neon
{
  /// A list of a component a script holds in place: `#list`, `list[i]`,
  /// `list[i] = v`, `list:insert(v)`, `list:insert(i, v)`, `list:remove(i)`,
  /// and `list:clear()`, each reaching the vector where it lives through the
  /// field's `reach`. Nothing is copied but the one element read or written.
  /// The vector keeps its own size, so an index past the end is an error
  /// and growth goes through `insert`.
  struct LuaListHandle
  {
    LuaComponentHandle target;
    const FieldInfo *field = nullptr;
  };

  /// Adds the metatable of lists to the state, once.
  void open_list_handles(lua_State *lua);

  /// Pushes a handle of a list field of a component. The field has to be a
  /// list kind with a `reach`.
  void push_list(lua_State *lua, const LuaComponentHandle &target, const FieldInfo &field);

  [[nodiscard]] LuaListHandle *test_list(lua_State *lua, int index);
} // neon

#endif //LUA_LIST_HANDLE_HPP
