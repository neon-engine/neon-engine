#ifndef LUA_API_HPP
#define LUA_API_HPP

// Lua is C. Everything of this library includes it through here and nothing
// outside the library includes it at all.
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#endif //LUA_API_HPP
