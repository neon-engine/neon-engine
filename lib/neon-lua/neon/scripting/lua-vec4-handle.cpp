#include "lua-vec4-handle.hpp"

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
    constexpr const char *metatable = "neon.vec4";

    LuaVec4Handle *new_handle(lua_State *lua)
    {
      auto *handle = static_cast<LuaVec4Handle *>(lua_newuserdatauv(lua, sizeof(LuaVec4Handle), 0));
      new(handle) LuaVec4Handle();
      luaL_setmetatable(lua, metatable);
      return handle;
    }

    /// The value of a handle: from the component while it is bound and the
    /// component is there, which is where a script may have changed it
    /// through another handle, and its own otherwise.
    glm::vec4 value_of(lua_State *lua, LuaVec4Handle &handle)
    {
      if (handle.IsBound())
      {
        if (EntityStore *store = host_of(lua).store)
        {
          if (void *object = handle.target.Resolve(*store))
          {
            const FieldValue read = handle.field->get(object);
            if (const auto *held = std::get_if<glm::vec4>(&read)) { handle.value = *held; }
          }
        }
      }
      return handle.value;
    }

    /// Writes the value of a handle through to its component, if bound.
    void write_back(lua_State *lua, const LuaVec4Handle &handle)
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
      if (std::strcmp(key, "w") == 0) { return 3; }
      return luaL_error(lua, "A vec4 has x, y, z, and w, not '%s'", key);
    }

    int vec4_index(lua_State *lua)
    {
      auto &handle = *static_cast<LuaVec4Handle *>(luaL_checkudata(lua, 1, metatable));

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

    int vec4_newindex(lua_State *lua)
    {
      auto &handle = *static_cast<LuaVec4Handle *>(luaL_checkudata(lua, 1, metatable));
      const int axis = axis_index(lua, luaL_checkstring(lua, 2));
      const auto number = static_cast<float>(luaL_checknumber(lua, 3));

      value_of(lua, handle);
      handle.value[axis] = number;
      write_back(lua, handle);
      return 0;
    }

    int vec4_add(lua_State *lua)
    {
      push_vec4(lua, check_vec4(lua, 1) + check_vec4(lua, 2));
      return 1;
    }

    int vec4_sub(lua_State *lua)
    {
      push_vec4(lua, check_vec4(lua, 1) - check_vec4(lua, 2));
      return 1;
    }

    int vec4_mul(lua_State *lua)
    {
      if (lua_isnumber(lua, 1)) { push_vec4(lua, static_cast<float>(lua_tonumber(lua, 1)) * check_vec4(lua, 2)); }
      else if (lua_isnumber(lua, 2)) { push_vec4(lua, check_vec4(lua, 1) * static_cast<float>(lua_tonumber(lua, 2))); }
      else { push_vec4(lua, check_vec4(lua, 1) * check_vec4(lua, 2)); }
      return 1;
    }

    int vec4_div(lua_State *lua)
    {
      push_vec4(lua, check_vec4(lua, 1) / static_cast<float>(luaL_checknumber(lua, 2)));
      return 1;
    }

    int vec4_unm(lua_State *lua)
    {
      push_vec4(lua, -check_vec4(lua, 1));
      return 1;
    }

    int vec4_eq(lua_State *lua)
    {
      lua_pushboolean(lua, check_vec4(lua, 1) == check_vec4(lua, 2) ? 1 : 0);
      return 1;
    }

    int vec4_tostring(lua_State *lua)
    {
      const glm::vec4 value = check_vec4(lua, 1);
      lua_pushfstring(lua, "vec4(%f, %f, %f, %f)", static_cast<double>(value.x), static_cast<double>(value.y), static_cast<double>(value.z), static_cast<double>(value.w));
      return 1;
    }

    int vec4_length(lua_State *lua)
    {
      lua_pushnumber(lua, glm::length(check_vec4(lua, 1)));
      return 1;
    }

    int vec4_normalized(lua_State *lua)
    {
      const glm::vec4 value = check_vec4(lua, 1);
      push_vec4(lua, glm::length(value) > 0.0f ? glm::normalize(value) : value);
      return 1;
    }

    int vec4_dot(lua_State *lua)
    {
      lua_pushnumber(lua, glm::dot(check_vec4(lua, 1), check_vec4(lua, 2)));
      return 1;
    }

    int vec4_distance(lua_State *lua)
    {
      lua_pushnumber(lua, glm::distance(check_vec4(lua, 1), check_vec4(lua, 2)));
      return 1;
    }

    /// A copy of its own, no longer bound to a component.
    int vec4_copy(lua_State *lua)
    {
      push_vec4(lua, check_vec4(lua, 1));
      return 1;
    }

    /// `vec4(x, y, z, w)`, `vec4(n)` for all four, or `vec4()` for zero.
    int vec4_new(lua_State *lua)
    {
      const int count = lua_gettop(lua);
      glm::vec4 value{0.0f};
      if (count == 1) { value = glm::vec4(static_cast<float>(luaL_checknumber(lua, 1))); }
      else if (count >= 4)
      {
        value = glm::vec4(
          static_cast<float>(luaL_checknumber(lua, 1)),
          static_cast<float>(luaL_checknumber(lua, 2)),
          static_cast<float>(luaL_checknumber(lua, 3)),
          static_cast<float>(luaL_checknumber(lua, 4)));
      }
      else if (count != 0) { return luaL_error(lua, "vec4 takes four numbers, one for all, or none"); }
      push_vec4(lua, value);
      return 1;
    }

    constexpr luaL_Reg methods[] = {
      {"__index", vec4_index},
      {"__newindex", vec4_newindex},
      {"__add", vec4_add},
      {"__sub", vec4_sub},
      {"__mul", vec4_mul},
      {"__div", vec4_div},
      {"__unm", vec4_unm},
      {"__eq", vec4_eq},
      {"__tostring", vec4_tostring},
      {"length", vec4_length},
      {"normalized", vec4_normalized},
      {"dot", vec4_dot},
      {"distance", vec4_distance},
      {"copy", vec4_copy},
      {nullptr, nullptr}
    };
  }

  void open_vec4_handles(lua_State *lua)
  {
    if (luaL_newmetatable(lua, metatable) != 0) { luaL_setfuncs(lua, methods, 0); }
    lua_pop(lua, 1);

    lua_pushcfunction(lua, vec4_new);
    lua_setglobal(lua, "vec4");
  }

  void push_vec4(lua_State *lua, const glm::vec4 &value)
  {
    new_handle(lua)->value = value;
  }

  void push_bound_vec4(lua_State *lua, const glm::vec4 &value, const LuaComponentHandle &target, const FieldInfo &field)
  {
    LuaVec4Handle *handle = new_handle(lua);
    handle->value = value;
    handle->target = target;
    handle->field = &field;
  }

  glm::vec4 check_vec4(lua_State *lua, const int index)
  {
    auto &handle = *static_cast<LuaVec4Handle *>(luaL_checkudata(lua, index, metatable));
    return value_of(lua, handle);
  }

  LuaVec4Handle *test_vec4(lua_State *lua, const int index)
  {
    return static_cast<LuaVec4Handle *>(luaL_testudata(lua, index, metatable));
  }
} // neon
