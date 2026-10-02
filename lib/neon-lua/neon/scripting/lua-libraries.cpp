#include "lua-libraries.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "lua-api.hpp"
#include "lua-component-handle.hpp"
#include "lua-entity-handle.hpp"
#include "lua-host.hpp"

namespace neon
{
  // Helpers of the libraries, for this file alone.
  namespace
  {
    EntityStore &store_of(lua_State *lua)
    {
      EntityStore *store = host_of(lua).store;
      if (store == nullptr) { luaL_error(lua, "The world is not running"); }
      return *store;
    }

    /// Pushes the handle of a component of an entity, by its id and name.
    void push_component_handle(lua_State *lua, const Entity entity, const ComponentId id, const char *name)
    {
      LuaComponentHandle handle;
      handle.entity = entity;
      handle.component = id;
      const LuaHost &host = host_of(lua);
      const ComponentFormat *format = host.formats != nullptr ? host.formats->Find(name) : nullptr;
      handle.type = format != nullptr ? format->type.get() : nullptr;
      push_component(lua, handle);
    }

    /// The step of `world.each`: the next entity of the snapshot that still
    /// carries every component, with its components, or nothing.
    int each_step(lua_State *lua)
    {
      // upvalues: the entities, the names, and the index of the next one
      EntityStore &store = store_of(lua);
      const auto count = static_cast<lua_Integer>(lua_rawlen(lua, lua_upvalueindex(1)));
      const auto names = static_cast<int>(lua_rawlen(lua, lua_upvalueindex(2)));

      std::vector<ComponentId> ids;
      for (int i = 1; i <= names; i++)
      {
        lua_rawgeti(lua, lua_upvalueindex(2), i);
        ids.push_back(store.FindComponent(lua_tostring(lua, -1)));
        lua_pop(lua, 1);
      }

      for (auto index = lua_tointeger(lua, lua_upvalueindex(3)); index <= count; index++)
      {
        lua_rawgeti(lua, lua_upvalueindex(1), index);
        const auto entity = static_cast<Entity>(lua_tointeger(lua, -1));
        lua_pop(lua, 1);

        lua_pushinteger(lua, index + 1);
        lua_replace(lua, lua_upvalueindex(3));

        if (!store.IsAlive(entity)) { continue; }
        if (!std::ranges::all_of(ids, [&](const ComponentId id) { return store.HasComponent(entity, id); })) { continue; }

        push_entity(lua, entity);
        for (int i = 1; i <= names; i++)
        {
          lua_rawgeti(lua, lua_upvalueindex(2), i);
          const char *name = lua_tostring(lua, -1);
          lua_pop(lua, 1);
          push_component_handle(lua, entity, ids[static_cast<std::size_t>(i - 1)], name);
        }
        return 1 + names;
      }

      return 0;
    }

    /// `for entity, a, b in world.each("A", "B") do`: the entities that
    /// carry every named component. The entities are taken at the start,
    /// so one made or destroyed inside the loop is seen the next time.
    int world_each(lua_State *lua)
    {
      const int count = lua_gettop(lua);
      if (count == 0) { return luaL_error(lua, "world.each needs the names of the components to look for"); }

      EntityStore &store = store_of(lua);
      LuaHost &host = host_of(lua);

      std::string key;
      std::vector<ComponentId> ids;
      for (int i = 1; i <= count; i++)
      {
        const char *name = luaL_checkstring(lua, i);
        const ComponentId id = store.FindComponent(name);
        if (id == No_Component) { return luaL_error(lua, "There is no component called '%s'", name); }
        ids.push_back(id);
        key += name;
        key += ' ';
      }

      // one query per set of names, kept
      auto found = host.queries.find(key);
      if (found == host.queries.end())
      {
        found = host.queries.emplace(key, store.CreateQuery(QueryInfo{.components = ids})).first;
      }

      lua_newtable(lua);
      lua_Integer index = 1;
      store.Each(found->second, [&](const EntityBlock &block)
      {
        for (std::size_t i = 0; i < block.count; i++)
        {
          lua_pushinteger(lua, static_cast<lua_Integer>(block.entities[i]));
          lua_rawseti(lua, -2, index++);
        }
      });

      lua_createtable(lua, count, 0);
      for (int i = 1; i <= count; i++)
      {
        lua_pushvalue(lua, i);
        lua_rawseti(lua, -2, i);
      }

      lua_pushinteger(lua, 1);
      lua_pushcclosure(lua, each_step, 3);
      return 1;
    }

    int world_find(lua_State *lua)
    {
      const Entity entity = store_of(lua).FindEntity(luaL_checkstring(lua, 1));
      if (entity == No_Entity) { lua_pushnil(lua); }
      else { push_entity(lua, entity); }
      return 1;
    }

    int world_create(lua_State *lua)
    {
      const char *name = luaL_optstring(lua, 1, "");
      const LuaEntityHandle *parent = lua_isnoneornil(lua, 2) ? nullptr : &check_entity(lua, 2);
      push_entity(lua, store_of(lua).CreateEntity(name, parent != nullptr ? parent->entity : No_Entity));
      return 1;
    }

    int world_destroy(lua_State *lua)
    {
      store_of(lua).DestroyEntity(check_entity(lua, 1).entity);
      return 0;
    }

    constexpr luaL_Reg world_functions[] = {
      {"each", world_each},
      {"find", world_find},
      {"create", world_create},
      {"destroy", world_destroy},
      {nullptr, nullptr}
    };

