#ifndef LUA_API_HPP
#define LUA_API_HPP

// Lua is C, and the Lua is LuaJIT. Everything of this library includes it
// through here and nothing outside the library includes it at all.
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <luajit.h>
}

// the functions of the later Lua API the bindings are written to, which
// LuaJIT's, that of Lua 5.1, does not have
#include "lua-compat.hpp"

#endif //LUA_API_HPP
