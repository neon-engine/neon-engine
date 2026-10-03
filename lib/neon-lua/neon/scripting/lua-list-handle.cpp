#include "lua-list-handle.hpp"

#include <cmath>
#include <string>
#include <vector>

#include "lua-api.hpp"
#include "lua-host.hpp"
#include "lua-vec3-handle.hpp"

namespace neon
{
  // Helpers of the list handles, for this file alone.
  namespace
  {
    constexpr const char *metatable = "neon.list";

    /// The vector of a handle, or a Lua error when the entity lost the
    /// component.
    void *vector_of(lua_State *lua, const LuaListHandle &handle)
    {
      EntityStore *store = host_of(lua).store;
      void *object = store != nullptr ? handle.target.Resolve(*store) : nullptr;
      if (object == nullptr)
      {
        luaL_error(lua, "The entity no longer has the component this list belongs to");
        return nullptr;
      }
      return handle.field->reach(object);
    }

    template<typename T>
    std::vector<T> &as(void *vector)
    {
      return *static_cast<std::vector<T> *>(vector);
    }

    /// Runs `with` on the vector of the handle, whichever element it holds.
    template<typename With>
    auto with_vector(lua_State *lua, const LuaListHandle &handle, With with)
    {
      void *vector = vector_of(lua, handle);
      switch (handle.field->kind)
      {
        case FieldKind::StringList: return with(as<std::string>(vector));
        case FieldKind::IntegerList: return with(as<int>(vector));
        case FieldKind::Vector3List: return with(as<glm::vec3>(vector));
        default: return with(as<float>(vector));
      }
    }

    std::size_t size_of(lua_State *lua, const LuaListHandle &handle)
    {
      return with_vector(lua, handle, [](auto &vector) { return vector.size(); });
    }

    /// An index from 1 to the size, or to the size plus one when `past` is
    /// allowed, as a Lua error otherwise.
    std::size_t check_index(lua_State *lua, const LuaListHandle &handle, const int at, const bool past)
    {
      const lua_Integer index = luaL_checkinteger(lua, at);
      const auto size = static_cast<lua_Integer>(size_of(lua, handle));
      if (index < 1 || index > size + (past ? 1 : 0))
      {
        luaL_error(lua, "The list has %d elements, so there is no element %d", static_cast<int>(size), static_cast<int>(index));
      }
      return static_cast<std::size_t>(index - 1);
    }

    void push_element(lua_State *lua, const LuaListHandle &handle, const std::size_t index)
    {
      void *vector = vector_of(lua, handle);
      switch (handle.field->kind)
      {
        case FieldKind::StringList: lua_pushstring(lua, as<std::string>(vector)[index].c_str());
          break;
        case FieldKind::IntegerList: lua_pushinteger(lua, as<int>(vector)[index]);
          break;
        case FieldKind::Vector3List: push_vec3(lua, as<glm::vec3>(vector)[index]);
          break;
        default: lua_pushnumber(lua, as<float>(vector)[index]);
          break;
      }
    }

    /// Reads the value at `at` as an element of the handle's list, or
    /// raises a Lua error.
    void read_element(lua_State *lua, const LuaListHandle &handle, const int at, std::string &text, int &whole, float &number, glm::vec3 &vector)
    {
      switch (handle.field->kind)
      {
        case FieldKind::StringList:
          if (lua_type(lua, at) != LUA_TSTRING) { luaL_error(lua, "An element of this list is text, not %s", luaL_typename(lua, at)); }
          text = lua_tostring(lua, at);
          break;
        case FieldKind::IntegerList:
          if (lua_isinteger(lua, at) == 0) { luaL_error(lua, "An element of this list is a whole number, not %s", luaL_typename(lua, at)); }
          whole = static_cast<int>(lua_tointeger(lua, at));
          break;
        case FieldKind::Vector3List:
          if (test_vec3(lua, at) == nullptr) { luaL_error(lua, "An element of this list is a vec3, not %s", luaL_typename(lua, at)); }
          vector = check_vec3(lua, at);
          break;
        default:
          if (lua_isnumber(lua, at) == 0) { luaL_error(lua, "An element of this list is a number, not %s", luaL_typename(lua, at)); }
          number = static_cast<float>(lua_tonumber(lua, at));
          break;
      }
    }

    void set_element(lua_State *lua, const LuaListHandle &handle, const std::size_t index, const int at)
    {
      std::string text;
      int whole = 0;
      float number = 0.0f;
      glm::vec3 vector{0.0f};
      read_element(lua, handle, at, text, whole, number, vector);

      void *held = vector_of(lua, handle);
      switch (handle.field->kind)
      {
        case FieldKind::StringList: as<std::string>(held)[index] = text;
          break;
        case FieldKind::IntegerList: as<int>(held)[index] = whole;
          break;
        case FieldKind::Vector3List: as<glm::vec3>(held)[index] = vector;
          break;
        default: as<float>(held)[index] = number;
          break;
      }
    }

