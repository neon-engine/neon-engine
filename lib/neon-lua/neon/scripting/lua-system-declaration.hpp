#ifndef LUA_SYSTEM_DECLARATION_HPP
#define LUA_SYSTEM_DECLARATION_HPP

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace neon
{
  /// The hooks a system may define, in the order a script reads about them.
  enum class LuaHook
  {
    Ready = 0,
    Update,
    FixedUpdate,
    Removed,
    TriggerEnter,
    TriggerExit,
    Collision,
    Possessed,
    Unpossessed
  };

  /// How many hooks there are.
  constexpr std::size_t lua_hook_count = 9;

  /// The names of the hooks, in the order of LuaHook.
  constexpr std::array<std::string_view, lua_hook_count> lua_hook_names = {
    "ready", "update", "fixed_update", "removed", "on_trigger_enter", "on_trigger_exit", "on_collision",
    "on_possessed", "on_unpossessed"
  };

  /// What a script declared with `System:extend`, read off its class table:
  /// the components it runs over, by name, and which hooks it defines. The
  /// functions themselves stay in the class table, which the registry keeps
  /// a reference to.
  struct LuaSystemDeclaration
  {
    std::string name;

    std::vector<std::string> components;

    /// Whether the hook is defined, in the order of LuaHook.
    std::array<bool, lua_hook_count> hooks{};

    /// The reference of the class table in the registry.
    int ref = -1;
  };
} // neon

#endif //LUA_SYSTEM_DECLARATION_HPP
