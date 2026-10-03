#include "lua-quat-handle.hpp"

#include <cstring>

#include <glm/gtc/quaternion.hpp>

#include "lua-api.hpp"
#include "lua-host.hpp"
#include "lua-vec3-handle.hpp"

namespace neon
{
  // Helpers of the quaternion handles, for this file alone.
  namespace
  {
    constexpr const char *metatable = "neon.quat";

    LuaQuatHandle *new_handle(lua_State *lua)
    {
      auto *handle = static_cast<LuaQuatHandle *>(lua_newuserdatauv(lua, sizeof(LuaQuatHandle), 0));
      new(handle) LuaQuatHandle();
      luaL_setmetatable(lua, metatable);
      return handle;
    }

    glm::quat value_of(lua_State *lua, LuaQuatHandle &handle)
    {
      if (handle.IsBound())
      {
        if (EntityStore *store = host_of(lua).store)
        {
          if (void *object = handle.target.Resolve(*store))
          {
            const FieldValue read = handle.field->get(object);
            if (const auto *held = std::get_if<glm::quat>(&read)) { handle.value = *held; }
          }
        }
      }
      return handle.value;
    }

    void write_back(lua_State *lua, const LuaQuatHandle &handle)
    {
      if (!handle.IsBound()) { return; }

      EntityStore *store = host_of(lua).store;
      void *object = store != nullptr ? handle.target.Resolve(*store) : nullptr;
      if (object == nullptr)
      {
        luaL_error(lua, "The entity no longer has the component this quaternion belongs to");
        return;
      }
      handle.field->set(object, handle.value);
    }

    float &part(glm::quat &value, lua_State *lua, const char *key)
    {
      if (std::strcmp(key, "x") == 0) { return value.x; }
      if (std::strcmp(key, "y") == 0) { return value.y; }
      if (std::strcmp(key, "z") == 0) { return value.z; }
      if (std::strcmp(key, "w") == 0) { return value.w; }
      luaL_error(lua, "A quat has x, y, z, and w, not '%s'", key);
      return value.x;
    }

    int quat_index(lua_State *lua)
    {
      auto &handle = *static_cast<LuaQuatHandle *>(luaL_checkudata(lua, 1, metatable));

      if (lua_type(lua, 2) == LUA_TSTRING)
      {
        luaL_getmetatable(lua, metatable);
        lua_pushvalue(lua, 2);
        lua_rawget(lua, -2);
        if (!lua_isnil(lua, -1)) { return 1; }
        lua_pop(lua, 2);
      }

      glm::quat value = value_of(lua, handle);
      lua_pushnumber(lua, part(value, lua, luaL_checkstring(lua, 2)));
      return 1;
    }

    int quat_newindex(lua_State *lua)
    {
      auto &handle = *static_cast<LuaQuatHandle *>(luaL_checkudata(lua, 1, metatable));
      const char *key = luaL_checkstring(lua, 2);
      const auto number = static_cast<float>(luaL_checknumber(lua, 3));

      value_of(lua, handle);
      part(handle.value, lua, key) = number;
      write_back(lua, handle);
      return 0;
    }

    /// `a * b` turns by b then a, as quaternions compose; `q * v` turns a
    /// vector.
    int quat_mul(lua_State *lua)
    {
      if (test_vec3(lua, 2) != nullptr)
      {
        push_vec3(lua, check_quat(lua, 1) * check_vec3(lua, 2));
        return 1;
      }
      push_quat(lua, check_quat(lua, 1) * check_quat(lua, 2));
      return 1;
    }

    int quat_eq(lua_State *lua)
    {
      lua_pushboolean(lua, check_quat(lua, 1) == check_quat(lua, 2) ? 1 : 0);
      return 1;
    }

    int quat_tostring(lua_State *lua)
    {
      const glm::quat value = check_quat(lua, 1);
      lua_pushfstring(
        lua,
        "quat(%f, %f, %f, %f)",
        static_cast<double>(value.x),
        static_cast<double>(value.y),
        static_cast<double>(value.z),
        static_cast<double>(value.w));
      return 1;
    }

    int quat_normalized(lua_State *lua)
    {
      push_quat(lua, glm::normalize(check_quat(lua, 1)));
      return 1;
    }

