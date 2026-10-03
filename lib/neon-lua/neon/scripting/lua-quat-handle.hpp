#ifndef LUA_QUAT_HANDLE_HPP
#define LUA_QUAT_HANDLE_HPP

#include <glm/gtc/quaternion.hpp>

#include <neon/reflection/type-info.hpp>

#include "lua-component-handle.hpp"

struct lua_State;

namespace neon
{
  /// A quaternion a script holds, bound to a field of a component when it
  /// was read from one, as a vector is. See LuaVec3Handle.
  struct LuaQuatHandle
  {
    glm::quat value{1.0f, 0.0f, 0.0f, 0.0f};

    LuaComponentHandle target;
    const FieldInfo *field = nullptr;

    [[nodiscard]] bool IsBound() const
    {
      return field != nullptr;
    }
  };

  /// Adds the metatable of quaternions and the `quat` function to the
  /// state, once. `quat()` is no turn, `quat(x, y, z, w)` the numbers,
  /// `quat.from_euler(pitch, yaw, roll)` from degrees.
  void open_quat_handles(lua_State *lua);

  void push_quat(lua_State *lua, const glm::quat &value);

  void push_bound_quat(lua_State *lua, const glm::quat &value, const LuaComponentHandle &target, const FieldInfo &field);

  /// The quaternion at the index, or a Lua error when there is none.
  [[nodiscard]] glm::quat check_quat(lua_State *lua, int index);

  [[nodiscard]] LuaQuatHandle *test_quat(lua_State *lua, int index);
} // neon

#endif //LUA_QUAT_HANDLE_HPP
