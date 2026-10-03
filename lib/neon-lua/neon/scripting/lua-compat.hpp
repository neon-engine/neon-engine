#ifndef LUA_COMPAT_HPP
#define LUA_COMPAT_HPP

// The functions of the Lua 5.4 API the bindings are written to and LuaJIT,
// whose API is that of Lua 5.1 with parts of 5.2, does not have. Included
// by lua-api.hpp after LuaJIT itself.
//
// A user value of a userdata, which Lua 5.4 keeps beside it, is here its
// environment table: lua_newuserdatauv() with one user value gives the
// userdata a table of its own, and lua_getiuservalue() reads it. The
// bindings use one user value at most, as a cache of handles.

#include <cmath>
#include <cstddef>

namespace neon
{
  /// How many parameters a Lua function takes, or -1 when it takes any
  /// number, or is not written in Lua.
  int parameters_of(lua_State *lua, int index);

  /// Keeps what parameters_of() needs. Call it once on a new state.
  void open_compat(lua_State *lua);
} // neon

#define LUA_GNAME "_G"

inline std::size_t lua_rawlen(lua_State *lua, const int index)
{
  return lua_objlen(lua, index);
}

inline int lua_absindex(lua_State *lua, const int index)
{
  return index > 0 || index <= LUA_REGISTRYINDEX ? index : lua_gettop(lua) + index + 1;
}

/// LuaJIT has one kind of number. A number is whole when it has no
/// fraction and fits a lua_Integer, which is what a whole field takes.
inline int lua_isinteger(lua_State *lua, const int index)
{
  if (lua_type(lua, index) != LUA_TNUMBER) { return 0; }
  const lua_Number number = lua_tonumber(lua, index);
  return number == std::floor(number) && number >= -9223372036854775808.0 && number < 9223372036854775808.0 ? 1 : 0;
}

inline void *lua_newuserdatauv(lua_State *lua, const std::size_t size, const int user_values)
{
  void *made = lua_newuserdata(lua, size);
  if (user_values > 0)
  {
    lua_newtable(lua);
    lua_setfenv(lua, -2);
  }
  return made;
}

inline int lua_getiuservalue(lua_State *lua, const int index, const int)
{
  lua_getfenv(lua, index);
  return lua_type(lua, -1);
}

inline int lua_setiuservalue(lua_State *lua, const int index, const int)
{
  return lua_setfenv(lua, index);
}

inline const char *luaL_tolstring(lua_State *lua, const int index, std::size_t *length)
{
  if (luaL_callmeta(lua, index, "__tostring"))
  {
    if (!lua_isstring(lua, -1)) { luaL_error(lua, "'__tostring' must return a string"); }
  }
  else
  {
    switch (lua_type(lua, index))
    {
      case LUA_TNUMBER:
      case LUA_TSTRING:
        lua_pushvalue(lua, index);
        break;
      case LUA_TBOOLEAN:
        lua_pushstring(lua, lua_toboolean(lua, index) ? "true" : "false");
        break;
      case LUA_TNIL:
        lua_pushliteral(lua, "nil");
        break;
      default:
        lua_pushfstring(lua, "%s: %p", luaL_typename(lua, index), lua_topointer(lua, index));
        break;
    }
  }
  return lua_tolstring(lua, -1, length);
}

/// Opens a library with `open`, keeps it in package.loaded under `name`,
/// and sets it as a global when `global` is not 0. The library is left on
/// the stack.
inline void luaL_requiref(lua_State *lua, const char *name, const lua_CFunction open, const int global)
{
  lua_pushcfunction(lua, open);
  lua_pushstring(lua, name);
  lua_call(lua, 1, 1);
  luaL_findtable(lua, LUA_REGISTRYINDEX, "_LOADED", 1);
  lua_pushvalue(lua, -2);
  lua_setfield(lua, -2, name);
  lua_pop(lua, 1);
  if (global)
  {
    lua_pushvalue(lua, -1);
    lua_setglobal(lua, name);
  }
}

#endif //LUA_COMPAT_HPP
