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
    Collision
  };

  /// The names of the hooks, in the order of LuaHook.
  constexpr std::array<std::string_view, 7> lua_hook_names = {
    "ready", "update", "fixed_update", "removed", "on_trigger_enter", "on_trigger_exit", "on_collision"
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
    std::array<bool, 7> hooks{};

    /// The reference of the class table in the registry.
    int ref = -1;
  };
} // neon

#endif //LUA_SYSTEM_DECLARATION_HPP
