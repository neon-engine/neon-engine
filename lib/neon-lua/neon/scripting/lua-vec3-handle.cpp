#include "lua-vec3-handle.hpp"

#include <cstring>
#include <string>

#include <glm/geometric.hpp>

#include "lua-api.hpp"
#include "lua-host.hpp"

namespace neon
{
  // Helpers of the vector handles, for this file alone.
  namespace
  {
    constexpr const char *metatable = "neon.vec3";

    LuaVec3Handle *new_handle(lua_State *lua)
    {
      auto *handle = static_cast<LuaVec3Handle *>(lua_newuserdatauv(lua, sizeof(LuaVec3Handle), 0));
      new(handle) LuaVec3Handle();
      luaL_setmetatable(lua, metatable);
      return handle;
    }

    /// The value of a handle: from the component while it is bound and the
    /// component is there, which is where a script may have changed it
    /// through another handle, and its own otherwise.
    glm::vec3 value_of(lua_State *lua, LuaVec3Handle &handle)
    {
      if (handle.IsBound())
      {
        if (EntityStore *store = host_of(lua).store)
        {
          if (void *object = handle.target.Resolve(*store))
          {
            const FieldValue read = handle.field->get(object);
            if (const auto *held = std::get_if<glm::vec3>(&read)) { handle.value = *held; }
          }
        }
      }
      return handle.value;
    }

    /// Writes the value of a handle through to its component, if bound.
    void write_back(lua_State *lua, const LuaVec3Handle &handle)
    {
      if (!handle.IsBound()) { return; }

      EntityStore *store = host_of(lua).store;
      void *object = store != nullptr ? handle.target.Resolve(*store) : nullptr;
      if (object == nullptr)
      {
        luaL_error(lua, "The entity no longer has the component this vector belongs to");
        return;
      }
      handle.field->set(object, handle.value);
    }

    int axis_index(lua_State *lua, const char *key)
    {
      if (std::strcmp(key, "x") == 0) { return 0; }
      if (std::strcmp(key, "y") == 0) { return 1; }
      if (std::strcmp(key, "z") == 0) { return 2; }
      return luaL_error(lua, "A vector has x, y, and z, not '%s'", key);
    }

    int vec3_index(lua_State *lua)
    {
      auto &handle = *static_cast<LuaVec3Handle *>(luaL_checkudata(lua, 1, metatable));

      // a method first
      if (lua_type(lua, 2) == LUA_TSTRING)
      {
        luaL_getmetatable(lua, metatable);
        lua_pushvalue(lua, 2);
        lua_rawget(lua, -2);
        if (!lua_isnil(lua, -1)) { return 1; }
        lua_pop(lua, 2);
      }

      const int axis = axis_index(lua, luaL_checkstring(lua, 2));
      lua_pushnumber(lua, value_of(lua, handle)[axis]);
      return 1;
    }

    int vec3_newindex(lua_State *lua)
    {
      auto &handle = *static_cast<LuaVec3Handle *>(luaL_checkudata(lua, 1, metatable));
      const int axis = axis_index(lua, luaL_checkstring(lua, 2));
      const auto number = static_cast<float>(luaL_checknumber(lua, 3));

      value_of(lua, handle);
      handle.value[axis] = number;
      write_back(lua, handle);
      return 0;
    }

    int vec3_add(lua_State *lua)
    {
      push_vec3(lua, check_vec3(lua, 1) + check_vec3(lua, 2));
      return 1;
    }

    int vec3_sub(lua_State *lua)
    {
      push_vec3(lua, check_vec3(lua, 1) - check_vec3(lua, 2));
      return 1;
    }

    int vec3_mul(lua_State *lua)
    {
      if (lua_isnumber(lua, 1)) { push_vec3(lua, static_cast<float>(lua_tonumber(lua, 1)) * check_vec3(lua, 2)); }
      else if (lua_isnumber(lua, 2)) { push_vec3(lua, check_vec3(lua, 1) * static_cast<float>(lua_tonumber(lua, 2))); }
      else { push_vec3(lua, check_vec3(lua, 1) * check_vec3(lua, 2)); }
      return 1;
    }

    int vec3_div(lua_State *lua)
    {
      push_vec3(lua, check_vec3(lua, 1) / static_cast<float>(luaL_checknumber(lua, 2)));
      return 1;
    }

