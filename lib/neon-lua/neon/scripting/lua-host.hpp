#ifndef LUA_HOST_HPP
#define LUA_HOST_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <neon/filesystem/file-system-context.hpp>
#include <neon/input/input-context.hpp>
#include <neon/logging/logger.hpp>
#include <neon/settings/settings-store.hpp>
#include <neon/ui/ui-context.hpp>
#include <neon/world-system/ecs/entity-store.hpp>
#include <neon/world-system/ecs/scene-file/component-format.hpp>
#include <neon/world-system/world-system.hpp>

struct lua_State;

namespace neon
{
  /// What the functions a script calls reach of the engine. One is kept in
  /// the registry of the Lua state, so that a function finds it from the
  /// state alone.
  ///
  /// `store` is set while the scripts are loaded and while a hook runs, and
  /// null between frames, since the store is what the world hands over.
  /// `input`, `ui`, `settings`, and `world` may be null for a run without
  /// them: a script that asks is then told so.
  struct LuaHost
  {
    EntityStore *store = nullptr;
    FileSystemContext *file_system = nullptr;
    InputContext *input = nullptr;
    UiContext *ui = nullptr;
    SettingsStore *settings = nullptr;
    WorldSystem *world = nullptr;
    const ComponentFormats *formats = nullptr;
    std::shared_ptr<Logger> logger;

    /// The folder the scripts are read from, which `require` stays inside.
    std::string folder;

    /// The queries `world.each` made, by the names it was given, kept so
    /// that a loop in a hook does not make one every frame.
    std::unordered_map<std::string, QueryId> queries;

    /// What `settings.on_change` subscribed, for as long as the state
    /// lives: ended before the state is closed, since each calls into it.
    std::vector<SettingsSubscription> subscriptions;
  };

  /// The host of a state. The state has to have been given one.
  [[nodiscard]] LuaHost &host_of(lua_State *lua);

  /// Gives a state its host.
  void set_host(lua_State *lua, LuaHost *host);
} // neon

#endif //LUA_HOST_HPP
