#include "lua-classes.hpp"

#include <algorithm>
#include <cstring>

#include "lua-api.hpp"
#include "lua-color-handle.hpp"
#include "lua-vec3-handle.hpp"

namespace neon
{
  // Helpers of the classes, for this file alone.
  namespace
  {
    constexpr const char *component_class = "neon.component-class";
    constexpr const char *system_class = "neon.system-class";

    /// The hooks a system's functions are called with, after `self`, the
    /// entity, and one component each: nothing, the seconds, the other
    /// entity, or the other entity with the point and the normal.
    constexpr int extra_parameters[] = {0, 1, 1, 0, 1, 1, 3};

    /// `Component:extend { fields }` or `Component:extend("Name", { fields })`.
    int component_extend(lua_State *lua)
    {
      // self is the Component table, which is argument 1
      int fields = 2;
      const char *name = nullptr;
      if (lua_type(lua, 2) == LUA_TSTRING)
      {
        name = lua_tostring(lua, 2);
        fields = 3;
      }
      luaL_checktype(lua, fields, LUA_TTABLE);

      lua_newtable(lua);
      lua_pushvalue(lua, fields);
      lua_setfield(lua, -2, "__fields");
      if (name != nullptr)
      {
        lua_pushstring(lua, name);
        lua_setfield(lua, -2, "name");
      }
      luaL_setmetatable(lua, component_class);
      return 1;
    }

    /// `System:extend("Door", ...)`, each a name or a component class.
    int system_extend(lua_State *lua)
    {
      const int count = lua_gettop(lua);
      if (count < 2) { return luaL_error(lua, "System:extend needs the components the system runs over"); }

      lua_newtable(lua);
      lua_newtable(lua);
      for (int i = 2; i <= count; i++)
      {
        // a class table is kept as it is: its name is known once the file
        // has returned it and the engine has declared it
        if (lua_type(lua, i) == LUA_TSTRING || is_component_class(lua, i)) { lua_pushvalue(lua, i); }
        else
        {
          return luaL_error(lua, "System:extend takes the names of components, or their classes, not %s", luaL_typename(lua, i));
        }
        lua_rawseti(lua, -2, i - 1);
      }
      lua_setfield(lua, -2, "__components");
      luaL_setmetatable(lua, system_class);
      return 1;
    }

    /// Reads the kind and the default of a field from the value at the top
    /// of the stack. Returns false when nothing a component holds is there.
    bool infer_field(lua_State *lua, ScriptField &field)
    {
      if (lua_isboolean(lua, -1))
      {
        field.kind = FieldKind::Boolean;
        field.standard = lua_toboolean(lua, -1) != 0;
        return true;
      }
      if (lua_isinteger(lua, -1))
      {
        field.kind = FieldKind::Integer;
        field.standard = static_cast<int>(lua_tointeger(lua, -1));
        return true;
      }
      if (lua_isnumber(lua, -1))
      {
        field.kind = FieldKind::Float;
        field.standard = static_cast<float>(lua_tonumber(lua, -1));
        return true;
      }
      if (lua_type(lua, -1) == LUA_TSTRING)
      {
        field.kind = FieldKind::String;
        field.standard = std::string(lua_tostring(lua, -1));
        return true;
      }
      if (test_vec3(lua, -1) != nullptr)
      {
        field.kind = FieldKind::Vector3;
        field.standard = check_vec3(lua, -1);
        return true;
      }
      if (test_color(lua, -1) != nullptr)
      {
        field.kind = FieldKind::Color;
        field.standard = check_color(lua, -1);
        return true;
      }
      return false;
    }

    /// How many parameters a Lua function takes, or -1 when it takes any
    /// number, or is not written in Lua.
    int parameters_of(lua_State *lua, const int index)
    {
      lua_Debug info;
      lua_pushvalue(lua, index);
      lua_getinfo(lua, ">Su", &info);
      if (info.isvararg != 0 || std::strcmp(info.what, "Lua") != 0) { return -1; }
      return static_cast<int>(info.nparams);
    }

    constexpr luaL_Reg component_functions[] = {{"extend", component_extend}, {nullptr, nullptr}};
    constexpr luaL_Reg system_functions[] = {{"extend", system_extend}, {nullptr, nullptr}};
  }

  void open_classes(lua_State *lua)
  {
    luaL_newmetatable(lua, component_class);
    lua_pop(lua, 1);
    luaL_newmetatable(lua, system_class);
    lua_pop(lua, 1);

    luaL_newlib(lua, component_functions);
    lua_setglobal(lua, "Component");
    luaL_newlib(lua, system_functions);
    lua_setglobal(lua, "System");
  }

  bool is_component_class(lua_State *lua, const int index)
  {
    if (!lua_istable(lua, index) || lua_getmetatable(lua, index) == 0) { return false; }
    luaL_getmetatable(lua, component_class);
    const bool same = lua_rawequal(lua, -1, -2) != 0;
    lua_pop(lua, 2);
    return same;
  }

  bool is_system_class(lua_State *lua, const int index)
  {
    if (!lua_istable(lua, index) || lua_getmetatable(lua, index) == 0) { return false; }
    luaL_getmetatable(lua, system_class);
    const bool same = lua_rawequal(lua, -1, -2) != 0;
    lua_pop(lua, 2);
    return same;
  }

