#include "lua-color-handle.hpp"

#include <cstring>

#include "lua-api.hpp"
#include "lua-host.hpp"

namespace neon
{
  // Helpers of the color handles, for this file alone.
  namespace
  {
    constexpr const char *metatable = "neon.color";

    LuaColorHandle *new_handle(lua_State *lua)
    {
      auto *handle = static_cast<LuaColorHandle *>(lua_newuserdatauv(lua, sizeof(LuaColorHandle), 0));
      new(handle) LuaColorHandle();
      luaL_setmetatable(lua, metatable);
      return handle;
    }

    Color value_of(lua_State *lua, LuaColorHandle &handle)
    {
      if (handle.IsBound())
      {
        if (EntityStore *store = host_of(lua).store)
        {
          if (void *object = handle.target.Resolve(*store))
          {
            const FieldValue read = handle.field->get(object);
            if (const auto *held = std::get_if<Color>(&read)) { handle.value = *held; }
          }
        }
      }
      return handle.value;
    }

    void write_back(lua_State *lua, const LuaColorHandle &handle)
    {
      if (!handle.IsBound()) { return; }

      EntityStore *store = host_of(lua).store;
      void *object = store != nullptr ? handle.target.Resolve(*store) : nullptr;
      if (object == nullptr)
      {
        luaL_error(lua, "The entity no longer has the component this color belongs to");
        return;
      }
      handle.field->set(object, handle.value);
    }

    float &channel(Color &value, lua_State *lua, const char *key)
    {
      if (std::strcmp(key, "r") == 0) { return value.r; }
      if (std::strcmp(key, "g") == 0) { return value.g; }
      if (std::strcmp(key, "b") == 0) { return value.b; }
      if (std::strcmp(key, "a") == 0) { return value.a; }
      luaL_error(lua, "A color has r, g, b, and a, not '%s'", key);
      return value.r;
    }

    int color_index(lua_State *lua)
    {
      auto &handle = *static_cast<LuaColorHandle *>(luaL_checkudata(lua, 1, metatable));

      if (lua_type(lua, 2) == LUA_TSTRING)
      {
        luaL_getmetatable(lua, metatable);
        lua_pushvalue(lua, 2);
        lua_rawget(lua, -2);
        if (!lua_isnil(lua, -1)) { return 1; }
        lua_pop(lua, 2);
      }

      Color value = value_of(lua, handle);
      lua_pushnumber(lua, channel(value, lua, luaL_checkstring(lua, 2)));
      return 1;
    }

    int color_newindex(lua_State *lua)
    {
      auto &handle = *static_cast<LuaColorHandle *>(luaL_checkudata(lua, 1, metatable));
      const char *key = luaL_checkstring(lua, 2);
      const auto number = static_cast<float>(luaL_checknumber(lua, 3));

      value_of(lua, handle);
      channel(handle.value, lua, key) = number;
      write_back(lua, handle);
      return 0;
    }

    int color_eq(lua_State *lua)
    {
      const Color left = check_color(lua, 1);
      const Color right = check_color(lua, 2);
      lua_pushboolean(lua, left.r == right.r && left.g == right.g && left.b == right.b && left.a == right.a ? 1 : 0);
      return 1;
    }

    int color_tostring(lua_State *lua)
    {
      const Color value = check_color(lua, 1);
      lua_pushfstring(
        lua,
        "color(%f, %f, %f, %f)",
        static_cast<double>(value.r),
        static_cast<double>(value.g),
        static_cast<double>(value.b),
        static_cast<double>(value.a));
      return 1;
    }

    int color_copy(lua_State *lua)
    {
      push_color(lua, check_color(lua, 1));
      return 1;
    }

    /// `color(r, g, b)` with alpha 1, or `color(r, g, b, a)`.
    int color_new(lua_State *lua)
    {
      const int count = lua_gettop(lua);
      if (count != 3 && count != 4) { return luaL_error(lua, "color takes red, green, and blue, and alpha if wanted"); }

      Color value;
      value.r = static_cast<float>(luaL_checknumber(lua, 1));
      value.g = static_cast<float>(luaL_checknumber(lua, 2));
      value.b = static_cast<float>(luaL_checknumber(lua, 3));
      value.a = count == 4 ? static_cast<float>(luaL_checknumber(lua, 4)) : 1.0f;
      push_color(lua, value);
      return 1;
    }

    constexpr luaL_Reg methods[] = {
      {"__index", color_index},
      {"__newindex", color_newindex},
      {"__eq", color_eq},
      {"__tostring", color_tostring},
      {"copy", color_copy},
      {nullptr, nullptr}
    };
  }

  void open_color_handles(lua_State *lua)
  {
    if (luaL_newmetatable(lua, metatable) != 0) { luaL_setfuncs(lua, methods, 0); }
    lua_pop(lua, 1);

    lua_pushcfunction(lua, color_new);
    lua_setglobal(lua, "color");
  }

  void push_color(lua_State *lua, const Color &value)
  {
    new_handle(lua)->value = value;
  }

  void push_bound_color(lua_State *lua, const Color &value, const LuaComponentHandle &target, const FieldInfo &field)
  {
    LuaColorHandle *handle = new_handle(lua);
    handle->value = value;
    handle->target = target;
    handle->field = &field;
  }

  Color check_color(lua_State *lua, const int index)
  {
    auto &handle = *static_cast<LuaColorHandle *>(luaL_checkudata(lua, index, metatable));
    return value_of(lua, handle);
  }

  LuaColorHandle *test_color(lua_State *lua, const int index)
  {
    return static_cast<LuaColorHandle *>(luaL_testudata(lua, index, metatable));
  }
} // neon
