#include "lua-component-handle.hpp"

#include <string>
#include <variant>

#include "lua-api.hpp"
#include "lua-color-handle.hpp"
#include "lua-host.hpp"
#include "lua-vec3-handle.hpp"

namespace neon
{
  // Helpers of the component handles, for this file alone.
  namespace
  {
    constexpr const char *metatable = "neon.component";

    // the private fields of every entity, in the registry: by entity, then
    // by component
    constexpr const char *privates_key = "neon.private";

    const char *name_of(const LuaComponentHandle &handle)
    {
      return handle.type != nullptr ? handle.type->name.c_str() : "The component";
    }

    /// The fields of a type, for a message.
    std::string fields_of(const TypeInfo &type)
    {
      std::string names;
      for (const std::string &path : type.GetPaths())
      {
        if (!names.empty()) { names += ", "; }
        names += path;
      }
      return names.empty() ? "none" : names;
    }

    /// The component of a handle, or a Lua error when the entity lost it.
    void *object_of(lua_State *lua, const LuaComponentHandle &handle)
    {
      EntityStore *store = host_of(lua).store;
      void *object = store != nullptr ? handle.Resolve(*store) : nullptr;
      if (object == nullptr)
      {
        luaL_error(lua, "%s is gone: the entity no longer has it", name_of(handle));
      }
      return object;
    }

    /// Pushes the table of private fields of the handle's component on its
    /// entity, making it when `create` says so, or nil.
    void push_private_table(lua_State *lua, const LuaComponentHandle &handle, const bool create)
    {
      lua_getfield(lua, LUA_REGISTRYINDEX, privates_key);
      if (lua_isnil(lua, -1))
      {
        lua_pop(lua, 1);
        if (!create) { return; }
        lua_newtable(lua);
        lua_pushvalue(lua, -1);
        lua_setfield(lua, LUA_REGISTRYINDEX, privates_key);
      }

      lua_rawgeti(lua, -1, static_cast<lua_Integer>(handle.entity));
      if (lua_isnil(lua, -1))
      {
        lua_pop(lua, 1);
        if (!create)
        {
          lua_pop(lua, 1);
          lua_pushnil(lua);
          return;
        }
        lua_newtable(lua);
        lua_pushvalue(lua, -1);
        lua_rawseti(lua, -3, static_cast<lua_Integer>(handle.entity));
      }
      lua_remove(lua, -2);

      lua_rawgeti(lua, -1, static_cast<lua_Integer>(handle.component));
      if (lua_isnil(lua, -1))
      {
        lua_pop(lua, 1);
        if (!create)
        {
          lua_pop(lua, 1);
          lua_pushnil(lua);
          return;
        }
        lua_newtable(lua);
        lua_pushvalue(lua, -1);
        lua_rawseti(lua, -3, static_cast<lua_Integer>(handle.component));
      }
      lua_remove(lua, -2);
    }

    int component_index(lua_State *lua)
    {
      const LuaComponentHandle &handle = check_component(lua, 1);
      const char *key = luaL_checkstring(lua, 2);

      if (key[0] == '_')
      {
        push_private_table(lua, handle, false);
        if (lua_isnil(lua, -1)) { return 1; }
        lua_getfield(lua, -1, key);
        return 1;
      }

      if (handle.type == nullptr)
      {
        return luaL_error(lua, "The component has no description, so a script cannot read its fields");
      }

      const FieldInfo *field = handle.type->Find(key);
      if (field == nullptr)
      {
        return luaL_error(lua, "%s has no field '%s'; the fields are %s", name_of(handle), key, fields_of(*handle.type).c_str());
      }

      push_field(lua, handle, *field, object_of(lua, handle));
      return 1;
    }

