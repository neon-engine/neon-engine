#include "lua-host.hpp"

#include "lua-api.hpp"

namespace neon
{
  // Helpers of the host, for this file alone.
  namespace
  {
    // the key of the host in the registry
    constexpr const char *host_key = "neon.host";
  }

  LuaHost &host_of(lua_State *lua)
  {
    lua_getfield(lua, LUA_REGISTRYINDEX, host_key);
    auto *host = static_cast<LuaHost *>(lua_touserdata(lua, -1));
    lua_pop(lua, 1);
    return *host;
  }

  void set_host(lua_State *lua, LuaHost *host)
  {
    lua_pushlightuserdata(lua, host);
    lua_setfield(lua, LUA_REGISTRYINDEX, host_key);
  }
} // neon