    int quat_inverse(lua_State *lua)
    {
      push_quat(lua, glm::inverse(check_quat(lua, 1)));
      return 1;
    }

    /// Pitch, yaw, and roll in degrees, as a Transform writes them.
    int quat_to_euler(lua_State *lua)
    {
      const glm::vec3 angles = glm::degrees(glm::eulerAngles(check_quat(lua, 1)));
      lua_pushnumber(lua, angles.x);
      lua_pushnumber(lua, angles.y);
      lua_pushnumber(lua, angles.z);
      return 3;
    }

    int quat_copy(lua_State *lua)
    {
      push_quat(lua, check_quat(lua, 1));
      return 1;
    }

    /// `quat()` is no turn, `quat(x, y, z, w)` the numbers.
    int quat_new(lua_State *lua)
    {
      const int count = lua_gettop(lua);
      glm::quat value{1.0f, 0.0f, 0.0f, 0.0f};
      if (count >= 4)
      {
        value = glm::quat{
          static_cast<float>(luaL_checknumber(lua, 4)),
          static_cast<float>(luaL_checknumber(lua, 1)),
          static_cast<float>(luaL_checknumber(lua, 2)),
          static_cast<float>(luaL_checknumber(lua, 3))};
      }
      else if (count != 0) { return luaL_error(lua, "quat takes x, y, z, and w, or nothing for no turn"); }
      push_quat(lua, value);
      return 1;
    }

    /// `quat.from_euler(pitch, yaw, roll)` in degrees, applied yaw, then
    /// pitch, then roll, as a Transform does.
    int quat_from_euler(lua_State *lua)
    {
      const auto pitch = glm::radians(static_cast<float>(luaL_checknumber(lua, 1)));
      const auto yaw = glm::radians(static_cast<float>(luaL_checknumber(lua, 2)));
      const auto roll = glm::radians(static_cast<float>(luaL_optnumber(lua, 3, 0.0)));
      const glm::quat value = glm::angleAxis(yaw, glm::vec3{0.0f, 1.0f, 0.0f})
                              * glm::angleAxis(pitch, glm::vec3{1.0f, 0.0f, 0.0f})
                              * glm::angleAxis(roll, glm::vec3{0.0f, 0.0f, 1.0f});
      push_quat(lua, value);
      return 1;
    }

    constexpr luaL_Reg methods[] = {
      {"__index", quat_index},
      {"__newindex", quat_newindex},
      {"__mul", quat_mul},
      {"__eq", quat_eq},
      {"__tostring", quat_tostring},
      {"normalized", quat_normalized},
      {"inverse", quat_inverse},
      {"to_euler", quat_to_euler},
      {"copy", quat_copy},
      {nullptr, nullptr}
    };
  }

  void open_quat_handles(lua_State *lua)
  {
    if (luaL_newmetatable(lua, metatable) != 0) { luaL_setfuncs(lua, methods, 0); }
    lua_pop(lua, 1);

    // `quat(...)` makes one, and `quat.from_euler(...)` too: a table that is
    // called
    lua_newtable(lua);
    lua_pushcfunction(lua, quat_from_euler);
    lua_setfield(lua, -2, "from_euler");
    lua_newtable(lua);
    lua_pushcfunction(lua, [](lua_State *state)
    {
      lua_remove(state, 1);
      return quat_new(state);
    });
    lua_setfield(lua, -2, "__call");
    lua_setmetatable(lua, -2);
    lua_setglobal(lua, "quat");
  }

  void push_quat(lua_State *lua, const glm::quat &value)
  {
    new_handle(lua)->value = value;
  }

  void push_bound_quat(lua_State *lua, const glm::quat &value, const LuaComponentHandle &target, const FieldInfo &field)
  {
    LuaQuatHandle *handle = new_handle(lua);
    handle->value = value;
    handle->target = target;
    handle->field = &field;
  }

  glm::quat check_quat(lua_State *lua, const int index)
  {
    auto &handle = *static_cast<LuaQuatHandle *>(luaL_checkudata(lua, index, metatable));
    return value_of(lua, handle);
  }

  LuaQuatHandle *test_quat(lua_State *lua, const int index)
  {
    return static_cast<LuaQuatHandle *>(luaL_testudata(lua, index, metatable));
  }
} // neon
