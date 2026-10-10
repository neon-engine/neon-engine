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

    int scene_load(lua_State *lua);

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
        const auto query = store.CreateQuery(QueryInfo{.components = ids});
        if (query == neon::No_Query) { return luaL_error(lua, "The query over %s could not be made, see the log", key.c_str()); }
        found = host.queries.emplace(key, query).first;
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
      {"find_entity", world_find},
      {"create_entity", world_create},
      {"destroy_entity", world_destroy},
      {"load_scene", scene_load},
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
      lua_pushnumber(lua, input != nullptr ? input->ActionAxis(luaL_checkstring(lua, 1)) : 0.0);
      return 1;
    }

    int input_axis(lua_State *lua)
    {
      InputContext *input = input_of(lua);
      const glm::vec2 axis = input != nullptr ? input->ActionAxis2(luaL_checkstring(lua, 1)) : glm::vec2{0.0f};
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
      {"is_action_down", input_is_down},
      {"was_action_pressed", input_pressed},
      {"action_axis", input_amount},
      {"action_axis2", input_axis},
      {"action_axis3", input_axis3},
      {nullptr, nullptr}
    };

    UiContext *ui_of(lua_State *lua)
    {
      return host_of(lua).ui;
    }

    int ui_set_text(lua_State *lua)
    {
      UiContext *ui = ui_of(lua);
      const char *name = luaL_checkstring(lua, 1);
      const char *text = luaL_checkstring(lua, 2);
      if (ui != nullptr) { ui->SetText(name, text); }
      return 0;
    }

    int ui_set_number(lua_State *lua)
    {
      UiContext *ui = ui_of(lua);
      const char *name = luaL_checkstring(lua, 1);
      const lua_Number number = luaL_checknumber(lua, 2);
      if (ui != nullptr) { ui->SetNumber(name, number); }
      return 0;
    }

    int ui_set_flag(lua_State *lua)
    {
      UiContext *ui = ui_of(lua);
      const char *name = luaL_checkstring(lua, 1);
      luaL_checktype(lua, 2, LUA_TBOOLEAN);
      if (ui != nullptr) { ui->SetFlag(name, lua_toboolean(lua, 2) != 0); }
      return 0;
    }

    int ui_set_text_of(lua_State *lua)
    {
      UiContext *ui = ui_of(lua);
      const char *interface = luaL_checkstring(lua, 1);
      const char *name = luaL_checkstring(lua, 2);
      const char *text = luaL_checkstring(lua, 3);
      if (ui != nullptr) { ui->SetTextOf(interface, name, text); }
      return 0;
    }

    int ui_set_number_of(lua_State *lua)
    {
      UiContext *ui = ui_of(lua);
      const char *interface = luaL_checkstring(lua, 1);
      const char *name = luaL_checkstring(lua, 2);
      const lua_Number number = luaL_checknumber(lua, 3);
      if (ui != nullptr) { ui->SetNumberOf(interface, name, number); }
      return 0;
    }

    int ui_set_flag_of(lua_State *lua)
    {
      UiContext *ui = ui_of(lua);
      const char *interface = luaL_checkstring(lua, 1);
      const char *name = luaL_checkstring(lua, 2);
      luaL_checktype(lua, 3, LUA_TBOOLEAN);
      if (ui != nullptr) { ui->SetFlagOf(interface, name, lua_toboolean(lua, 3) != 0); }
      return 0;
    }

    constexpr luaL_Reg ui_functions[] = {
      {"set_text", ui_set_text},
      {"set_number", ui_set_number},
      {"set_flag", ui_set_flag},
      {"set_text_of", ui_set_text_of},
      {"set_number_of", ui_set_number_of},
      {"set_flag_of", ui_set_flag_of},
      {nullptr, nullptr}
    };

    SettingsStore *settings_of(lua_State *lua)
    {
      return host_of(lua).settings;
    }

    /// The setting of that name, or a Lua error that lists the names.
    const SettingDeclaration &setting_of(lua_State *lua, SettingsStore &settings, const char *name)
    {
      const SettingDeclaration *declaration = settings.Find(name);
      if (declaration == nullptr)
      {
        std::string names;
        for (const std::string &each : settings.Names()) { names += (names.empty() ? "" : ", ") + each; }
        luaL_error(lua, "There is no setting called '%s'. The settings are: %s", name, names.empty() ? "none" : names.c_str());
      }
      return *declaration;
    }

    /// Pushes a value of a setting as the Lua value it is.
    void push_setting_value(lua_State *lua, const DataValue &value)
    {
      bool flag = false;
      double number = 0.0;
      std::string text;
      if (value.GetBool(flag)) { lua_pushboolean(lua, flag ? 1 : 0); }
      else if (value.GetNumber(number)) { lua_pushnumber(lua, number); }
      else if (value.GetText(text)) { lua_pushlstring(lua, text.data(), text.size()); }
      else { lua_pushnil(lua); }
    }

    int settings_get(lua_State *lua)
    {
      const char *name = luaL_checkstring(lua, 1);
      SettingsStore *settings = settings_of(lua);
      if (settings == nullptr)
      {
        lua_pushnil(lua);
        return 1;
      }

      (void) setting_of(lua, *settings, name);
      push_setting_value(lua, *settings->Get(name));
      return 1;
    }

    int settings_set(lua_State *lua)
    {
      const char *name = luaL_checkstring(lua, 1);
      luaL_checkany(lua, 2);
      SettingsStore *settings = settings_of(lua);
      if (settings == nullptr)
      {
        lua_pushboolean(lua, 0);
        return 1;
      }

      (void) setting_of(lua, *settings, name);

      // a value as the Lua value it is; the store says what does not fit
      DataValue value;
      switch (lua_type(lua, 2))
      {
        case LUA_TBOOLEAN: value = DataValue::Bool(lua_toboolean(lua, 2) != 0);
          break;
        case LUA_TNUMBER: value = DataValue::Number(static_cast<double>(lua_tonumber(lua, 2)));
          break;
        case LUA_TSTRING: value = DataValue::Text(lua_tostring(lua, 2));
          break;
        default: return luaL_error(lua, "settings.set takes a boolean, a number, or a string, not %s", luaL_typename(lua, 2));
      }

      lua_pushboolean(lua, settings->Set(name, value) ? 1 : 0);
      return 1;
    }

    int settings_trigger(lua_State *lua)
    {
      const char *name = luaL_checkstring(lua, 1);
      SettingsStore *settings = settings_of(lua);
      if (settings == nullptr)
      {
        lua_pushboolean(lua, 0);
        return 1;
      }

      (void) setting_of(lua, *settings, name);
      lua_pushboolean(lua, settings->Trigger(name) ? 1 : 0);
      return 1;
    }

    int settings_on_change(lua_State *lua)
    {
      const char *name = luaL_checkstring(lua, 1);
      luaL_checktype(lua, 2, LUA_TFUNCTION);
      SettingsStore *settings = settings_of(lua);
      if (settings == nullptr) { return 0; }

      (void) setting_of(lua, *settings, name);

      // the function is kept in the registry for as long as the state lives
      lua_pushvalue(lua, 2);
      const int function = luaL_ref(lua, LUA_REGISTRYINDEX);
      LuaHost &host = host_of(lua);

      host.subscriptions.push_back(settings->OnChange(name, [lua, function, &host](const std::string &setting, const DataValue &value)
      {
        lua_rawgeti(lua, LUA_REGISTRYINDEX, function);
        push_setting_value(lua, value);
        if (lua_pcall(lua, 1, 0, 0) != LUA_OK)
        {
          const char *text = lua_tostring(lua, -1);
          const std::string message = text != nullptr ? text : "no message";
          host.logger->Error("The function settings.on_change was given for {} failed: {}", setting, message);
          lua_pop(lua, 1);
        }
      }));
      return 0;
    }

    constexpr luaL_Reg settings_functions[] = {
      {"get", settings_get},
      {"set", settings_set},
      {"trigger", settings_trigger},
      {"on_change", settings_on_change},
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

    constexpr luaL_Reg scene_functions[] = {{"load_scene", scene_load}, {nullptr, nullptr}};

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

  void open_ui_library(lua_State *lua)
  {
    luaL_newlib(lua, ui_functions);
    lua_setglobal(lua, "ui");
  }

  void open_settings_library(lua_State *lua)
  {
    luaL_newlib(lua, settings_functions);
    lua_setglobal(lua, "settings");
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
    // `scene.load_scene` is `world.load_scene`; the table stays for a
    // script that reads better with it
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