    int component_newindex(lua_State *lua)
    {
      const LuaComponentHandle &handle = check_component(lua, 1);
      const char *key = luaL_checkstring(lua, 2);

      if (key[0] == '_')
      {
        push_private_table(lua, handle, true);
        lua_pushvalue(lua, 3);
        lua_setfield(lua, -2, key);
        return 0;
      }

      if (handle.type == nullptr)
      {
        return luaL_error(lua, "The component has no description, so a script cannot change its fields");
      }

      const FieldInfo *field = handle.type->Find(key);
      if (field == nullptr)
      {
        return luaL_error(lua, "%s has no field '%s'; the fields are %s", name_of(handle), key, fields_of(*handle.type).c_str());
      }

      set_field(lua, 3, *field, object_of(lua, handle), name_of(handle));
      return 0;
    }

    int component_tostring(lua_State *lua)
    {
      const LuaComponentHandle &handle = check_component(lua, 1);
      lua_pushfstring(lua, "%s of entity %d", name_of(handle), static_cast<int>(handle.entity));
      return 1;
    }

    constexpr luaL_Reg methods[] = {
      {"__index", component_index},
      {"__newindex", component_newindex},
      {"__tostring", component_tostring},
      {nullptr, nullptr}
    };

    /// Pushes a list of texts or numbers as a table.
    template<typename T>
    void push_list(lua_State *lua, const std::vector<T> &items)
    {
      lua_createtable(lua, static_cast<int>(items.size()), 0);
      for (std::size_t i = 0; i < items.size(); i++)
      {
        if constexpr (std::is_same_v<T, std::string>) { lua_pushstring(lua, items[i].c_str()); }
        else { lua_pushnumber(lua, items[i]); }
        lua_rawseti(lua, -2, static_cast<lua_Integer>(i + 1));
      }
    }

    /// Reads a table of texts or numbers at the index.
    template<typename T>
    bool read_list(lua_State *lua, const int index, std::vector<T> &items)
    {
      if (!lua_istable(lua, index)) { return false; }
      const auto count = static_cast<lua_Integer>(lua_rawlen(lua, index));
      for (lua_Integer i = 1; i <= count; i++)
      {
        lua_rawgeti(lua, index, i);
        if constexpr (std::is_same_v<T, std::string>)
        {
          if (lua_type(lua, -1) != LUA_TSTRING)
          {
            lua_pop(lua, 1);
            return false;
          }
          items.emplace_back(lua_tostring(lua, -1));
        }
        else
        {
          if (!lua_isnumber(lua, -1))
          {
            lua_pop(lua, 1);
            return false;
          }
          items.push_back(static_cast<T>(lua_tonumber(lua, -1)));
        }
        lua_pop(lua, 1);
      }
      return true;
    }
  }

  void open_component_handles(lua_State *lua)
  {
    if (luaL_newmetatable(lua, metatable) != 0) { luaL_setfuncs(lua, methods, 0); }
    lua_pop(lua, 1);
  }

  void push_component(lua_State *lua, const LuaComponentHandle &handle)
  {
    auto *made = static_cast<LuaComponentHandle *>(lua_newuserdatauv(lua, sizeof(LuaComponentHandle), 0));
    new(made) LuaComponentHandle(handle);
    luaL_setmetatable(lua, metatable);
  }

  LuaComponentHandle &check_component(lua_State *lua, const int index)
  {
    return *static_cast<LuaComponentHandle *>(luaL_checkudata(lua, index, metatable));
  }

  LuaComponentHandle *test_component(lua_State *lua, const int index)
  {
    return static_cast<LuaComponentHandle *>(luaL_testudata(lua, index, metatable));
  }

