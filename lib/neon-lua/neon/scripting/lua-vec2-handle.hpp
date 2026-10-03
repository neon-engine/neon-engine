#ifndef LUA_VEC2_HANDLE_HPP
#define LUA_VEC2_HANDLE_HPP

#include <glm/glm.hpp>

#include <neon/reflection/type-info.hpp>

#include "lua-component-handle.hpp"

struct lua_State;

namespace neon
{
  /// A vector a script holds. One read from a component is bound to its
  /// field: writing `position.y = 1` changes the component. One made with
  /// `vec2(x, y)`, or that came out of a sum, is a value of its own
  /// until it is assigned to a field.
  struct LuaVec2Handle
  {
    glm::vec2 value{0.0f};

    /// The component and the field the vector is bound to, if any.
    LuaComponentHandle target;
    const FieldInfo *field = nullptr;

    [[nodiscard]] bool IsBound() const
    {
      return field != nullptr;
    }
  };

  /// Adds the metatable of vectors and the `vec2` function to the state,
  /// once.
  void open_vec2_handles(lua_State *lua);

  /// Pushes a vector of its own.
  void push_vec2(lua_State *lua, const glm::vec2 &value);

  /// Pushes a vector bound to a field of a component.
  void push_bound_vec2(lua_State *lua, const glm::vec2 &value, const LuaComponentHandle &target, const FieldInfo &field);

  /// The vector at the index, or a Lua error when there is none.
  [[nodiscard]] glm::vec2 check_vec2(lua_State *lua, int index);

  /// The handle at the index, or nullptr when the value is none.
  [[nodiscard]] LuaVec2Handle *test_vec2(lua_State *lua, int index);
} // neon

#endif //LUA_VEC2_HANDLE_HPP
