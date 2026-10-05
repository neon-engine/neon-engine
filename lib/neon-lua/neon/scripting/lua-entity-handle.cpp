#include "lua-entity-handle.hpp"

#include <string>

#include "lua-api.hpp"
#include "lua-component-handle.hpp"
#include "lua-host.hpp"

namespace neon
{
  // Helpers of the entity handles, for this file alone.
  namespace
  {
    constexpr const char *metatable = "neon.entity";

    EntityStore &store_of(lua_State *lua)
    {
      EntityStore *store = host_of(lua).store;
      if (store == nullptr) { luaL_error(lua, "The world is not running, so no entity can be reached"); }
      return *store;
    }

    /// Pushes the component of the entity by its name, or nil when the
    /// entity has none. A name no component is registered under is an
    /// error, since it is a mistake in the script.
    int push_component_of(lua_State *lua, const Entity entity, const char *name)
    {
      EntityStore &store = store_of(lua);
      const ComponentId id = store.FindComponent(name);
      if (id == No_Component) { return luaL_error(lua, "There is no component called '%s'", name); }
      if (!store.HasComponent(entity, id))
      {
        lua_pushnil(lua);
        return 1;
      }

      // the handle of this component read from this entity handle before,
      // kept in the entity's user value, so that `entity.Transform` every
      // frame makes nothing new
      if (lua_getiuservalue(lua, 1, 1) == LUA_TNIL)
      {
        lua_pop(lua, 1);
        lua_newtable(lua);
        lua_pushvalue(lua, -1);
        lua_setiuservalue(lua, 1, 1);
      }
      lua_getfield(lua, -1, name);
      if (LuaComponentHandle *kept = test_component(lua, -1))
      {
        kept->entity = entity;
        kept->pointer = nullptr;
        lua_remove(lua, -2);
        return 1;
      }
      lua_pop(lua, 1);

      LuaComponentHandle handle;
      handle.entity = entity;
      handle.component = id;
      const ComponentFormat *format = host_of(lua).formats != nullptr ? host_of(lua).formats->Find(name) : nullptr;
      handle.type = format != nullptr ? format->type.get() : nullptr;
      push_component(lua, handle);
      lua_pushvalue(lua, -1);
      lua_setfield(lua, -3, name);
      lua_remove(lua, -2);
      return 1;
    }

    int entity_has(lua_State *lua)
    {
      const Entity entity = check_entity(lua, 1).entity;
      const char *name = luaL_checkstring(lua, 2);
      EntityStore &store = store_of(lua);
      const ComponentId id = store.FindComponent(name);
      if (id == No_Component) { return luaL_error(lua, "There is no component called '%s'", name); }
      lua_pushboolean(lua, store.HasComponent(entity, id) ? 1 : 0);
      return 1;
    }

    int entity_get(lua_State *lua)
    {
      return push_component_of(lua, check_entity(lua, 1).entity, luaL_checkstring(lua, 2));
    }

    int entity_remove(lua_State *lua)
    {
      const Entity entity = check_entity(lua, 1).entity;
      const char *name = luaL_checkstring(lua, 2);
      EntityStore &store = store_of(lua);
      const ComponentId id = store.FindComponent(name);
      if (id == No_Component) { return luaL_error(lua, "There is no component called '%s'", name); }
      store.RemoveComponent(entity, id);
      return 0;
    }

    int entity_set_enabled(lua_State *lua)
    {
      const Entity entity = check_entity(lua, 1).entity;
      const char *name = luaL_checkstring(lua, 2);
      luaL_checktype(lua, 3, LUA_TBOOLEAN);
      EntityStore &store = store_of(lua);
      const ComponentId id = store.FindComponent(name);
      if (id == No_Component) { return luaL_error(lua, "There is no component called '%s'", name); }
      store.SetEnabled(entity, id, lua_toboolean(lua, 3) != 0);
      return 0;
    }

    int entity_is_enabled(lua_State *lua)
    {
      const Entity entity = check_entity(lua, 1).entity;
      const char *name = luaL_checkstring(lua, 2);
      EntityStore &store = store_of(lua);
      const ComponentId id = store.FindComponent(name);
      lua_pushboolean(lua, id != No_Component && store.IsEnabled(entity, id) ? 1 : 0);
      return 1;
    }

    int entity_parent(lua_State *lua)
    {
      const Entity parent = store_of(lua).GetParent(check_entity(lua, 1).entity);
      if (parent == No_Entity) { lua_pushnil(lua); }
      else { push_entity(lua, parent); }
      return 1;
    }