    int vec3_unm(lua_State *lua)
    {
      push_vec3(lua, -check_vec3(lua, 1));
      return 1;
    }

    int vec3_eq(lua_State *lua)
    {
      lua_pushboolean(lua, check_vec3(lua, 1) == check_vec3(lua, 2) ? 1 : 0);
      return 1;
    }

    int vec3_tostring(lua_State *lua)
    {
      const glm::vec3 value = check_vec3(lua, 1);
      lua_pushfstring(lua, "vec3(%f, %f, %f)", static_cast<double>(value.x), static_cast<double>(value.y), static_cast<double>(value.z));
      return 1;
    }

    int vec3_length(lua_State *lua)
    {
      lua_pushnumber(lua, glm::length(check_vec3(lua, 1)));
      return 1;
    }

    int vec3_normalized(lua_State *lua)
    {
      const glm::vec3 value = check_vec3(lua, 1);
      push_vec3(lua, glm::length(value) > 0.0f ? glm::normalize(value) : value);
      return 1;
    }

    int vec3_dot(lua_State *lua)
    {
      lua_pushnumber(lua, glm::dot(check_vec3(lua, 1), check_vec3(lua, 2)));
      return 1;
    }

    int vec3_cross(lua_State *lua)
    {
      push_vec3(lua, glm::cross(check_vec3(lua, 1), check_vec3(lua, 2)));
      return 1;
    }

    int vec3_distance(lua_State *lua)
    {
      lua_pushnumber(lua, glm::distance(check_vec3(lua, 1), check_vec3(lua, 2)));
      return 1;
    }

    /// A copy of its own, no longer bound to a component.
    int vec3_copy(lua_State *lua)
    {
      push_vec3(lua, check_vec3(lua, 1));
      return 1;
    }

    /// `vec3(x, y, z)`, `vec3(n)` for all three, or `vec3()` for zero.
    int vec3_new(lua_State *lua)
    {
      const int count = lua_gettop(lua);
      glm::vec3 value{0.0f};
      if (count == 1) { value = glm::vec3(static_cast<float>(luaL_checknumber(lua, 1))); }
      else if (count >= 3)
      {
        value = glm::vec3(
          static_cast<float>(luaL_checknumber(lua, 1)),
          static_cast<float>(luaL_checknumber(lua, 2)),
          static_cast<float>(luaL_checknumber(lua, 3)));
      }
      else if (count != 0) { return luaL_error(lua, "vec3 takes three numbers, one for all, or none"); }
      push_vec3(lua, value);
      return 1;
    }

    constexpr luaL_Reg methods[] = {
      {"__index", vec3_index},
      {"__newindex", vec3_newindex},
      {"__add", vec3_add},
      {"__sub", vec3_sub},
      {"__mul", vec3_mul},
      {"__div", vec3_div},
      {"__unm", vec3_unm},
      {"__eq", vec3_eq},
      {"__tostring", vec3_tostring},
      {"length", vec3_length},
      {"normalized", vec3_normalized},
      {"dot", vec3_dot},
      {"cross", vec3_cross},
      {"distance", vec3_distance},
      {"copy", vec3_copy},
      {nullptr, nullptr}
    };
  }

  void open_vec3_handles(lua_State *lua)
  {
    if (luaL_newmetatable(lua, metatable) != 0) { luaL_setfuncs(lua, methods, 0); }
    lua_pop(lua, 1);

    lua_pushcfunction(lua, vec3_new);
    lua_setglobal(lua, "vec3");
  }

  void push_vec3(lua_State *lua, const glm::vec3 &value)
  {
    new_handle(lua)->value = value;
  }

  void push_bound_vec3(lua_State *lua, const glm::vec3 &value, const LuaComponentHandle &target, const FieldInfo &field)
  {
    LuaVec3Handle *handle = new_handle(lua);
    handle->value = value;
    handle->target = target;
    handle->field = &field;
  }

  glm::vec3 check_vec3(lua_State *lua, const int index)
  {
    auto &handle = *static_cast<LuaVec3Handle *>(luaL_checkudata(lua, index, metatable));
    return value_of(lua, handle);
  }

  LuaVec3Handle *test_vec3(lua_State *lua, const int index)
  {
    return static_cast<LuaVec3Handle *>(luaL_testudata(lua, index, metatable));
  }
} // neon