    InputContext *input_of(lua_State *lua)
    {
      return host_of(lua).input;
    }

    int input_is_down(lua_State *lua)
    {
      InputContext *input = input_of(lua);
      lua_pushboolean(lua, input != nullptr && input->IsActionDown(luaL_checkstring(lua, 1)) ? 1 : 0);
      return 1;
    }

    int input_pressed(lua_State *lua)
    {
      InputContext *input = input_of(lua);
      lua_pushboolean(lua, input != nullptr && input->WasActionPressed(luaL_checkstring(lua, 1)) ? 1 : 0);
      return 1;
    }

    int input_amount(lua_State *lua)
    {
      InputContext *input = input_of(lua);
      lua_pushnumber(lua, input != nullptr ? input->ActionAmount(luaL_checkstring(lua, 1)) : 0.0);
      return 1;
    }

    int input_axis(lua_State *lua)
    {
      InputContext *input = input_of(lua);
      const glm::vec2 axis = input != nullptr ? input->ActionAxis(luaL_checkstring(lua, 1)) : glm::vec2{0.0f};
      lua_pushnumber(lua, axis.x);
      lua_pushnumber(lua, axis.y);
      return 2;
    }

    int input_axis3(lua_State *lua)
    {
      InputContext *input = input_of(lua);
      const glm::vec3 axis = input != nullptr ? input->ActionAxis3(luaL_checkstring(lua, 1)) : glm::vec3{0.0f};
      lua_pushnumber(lua, axis.x);
      lua_pushnumber(lua, axis.y);
      lua_pushnumber(lua, axis.z);
      return 3;
    }

    constexpr luaL_Reg input_functions[] = {
      {"is_down", input_is_down},
      {"pressed", input_pressed},
      {"amount", input_amount},
      {"axis", input_axis},
      {"axis3", input_axis3},
      {nullptr, nullptr}
    };

    /// Every argument as text, with a space between, as print does.
    std::string line_of(lua_State *lua)
    {
      std::string line;
      const int count = lua_gettop(lua);
      for (int i = 1; i <= count; i++)
      {
        if (i > 1) { line += ' '; }
        std::size_t length = 0;
        const char *text = luaL_tolstring(lua, i, &length);
        line.append(text, length);
        lua_pop(lua, 1);
      }
      return line;
    }

    int log_debug(lua_State *lua)
    {
      const std::string line = line_of(lua);
      host_of(lua).logger->Debug("{}", line);
      return 0;
    }

    int log_info(lua_State *lua)
    {
      const std::string line = line_of(lua);
      host_of(lua).logger->Info("{}", line);
      return 0;
    }

    int log_warn(lua_State *lua)
    {
      const std::string line = line_of(lua);
      host_of(lua).logger->Warn("{}", line);
      return 0;
    }

    int log_error(lua_State *lua)
    {
      const std::string line = line_of(lua);
      host_of(lua).logger->Error("{}", line);
      return 0;
    }

    constexpr luaL_Reg log_functions[] = {
      {"debug", log_debug},
      {"info", log_info},
      {"warn", log_warn},
      {"error", log_error},
      {nullptr, nullptr}
    };

    int scene_load(lua_State *lua)
    {
      WorldSystem *world = host_of(lua).world;
      if (world == nullptr) { return luaL_error(lua, "There is no world to change the scene of"); }
      world->LoadScene(luaL_checkstring(lua, 1));
      return 0;
    }

    constexpr luaL_Reg scene_functions[] = {{"load", scene_load}, {nullptr, nullptr}};

    /// `math.move_toward(from, to, by)`: `from` moved by at most `by`
    /// towards `to`, and `to` itself when that is nearer.
    int math_move_toward(lua_State *lua)
    {
      const lua_Number from = luaL_checknumber(lua, 1);
      const lua_Number to = luaL_checknumber(lua, 2);
      const lua_Number by = luaL_checknumber(lua, 3);
      if (std::abs(to - from) <= by) { lua_pushnumber(lua, to); }
      else { lua_pushnumber(lua, from + (to > from ? by : -by)); }
      return 1;
    }

    int math_clamp(lua_State *lua)
    {
      const lua_Number value = luaL_checknumber(lua, 1);
      const lua_Number low = luaL_checknumber(lua, 2);
      const lua_Number high = luaL_checknumber(lua, 3);
      lua_pushnumber(lua, std::clamp(value, low, high));
      return 1;
    }
  }

  void open_world_library(lua_State *lua)
  {
    luaL_newlib(lua, world_functions);
    lua_setglobal(lua, "world");
  }

  void open_input_library(lua_State *lua)
  {
    luaL_newlib(lua, input_functions);
    lua_setglobal(lua, "input");
  }

  void open_log_library(lua_State *lua)
  {
    luaL_newlib(lua, log_functions);
    lua_setglobal(lua, "log");

    lua_pushcfunction(lua, log_info);
    lua_setglobal(lua, "print");
  }

  void open_scene_library(lua_State *lua)
  {
    luaL_newlib(lua, scene_functions);
    lua_setglobal(lua, "scene");
  }

  void open_math_extras(lua_State *lua)
  {
    lua_getglobal(lua, LUA_MATHLIBNAME);
    lua_pushcfunction(lua, math_move_toward);
    lua_setfield(lua, -2, "move_toward");
    lua_pushcfunction(lua, math_clamp);
    lua_setfield(lua, -2, "clamp");
    lua_pop(lua, 1);
  }
} // neon