    int entity_children(lua_State *lua)
    {
      const std::vector<Entity> children = store_of(lua).GetChildren(check_entity(lua, 1).entity);
      lua_createtable(lua, static_cast<int>(children.size()), 0);
      for (std::size_t i = 0; i < children.size(); i++)
      {
        push_entity(lua, children[i]);
        lua_rawseti(lua, -2, static_cast<lua_Integer>(i + 1));
      }
      return 1;
    }

    int entity_alive(lua_State *lua)
    {
      lua_pushboolean(lua, store_of(lua).IsAlive(check_entity(lua, 1).entity) ? 1 : 0);
      return 1;
    }

    int entity_destroy(lua_State *lua)
    {
      store_of(lua).DestroyEntity(check_entity(lua, 1).entity);
      return 0;
    }

    /// The names of the entity from the top, with slashes between.
    std::string path_of(EntityStore &store, const Entity entity)
    {
      std::string path = store.GetName(entity);
      for (Entity parent = store.GetParent(entity); parent != No_Entity; parent = store.GetParent(parent))
      {
        path = store.GetName(parent) + "/" + path;
      }
      return path;
    }

    int entity_index(lua_State *lua)
    {
      const LuaEntityHandle &handle = check_entity(lua, 1);
      const char *key = luaL_checkstring(lua, 2);

      // the methods, then what the entity is
      luaL_getmetatable(lua, metatable);
      lua_pushvalue(lua, 2);
      lua_rawget(lua, -2);
      if (!lua_isnil(lua, -1)) { return 1; }
      lua_pop(lua, 2);

      const std::string name(key);
      if (name == "id")
      {
        lua_pushinteger(lua, static_cast<lua_Integer>(handle.entity));
        return 1;
      }
      if (name == "name")
      {
        lua_pushstring(lua, store_of(lua).GetName(handle.entity).c_str());
        return 1;
      }
      if (name == "path")
      {
        lua_pushstring(lua, path_of(store_of(lua), handle.entity).c_str());
        return 1;
      }

      // `entity.Transform` is the component, or nil
      return push_component_of(lua, handle.entity, key);
    }

    int entity_newindex(lua_State *lua)
    {
      return luaL_error(lua, "An entity takes no fields; change its components instead");
    }

    int entity_eq(lua_State *lua)
    {
      lua_pushboolean(lua, check_entity(lua, 1).entity == check_entity(lua, 2).entity ? 1 : 0);
      return 1;
    }

    int entity_tostring(lua_State *lua)
    {
      const Entity entity = check_entity(lua, 1).entity;
      EntityStore *store = host_of(lua).store;
      if (store != nullptr && store->IsAlive(entity))
      {
        lua_pushfstring(lua, "entity '%s'", path_of(*store, entity).c_str());
      }
      else { lua_pushfstring(lua, "entity %d, which is gone", static_cast<int>(entity)); }
      return 1;
    }

    constexpr luaL_Reg methods[] = {
      {"__index", entity_index},
      {"__newindex", entity_newindex},
      {"__eq", entity_eq},
      {"__tostring", entity_tostring},
      {"has_component", entity_has},
      {"get_component", entity_get},
      {"remove_component", entity_remove},
      {"set_enabled", entity_set_enabled},
      {"is_enabled", entity_is_enabled},
      {"get_parent", entity_parent},
      {"get_children", entity_children},
      {"is_alive", entity_alive},
      {"destroy", entity_destroy},
      {nullptr, nullptr}
    };
  }

  void open_entity_handles(lua_State *lua)
  {
    if (luaL_newmetatable(lua, metatable) != 0) { luaL_setfuncs(lua, methods, 0); }
    lua_pop(lua, 1);
  }

  void push_entity(lua_State *lua, const Entity entity)
  {
    // one user value: the components read from this handle, kept
    auto *handle = static_cast<LuaEntityHandle *>(lua_newuserdatauv(lua, sizeof(LuaEntityHandle), 1));
    new(handle) LuaEntityHandle();
    handle->entity = entity;
    luaL_setmetatable(lua, metatable);
  }

  LuaEntityHandle &check_entity(lua_State *lua, const int index)
  {
    return *static_cast<LuaEntityHandle *>(luaL_checkudata(lua, index, metatable));
  }

  LuaEntityHandle *test_entity(lua_State *lua, const int index)
  {
    return static_cast<LuaEntityHandle *>(luaL_testudata(lua, index, metatable));
  }
} // neon