    void insert_element(lua_State *lua, const LuaListHandle &handle, const std::size_t index, const int at)
    {
      std::string text;
      int whole = 0;
      float number = 0.0f;
      glm::vec3 vector{0.0f};
      read_element(lua, handle, at, text, whole, number, vector);

      void *held = vector_of(lua, handle);
      switch (handle.field->kind)
      {
        case FieldKind::StringList:
        {
          auto &list = as<std::string>(held);
          list.insert(list.begin() + static_cast<std::ptrdiff_t>(index), text);
          break;
        }
        case FieldKind::IntegerList:
        {
          auto &list = as<int>(held);
          list.insert(list.begin() + static_cast<std::ptrdiff_t>(index), whole);
          break;
        }
        case FieldKind::Vector3List:
        {
          auto &list = as<glm::vec3>(held);
          list.insert(list.begin() + static_cast<std::ptrdiff_t>(index), vector);
          break;
        }
        default:
        {
          auto &list = as<float>(held);
          list.insert(list.begin() + static_cast<std::ptrdiff_t>(index), number);
          break;
        }
      }
    }

    LuaListHandle &check_list(lua_State *lua, const int index)
    {
      return *static_cast<LuaListHandle *>(luaL_checkudata(lua, index, metatable));
    }

    int list_len(lua_State *lua)
    {
      lua_pushinteger(lua, static_cast<lua_Integer>(size_of(lua, check_list(lua, 1))));
      return 1;
    }

    int list_index(lua_State *lua)
    {
      const LuaListHandle &handle = check_list(lua, 1);

      if (lua_type(lua, 2) == LUA_TSTRING)
      {
        luaL_getmetatable(lua, metatable);
        lua_pushvalue(lua, 2);
        lua_rawget(lua, -2);
        if (lua_isnil(lua, -1)) { return luaL_error(lua, "A list has insert, remove, and clear, not '%s'", lua_tostring(lua, 2)); }
        return 1;
      }

      push_element(lua, handle, check_index(lua, handle, 2, false));
      return 1;
    }

    int list_newindex(lua_State *lua)
    {
      const LuaListHandle &handle = check_list(lua, 1);
      set_element(lua, handle, check_index(lua, handle, 2, false), 3);
      return 0;
    }

    /// `list:insert(value)` at the end, or `list:insert(i, value)` before
    /// the element i.
    int list_insert(lua_State *lua)
    {
      const LuaListHandle &handle = check_list(lua, 1);
      if (lua_gettop(lua) >= 3)
      {
        insert_element(lua, handle, check_index(lua, handle, 2, true), 3);
      }
      else
      {
        insert_element(lua, handle, size_of(lua, handle), 2);
      }
      return 0;
    }

    int list_remove(lua_State *lua)
    {
      const LuaListHandle &handle = check_list(lua, 1);
      const std::size_t index = check_index(lua, handle, 2, false);
      with_vector(lua, handle, [index](auto &vector)
      {
        vector.erase(vector.begin() + static_cast<std::ptrdiff_t>(index));
        return 0;
      });
      return 0;
    }

    int list_clear(lua_State *lua)
    {
      with_vector(lua, check_list(lua, 1), [](auto &vector)
      {
        vector.clear();
        return 0;
      });
      return 0;
    }

    int list_tostring(lua_State *lua)
    {
      const LuaListHandle &handle = check_list(lua, 1);
      lua_pushfstring(lua, "list '%s' of %d elements", handle.field->name.c_str(), static_cast<int>(size_of(lua, handle)));
      return 1;
    }

    constexpr luaL_Reg methods[] = {
      {"__len", list_len},
      {"__index", list_index},
      {"__newindex", list_newindex},
      {"__tostring", list_tostring},
      {"insert", list_insert},
      {"remove", list_remove},
      {"clear", list_clear},
      {nullptr, nullptr}
    };
  }

  void open_list_handles(lua_State *lua)
  {
    if (luaL_newmetatable(lua, metatable) != 0) { luaL_setfuncs(lua, methods, 0); }
    lua_pop(lua, 1);
  }

  void push_list(lua_State *lua, const LuaComponentHandle &target, const FieldInfo &field)
  {
    auto *handle = static_cast<LuaListHandle *>(lua_newuserdatauv(lua, sizeof(LuaListHandle), 0));
    new(handle) LuaListHandle();
    handle->target = target;
    handle->field = &field;
    luaL_setmetatable(lua, metatable);
  }

  LuaListHandle *test_list(lua_State *lua, const int index)
  {
    return static_cast<LuaListHandle *>(luaL_testudata(lua, index, metatable));
  }
} // neon
