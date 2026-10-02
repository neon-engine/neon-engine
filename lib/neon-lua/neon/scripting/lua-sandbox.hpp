#ifndef LUA_SANDBOX_HPP
#define LUA_SANDBOX_HPP

#include <string>

struct lua_State;

namespace neon
{
  /// Opens what a script may use of Lua and nothing else: the base
  /// functions, strings, tables, math, utf8, and coroutines. There is no
  /// `io`, no `os`, no `debug`, and no `load` of text from outside the
  /// scripts, and `print` goes to the engine's log. `require` resolves
  /// inside the scripts' folder alone, through the engine's file system.
  void open_sandbox(lua_State *lua);

  /// The module name of a script, for `require` and for `package.loaded`:
  /// its path below the scripts' folder, without `.lua`, with dots for the
  /// slashes. `assets://scripts/lib/tween.lua` is `lib.tween`.
  [[nodiscard]] std::string module_name_of(const std::string &folder, const std::string &path);

  /// Runs the text of a script as a chunk named after its path and leaves
  /// what it returns on the stack. Returns the number of values, or -1 with
  /// the message on the stack when the script fails.
  int run_script(lua_State *lua, const std::string &path, const std::string &text);
} // neon

#endif //LUA_SANDBOX_HPP
