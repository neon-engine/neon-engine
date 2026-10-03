#include "lua-api.hpp"

#include <cstring>

namespace neon
{
  // Helpers of parameters_of(), for this file alone.
  namespace
  {
    // LuaJIT's lua_Debug has no nparams; its debug.getinfo gives one.
    // That function is kept in the registry under this key, since the
    // sandbox does not let the scripts have the debug library.
    constexpr char getinfo_key[] = "neon.debug.getinfo";
  }

  void open_compat(lua_State *lua)
  {
    luaL_requiref(lua, LUA_DBLIBNAME, luaopen_debug, 0);
    lua_getfield(lua, -1, "getinfo");
    lua_setfield(lua, LUA_REGISTRYINDEX, getinfo_key);
    lua_pop(lua, 1);

    // LuaJIT's luaopen_* set the global themselves; the scripts get no debug
    lua_pushnil(lua);
    lua_setglobal(lua, LUA_DBLIBNAME);

    // and the library is not left where require could find it
    luaL_findtable(lua, LUA_REGISTRYINDEX, "_LOADED", 1);
    lua_pushnil(lua);
    lua_setfield(lua, -2, LUA_DBLIBNAME);
    lua_pop(lua, 1);
  }

  int parameters_of(lua_State *lua, const int index)
  {
    const int function = lua_absindex(lua, index);
    lua_getfield(lua, LUA_REGISTRYINDEX, getinfo_key);
    lua_pushvalue(lua, function);
    lua_pushstring(lua, "Su");
    lua_call(lua, 2, 1);

    lua_getfield(lua, -1, "what");
    const bool in_lua = lua_isstring(lua, -1) && std::strcmp(lua_tostring(lua, -1), "Lua") == 0;
    lua_getfield(lua, -2, "isvararg");
    const bool vararg = lua_toboolean(lua, -1) != 0;
    lua_getfield(lua, -3, "nparams");
    const int parameters = static_cast<int>(lua_tointeger(lua, -1));
    lua_pop(lua, 4);

    return !in_lua || vararg ? -1 : parameters;
  }
} // neon
