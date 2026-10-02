#include "lua-sandbox.hpp"

#include <algorithm>

#include "lua-api.hpp"
#include "lua-host.hpp"

namespace neon
{
  // Helpers of the sandbox, for this file alone.
  namespace
  {
    // the modules `require` has loaded, in the registry, by module name
    constexpr const char *modules_key = "neon.modules";

    /// Adds a traceback to an error message, which pcall leaves to the
    /// handler. The debug library is not open to scripts, but the engine
    /// may use it.
    int add_traceback(lua_State *lua)
    {
      const char *message = lua_tostring(lua, 1);
      luaL_traceback(lua, lua, message, 1);
      return 1;
    }

    /// The file of a module name: dots become slashes under the folder.
    /// Returns false for a name that is not a module: empty, leaving the
    /// folder, or a file whose name says a kind, which the engine loads.
    bool path_of_module(const std::string &folder, const std::string &name, std::string &path)
    {
      if (name.empty() || name.front() == '.' || name.back() == '.' || name.find("..") != std::string::npos
          || name.find('/') != std::string::npos)
      {
        return false;
      }

      std::string relative = name;
      std::ranges::replace(relative, '.', '/');

      if (relative.ends_with("/component") || relative.ends_with("/system")) { return false; }

      path = (folder.ends_with('/') ? folder : folder + "/") + relative + ".lua";
      return true;
    }

    /// `require(name)`: the module, loaded once.
    int sandbox_require(lua_State *lua)
    {
      const std::string name = luaL_checkstring(lua, 1);

      lua_getfield(lua, LUA_REGISTRYINDEX, modules_key);
      lua_getfield(lua, -1, name.c_str());
      if (!lua_isnil(lua, -1)) { return 1; }
      lua_pop(lua, 1);

      const LuaHost &host = host_of(lua);

      std::string path;
      if (!path_of_module(host.folder, name, path))
      {
        return luaL_error(
          lua,
          "'%s' is not a module: a module is named by its path below %s with dots, such as lib.tween, and a file that declares a component or a system is not required",
          name.c_str(),
          host.folder.c_str());
      }

      std::string text;
      if (host.file_system == nullptr || !host.file_system->ReadText(path, text))
      {
        return luaL_error(lua, "The module '%s' is not there: %s cannot be read", name.c_str(), path.c_str());
      }

      const int results = run_script(lua, path, text);
      if (results < 0) { return lua_error(lua); }

      // what the module returned, or true for one that returns nothing, as
      // Lua does
      if (results == 0) { lua_pushboolean(lua, 1); }
      else if (results > 1) { lua_pop(lua, results - 1); }

      lua_pushvalue(lua, -1);
      lua_setfield(lua, -3, name.c_str());
      return 1;
    }

    /// Opens one standard library under its name.
    void open_library(lua_State *lua, const char *name, const lua_CFunction open)
    {
      luaL_requiref(lua, name, open, 1);
      lua_pop(lua, 1);
    }

    /// Takes a function away from the globals.
    void remove_global(lua_State *lua, const char *name)
    {
      lua_pushnil(lua);
      lua_setglobal(lua, name);
    }
  }

  void open_sandbox(lua_State *lua)
  {
    open_library(lua, LUA_GNAME, luaopen_base);
    open_library(lua, LUA_STRLIBNAME, luaopen_string);
    open_library(lua, LUA_TABLIBNAME, luaopen_table);
    open_library(lua, LUA_MATHLIBNAME, luaopen_math);
    open_library(lua, LUA_UTF8LIBNAME, luaopen_utf8);
    open_library(lua, LUA_COLIBNAME, luaopen_coroutine);

    // nothing reads a file or runs text from outside the scripts
    remove_global(lua, "dofile");
    remove_global(lua, "loadfile");
    remove_global(lua, "load");
    remove_global(lua, "loadstring");

    lua_newtable(lua);
    lua_setfield(lua, LUA_REGISTRYINDEX, modules_key);

    lua_pushcfunction(lua, sandbox_require);
    lua_setglobal(lua, "require");
  }

  std::string module_name_of(const std::string &folder, const std::string &path)
  {
    std::string name = path;
    const std::string prefix = folder.ends_with('/') ? folder : folder + "/";
    if (name.starts_with(prefix)) { name = name.substr(prefix.size()); }
    if (name.ends_with(".lua")) { name = name.substr(0, name.size() - 4); }
    std::ranges::replace(name, '/', '.');
    return name;
  }

  int run_script(lua_State *lua, const std::string &path, const std::string &text)
  {
    const int base = lua_gettop(lua);

    lua_pushcfunction(lua, add_traceback);
    const int handler = lua_gettop(lua);

    // the chunk is named after the path, so a message says where
    const std::string chunk = "=" + path;
    if (luaL_loadbuffer(lua, text.data(), text.size(), chunk.c_str()) != LUA_OK)
    {
      lua_remove(lua, handler);
      return -1;
    }

    if (lua_pcall(lua, 0, LUA_MULTRET, handler) != LUA_OK)
    {
      lua_remove(lua, handler);
      return -1;
    }

    lua_remove(lua, handler);
    return lua_gettop(lua) - base;
  }
} // neon
