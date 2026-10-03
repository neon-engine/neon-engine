#ifndef LUA_MATRIX_HANDLE_HPP
#define LUA_MATRIX_HANDLE_HPP

#include <glm/glm.hpp>

#include <neon/reflection/type-info.hpp>

#include "lua-component-handle.hpp"

struct lua_State;

namespace neon
{
  /// A matrix of three or four rows a script holds, kept as a 4 by 4 with
  /// the rest the identity. One read from a component is bound to its
  /// field and writes through when a cell is set; one made with `mat3()`
  /// or `mat4()` is a value of its own. Rows and columns count from 1, as
  /// Lua does.
  struct LuaMatrixHandle
  {
    glm::mat4 value{1.0f};

    /// 3 or 4.
    int size = 4;

    LuaComponentHandle target;
    const FieldInfo *field = nullptr;

    [[nodiscard]] bool IsBound() const
    {
      return field != nullptr;
    }
  };

  /// Adds the metatable of matrices and the `mat3` and `mat4` functions to
  /// the state, once. Each makes the identity, or takes its rows as
  /// vectors.
  void open_matrix_handles(lua_State *lua);

  void push_mat3(lua_State *lua, const glm::mat3 &value);

  void push_mat4(lua_State *lua, const glm::mat4 &value);

  void push_bound_mat3(lua_State *lua, const glm::mat3 &value, const LuaComponentHandle &target, const FieldInfo &field);

  void push_bound_mat4(lua_State *lua, const glm::mat4 &value, const LuaComponentHandle &target, const FieldInfo &field);

  /// The handle at the index, or a Lua error when there is none.
  [[nodiscard]] LuaMatrixHandle &check_matrix(lua_State *lua, int index);

  [[nodiscard]] LuaMatrixHandle *test_matrix(lua_State *lua, int index);

  /// The value of a handle, read from its component when bound.
  [[nodiscard]] glm::mat4 matrix_value(lua_State *lua, LuaMatrixHandle &handle);
} // neon

#endif //LUA_MATRIX_HANDLE_HPP
