#ifndef LUA_ENTITY_HANDLE_HPP
#define LUA_ENTITY_HANDLE_HPP

#include <neon/world-system/ecs/entity.hpp>

struct lua_State;

namespace neon
{
  /// What a script holds of an entity: its id. The store it belongs to is
  /// the host's, so a handle stays valid across frames and says so when
  /// the entity is gone.
  struct LuaEntityHandle
  {
    Entity entity = No_Entity;
  };

  /// Adds the metatable of entities to the state, once.
  void open_entity_handles(lua_State *lua);

  /// Pushes a handle of the entity.
  void push_entity(lua_State *lua, Entity entity);

  /// The handle at the index, or a Lua error when there is none.
  [[nodiscard]] LuaEntityHandle &check_entity(lua_State *lua, int index);

  /// The handle at the index, or nullptr when the value is none.
  [[nodiscard]] LuaEntityHandle *test_entity(lua_State *lua, int index);
} // neon

#endif //LUA_ENTITY_HANDLE_HPP