  bool read_component_class(
    lua_State *lua,
    const int index,
    const std::string &name,
    LuaComponentDeclaration &declaration,
    std::string &problem)
  {
    const int top = lua_gettop(lua);
    const int table = lua_absindex(lua, index);

    lua_getfield(lua, table, "name");
    declaration.name = lua_type(lua, -1) == LUA_TSTRING ? lua_tostring(lua, -1) : name;
    lua_pop(lua, 1);

    lua_getfield(lua, table, "__fields");
    const int fields = lua_gettop(lua);

    lua_pushnil(lua);
    while (lua_next(lua, fields) != 0)
    {
      if (lua_type(lua, -2) != LUA_TSTRING)
      {
        problem = "The fields of " + declaration.name + " need names; a field is `name = default`";
        lua_settop(lua, top);
        return false;
      }

      ScriptField field;
      field.name = lua_tostring(lua, -2);

      if (field.name.starts_with('_'))
      {
        problem = "The field '" + field.name + "' of " + declaration.name
                  + " starts with an underscore, which is for private state a system keeps, not for a field";
        lua_settop(lua, top);
        return false;
      }
      if (lua_isfunction(lua, -1))
      {
        problem = "'" + field.name + "' of " + declaration.name + " is a function; behaviour belongs in a System over the component";
        lua_settop(lua, top);
        return false;
      }
      if (!infer_field(lua, field))
      {
        problem = "The default of '" + field.name + "' of " + declaration.name + " is " + luaL_typename(lua, -1)
                  + "; a field holds a number, a bool, text, a vec3, or a color";
        lua_settop(lua, top);
        return false;
      }

      declaration.fields.push_back(field);
      lua_pop(lua, 1);
    }

    // in the order of their names, so the layout is the same on every run
    std::ranges::sort(declaration.fields, {}, &ScriptField::name);

    lua_settop(lua, top);
    return true;
  }

  bool read_system_class(
    lua_State *lua,
    const int index,
    const std::string &name,
    LuaSystemDeclaration &declaration,
    std::string &problem)
  {
    const int top = lua_gettop(lua);
    const int table = lua_absindex(lua, index);

    lua_getfield(lua, table, "name");
    declaration.name = lua_type(lua, -1) == LUA_TSTRING ? lua_tostring(lua, -1) : name;
    lua_pop(lua, 1);

    lua_getfield(lua, table, "__components");
    const auto count = static_cast<lua_Integer>(lua_rawlen(lua, -1));
    for (lua_Integer i = 1; i <= count; i++)
    {
      lua_rawgeti(lua, -1, i);
      if (lua_istable(lua, -1))
      {
        lua_getfield(lua, -1, "name");
        if (lua_type(lua, -1) != LUA_TSTRING)
        {
          problem = declaration.name + " runs over a component class that was not declared; return the class from its file, before or with the system";
          lua_settop(lua, top);
          return false;
        }
        lua_remove(lua, -2);
      }
      declaration.components.emplace_back(lua_tostring(lua, -1));
      lua_pop(lua, 1);
    }
    lua_pop(lua, 1);

    std::string hooks;
    for (const auto hook : lua_hook_names)
    {
      if (!hooks.empty()) { hooks += ", "; }
      hooks += hook;
    }

    lua_pushnil(lua);
    while (lua_next(lua, table) != 0)
    {
      if (lua_type(lua, -2) != LUA_TSTRING)
      {
        problem = declaration.name + " has a key that is not a name; a system holds its hooks and nothing else";
        lua_settop(lua, top);
        return false;
      }

      const std::string key = lua_tostring(lua, -2);
      if (key == "__components" || key == "name")
      {
        lua_pop(lua, 1);
        continue;
      }

      const auto found = std::ranges::find(lua_hook_names, key);
      if (found == lua_hook_names.end())
      {
        problem = declaration.name + " defines '" + key + "', which is not a hook of System. The hooks are " + hooks;
        lua_settop(lua, top);
        return false;
      }
      if (!lua_isfunction(lua, -1))
      {
        problem = "'" + key + "' of " + declaration.name + " is " + luaL_typename(lua, -1) + ", not a function";
        lua_settop(lua, top);
        return false;
      }

      const auto hook = static_cast<std::size_t>(found - lua_hook_names.begin());
      const int parameters = parameters_of(lua, -1);
      const int components = hook == static_cast<std::size_t>(LuaHook::Removed) ? 0 : static_cast<int>(declaration.components.size());
      const int expected = 2 + components + extra_parameters[hook];
      if (parameters != -1 && parameters != expected)
      {
        problem = "'" + key + "' of " + declaration.name + " takes " + std::to_string(parameters) + " parameters; it is called with "
                  + std::to_string(expected) + ": self, the entity"
                  + (components > 0 ? ", one for each of its " + std::to_string(components) + " components" : "")
                  + (extra_parameters[hook] == 1 && hook <= static_cast<std::size_t>(LuaHook::Step) ? ", and the seconds" : "")
                  + (hook == static_cast<std::size_t>(LuaHook::TriggerEnter) || hook == static_cast<std::size_t>(LuaHook::TriggerExit)
                       ? ", and the other entity"
                       : "")
                  + (hook == static_cast<std::size_t>(LuaHook::Collision) ? ", the other entity, the point, and the normal" : "");
        lua_settop(lua, top);
        return false;
      }

      declaration.hooks[hook] = true;
      lua_pop(lua, 1);
    }

    lua_pushvalue(lua, table);
    declaration.ref = luaL_ref(lua, LUA_REGISTRYINDEX);

    lua_settop(lua, top);
    return true;
  }
} // neon