  void push_field(lua_State *lua, const LuaComponentHandle &handle, const FieldInfo &field, void *object)
  {
    const FieldValue value = field.get(object);

    if (const auto *held = std::get_if<bool>(&value)) { lua_pushboolean(lua, *held ? 1 : 0); }
    else if (const auto *whole = std::get_if<int>(&value)) { lua_pushinteger(lua, *whole); }
    else if (const auto *number = std::get_if<float>(&value)) { lua_pushnumber(lua, *number); }
    else if (const auto *precise = std::get_if<double>(&value)) { lua_pushnumber(lua, *precise); }
    else if (const auto *text = std::get_if<std::string>(&value)) { lua_pushstring(lua, text->c_str()); }
    else if (const auto *vector = std::get_if<glm::vec3>(&value)) { push_bound_vec3(lua, *vector, handle, field); }
    else if (const auto *color = std::get_if<Color>(&value)) { push_bound_color(lua, *color, handle, field); }
    else if (const auto *texts = std::get_if<std::vector<std::string>>(&value)) { push_list(lua, *texts); }
    else if (const auto *numbers = std::get_if<std::vector<float>>(&value)) { push_list(lua, *numbers); }
    else if (const auto *length = std::get_if<FieldLength>(&value))
    {
      // a length as a style sheet writes one is three numbers to a script
      lua_createtable(lua, 0, 3);
      lua_pushboolean(lua, length->is_auto ? 1 : 0);
      lua_setfield(lua, -2, "auto");
      lua_pushnumber(lua, length->pixels);
      lua_setfield(lua, -2, "pixels");
      lua_pushnumber(lua, length->percent);
      lua_setfield(lua, -2, "percent");
    }
    else { lua_pushnil(lua); }
  }

  void set_field(lua_State *lua, const int index, const FieldInfo &field, void *object, const char *owner)
  {
    FieldValue value;
    bool fits = true;

    switch (field.kind)
    {
      case FieldKind::Boolean:
        fits = lua_isboolean(lua, index);
        if (fits) { value = lua_toboolean(lua, index) != 0; }
        break;
      case FieldKind::Integer:
        fits = lua_isinteger(lua, index) != 0;
        if (fits) { value = static_cast<int>(lua_tointeger(lua, index)); }
        break;
      case FieldKind::Float:
        fits = lua_isnumber(lua, index) != 0;
        if (fits) { value = static_cast<float>(lua_tonumber(lua, index)); }
        break;
      case FieldKind::Double:
        fits = lua_isnumber(lua, index) != 0;
        if (fits) { value = static_cast<double>(lua_tonumber(lua, index)); }
        break;
      case FieldKind::String:
      case FieldKind::Choice:
        fits = lua_type(lua, index) == LUA_TSTRING;
        if (fits) { value = std::string(lua_tostring(lua, index)); }
        break;
      case FieldKind::Vector3:
        fits = test_vec3(lua, index) != nullptr;
        if (fits) { value = check_vec3(lua, index); }
        break;
      case FieldKind::Color:
        fits = test_color(lua, index) != nullptr;
        if (fits) { value = check_color(lua, index); }
        break;
      case FieldKind::StringList:
      {
        std::vector<std::string> texts;
        fits = read_list(lua, index, texts);
        if (fits) { value = texts; }
        break;
      }
      case FieldKind::FloatList:
      case FieldKind::Layers:
      {
        std::vector<float> numbers;
        fits = read_list(lua, index, numbers);
        if (fits) { value = numbers; }
        break;
      }
      default:
        luaL_error(lua, "'%s' of %s holds %s, which a script cannot set", field.name.c_str(), owner, Describe(field.kind).c_str());
        return;
    }

    if (!fits)
    {
      luaL_error(lua, "'%s' of %s holds %s, not %s", field.name.c_str(), owner, Describe(field.kind).c_str(), luaL_typename(lua, index));
      return;
    }

    const std::string problem = field.Check(value, "'" + field.name + "' of " + owner);
    if (!problem.empty())
    {
      luaL_error(lua, "%s", problem.c_str());
      return;
    }

    field.set(object, value);
  }

  void forget_private_fields(lua_State *lua, const Entity entity)
  {
    lua_getfield(lua, LUA_REGISTRYINDEX, privates_key);
    if (!lua_isnil(lua, -1))
    {
      lua_pushnil(lua);
      lua_rawseti(lua, -2, static_cast<lua_Integer>(entity));
    }
    lua_pop(lua, 1);
  }
} // neon
