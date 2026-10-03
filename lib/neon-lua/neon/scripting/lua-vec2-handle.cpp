#include "lua-vec2-handle.hpp"

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
    constexpr const char *metatable = "neon.vec2";

    LuaVec2Handle *new_handle(lua_State *lua)
    {
      auto *handle = static_cast<LuaVec2Handle *>(lua_newuserdatauv(lua, sizeof(LuaVec2Handle), 0));
      new(handle) LuaVec2Handle();
      luaL_setmetatable(lua, metatable);
      return handle;
    }

    /// The value of a handle: from the component while it is bound and the
    /// component is there, which is where a script may have changed it
    /// through another handle, and its own otherwise.
    glm::vec2 value_of(lua_State *lua, LuaVec2Handle &handle)
    {
      if (handle.IsBound())
      {
        if (EntityStore *store = host_of(lua).store)
        {
          if (void *object = handle.target.Resolve(*store))
          {
            const FieldValue read = handle.field->get(object);
            if (const auto *held = std::get_if<glm::vec2>(&read)) { handle.value = *held; }
          }
        }
      }
      return handle.value;
    }

    /// Writes the value of a handle through to its component, if bound.
    void write_back(lua_State *lua, const LuaVec2Handle &handle)
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
      return luaL_error(lua, "A vec2 has x, and y, not '%s'", key);
    }

    int vec2_index(lua_State *lua)
    {
      auto &handle = *static_cast<LuaVec2Handle *>(luaL_checkudata(lua, 1, metatable));

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

    int vec2_newindex(lua_State *lua)
    {
      auto &handle = *static_cast<LuaVec2Handle *>(luaL_checkudata(lua, 1, metatable));
      const int axis = axis_index(lua, luaL_checkstring(lua, 2));
      const auto number = static_cast<float>(luaL_checknumber(lua, 3));

      value_of(lua, handle);
      handle.value[axis] = number;
      write_back(lua, handle);
      return 0;
    }

    int vec2_add(lua_State *lua)
    {
      push_vec2(lua, check_vec2(lua, 1) + check_vec2(lua, 2));
      return 1;
    }

    int vec2_sub(lua_State *lua)
    {
      push_vec2(lua, check_vec2(lua, 1) - check_vec2(lua, 2));
      return 1;
    }

    int vec2_mul(lua_State *lua)
    {
      if (lua_isnumber(lua, 1)) { push_vec2(lua, static_cast<float>(lua_tonumber(lua, 1)) * check_vec2(lua, 2)); }
      else if (lua_isnumber(lua, 2)) { push_vec2(lua, check_vec2(lua, 1) * static_cast<float>(lua_tonumber(lua, 2))); }
      else { push_vec2(lua, check_vec2(lua, 1) * check_vec2(lua, 2)); }
      return 1;
    }

    int vec2_div(lua_State *lua)
    {
      push_vec2(lua, check_vec2(lua, 1) / static_cast<float>(luaL_checknumber(lua, 2)));
      return 1;
    }

    int vec2_unm(lua_State *lua)
    {
      push_vec2(lua, -check_vec2(lua, 1));
      return 1;
    }

    int vec2_eq(lua_State *lua)
    {
      lua_pushboolean(lua, check_vec2(lua, 1) == check_vec2(lua, 2) ? 1 : 0);
      return 1;
    }

    int vec2_tostring(lua_State *lua)
    {
      const glm::vec2 value = check_vec2(lua, 1);
      lua_pushfstring(lua, "vec2(%f, %f)", static_cast<double>(value.x), static_cast<double>(value.y));
      return 1;
    }

    int vec2_length(lua_State *lua)
    {
      lua_pushnumber(lua, glm::length(check_vec2(lua, 1)));
      return 1;
    }

    int vec2_normalized(lua_State *lua)
    {
      const glm::vec2 value = check_vec2(lua, 1);
      push_vec2(lua, glm::length(value) > 0.0f ? glm::normalize(value) : value);
      return 1;
    }

    int vec2_dot(lua_State *lua)
    {
      lua_pushnumber(lua, glm::dot(check_vec2(lua, 1), check_vec2(lua, 2)));
      return 1;
    }

    int vec2_distance(lua_State *lua)
    {
      lua_pushnumber(lua, glm::distance(check_vec2(lua, 1), check_vec2(lua, 2)));
      return 1;
    }

    /// A copy of its own, no longer bound to a component.
    int vec2_copy(lua_State *lua)
    {
      push_vec2(lua, check_vec2(lua, 1));
      return 1;
    }

    /// `vec2(x, y)`, `vec2(n)` for both, or `vec2()` for zero.
    int vec2_new(lua_State *lua)
    {
      const int count = lua_gettop(lua);
      glm::vec2 value{0.0f};
      if (count == 1) { value = glm::vec2(static_cast<float>(luaL_checknumber(lua, 1))); }
      else if (count >= 2)
      {
        value = glm::vec2(static_cast<float>(luaL_checknumber(lua, 1)), static_cast<float>(luaL_checknumber(lua, 2)));
      }
      else if (count != 0) { return luaL_error(lua, "vec2 takes two numbers, one for both, or none"); }
      push_vec2(lua, value);
      return 1;
    }

    constexpr luaL_Reg methods[] = {
      {"__index", vec2_index},
      {"__newindex", vec2_newindex},
      {"__add", vec2_add},
      {"__sub", vec2_sub},
      {"__mul", vec2_mul},
      {"__div", vec2_div},
      {"__unm", vec2_unm},
      {"__eq", vec2_eq},
      {"__tostring", vec2_tostring},
      {"length", vec2_length},
      {"normalized", vec2_normalized},
      {"dot", vec2_dot},
      {"distance", vec2_distance},
      {"copy", vec2_copy},
      {nullptr, nullptr}
    };
  }

  void open_vec2_handles(lua_State *lua)
  {
    if (luaL_newmetatable(lua, metatable) != 0) { luaL_setfuncs(lua, methods, 0); }
    lua_pop(lua, 1);

    lua_pushcfunction(lua, vec2_new);
    lua_setglobal(lua, "vec2");
  }

  void push_vec2(lua_State *lua, const glm::vec2 &value)
  {
    new_handle(lua)->value = value;
  }

  void push_bound_vec2(lua_State *lua, const glm::vec2 &value, const LuaComponentHandle &target, const FieldInfo &field)
  {
    LuaVec2Handle *handle = new_handle(lua);
    handle->value = value;
    handle->target = target;
    handle->field = &field;
  }

  glm::vec2 check_vec2(lua_State *lua, const int index)
  {
    auto &handle = *static_cast<LuaVec2Handle *>(luaL_checkudata(lua, index, metatable));
    return value_of(lua, handle);
  }

  LuaVec2Handle *test_vec2(lua_State *lua, const int index)
  {
    return static_cast<LuaVec2Handle *>(luaL_testudata(lua, index, metatable));
  }
} // neon
