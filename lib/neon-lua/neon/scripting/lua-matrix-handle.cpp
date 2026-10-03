#include "lua-matrix-handle.hpp"

#include <string>

#include "lua-api.hpp"
#include "lua-host.hpp"
#include "lua-vec3-handle.hpp"
#include "lua-vec4-handle.hpp"

namespace neon
{
  // Helpers of the matrix handles, for this file alone.
  namespace
  {
    constexpr const char *metatable = "neon.matrix";

    LuaMatrixHandle *new_handle(lua_State *lua, const int size)
    {
      auto *handle = static_cast<LuaMatrixHandle *>(lua_newuserdatauv(lua, sizeof(LuaMatrixHandle), 0));
      new(handle) LuaMatrixHandle();
      handle->size = size;
      luaL_setmetatable(lua, metatable);
      return handle;
    }

    /// The field's value as a 4 by 4, whichever the field holds.
    glm::mat4 as_mat4(const FieldValue &value)
    {
      if (const auto *three = std::get_if<glm::mat3>(&value)) { return glm::mat4(*three); }
      if (const auto *four = std::get_if<glm::mat4>(&value)) { return *four; }
      return glm::mat4{1.0f};
    }

    void write_back(lua_State *lua, const LuaMatrixHandle &handle)
    {
      if (!handle.IsBound()) { return; }

      EntityStore *store = host_of(lua).store;
      void *object = store != nullptr ? handle.target.Resolve(*store) : nullptr;
      if (object == nullptr)
      {
        luaL_error(lua, "The entity no longer has the component this matrix belongs to");
        return;
      }
      if (handle.size == 3) { handle.field->set(object, glm::mat3(handle.value)); }
      else { handle.field->set(object, handle.value); }
    }

    /// A row and a column of the handle's size, counted from 1, or a Lua
    /// error.
    void check_cell(lua_State *lua, const LuaMatrixHandle &handle, const int row_index, const int column_index, int &row, int &column)
    {
      row = static_cast<int>(luaL_checkinteger(lua, row_index));
      column = static_cast<int>(luaL_checkinteger(lua, column_index));
      if (row < 1 || row > handle.size || column < 1 || column > handle.size)
      {
        luaL_error(lua, "A mat%d has rows and columns 1 to %d, not %d, %d", handle.size, handle.size, row, column);
      }
    }

    int matrix_get(lua_State *lua)
    {
      LuaMatrixHandle &handle = check_matrix(lua, 1);
      int row = 0;
      int column = 0;
      check_cell(lua, handle, 2, 3, row, column);
      // glm keeps columns, so a cell at a row and a column is [column][row]
      lua_pushnumber(lua, matrix_value(lua, handle)[column - 1][row - 1]);
      return 1;
    }

    int matrix_set(lua_State *lua)
    {
      LuaMatrixHandle &handle = check_matrix(lua, 1);
      int row = 0;
      int column = 0;
      check_cell(lua, handle, 2, 3, row, column);
      const auto number = static_cast<float>(luaL_checknumber(lua, 4));

      handle.value = matrix_value(lua, handle);
      handle.value[column - 1][row - 1] = number;
      write_back(lua, handle);
      return 0;
    }

    /// A row as a vector of the matrix's size.
    int matrix_row(lua_State *lua)
    {
      LuaMatrixHandle &handle = check_matrix(lua, 1);
      const auto row = static_cast<int>(luaL_checkinteger(lua, 2));
      if (row < 1 || row > handle.size) { return luaL_error(lua, "A mat%d has rows 1 to %d, not %d", handle.size, handle.size, row); }

      const glm::mat4 value = matrix_value(lua, handle);
      if (handle.size == 3) { push_vec3(lua, glm::vec3{value[0][row - 1], value[1][row - 1], value[2][row - 1]}); }
      else { push_vec4(lua, glm::vec4{value[0][row - 1], value[1][row - 1], value[2][row - 1], value[3][row - 1]}); }
      return 1;
    }

    /// `a * b` for two matrices of one size, `m * v` for a vector of it.
    int matrix_mul(lua_State *lua)
    {
      LuaMatrixHandle &left = check_matrix(lua, 1);
      const glm::mat4 value = matrix_value(lua, left);

      if (test_vec3(lua, 2) != nullptr && left.size == 3)
      {
        push_vec3(lua, glm::mat3(value) * check_vec3(lua, 2));
        return 1;
      }
      if (test_vec4(lua, 2) != nullptr && left.size == 4)
      {
        push_vec4(lua, value * check_vec4(lua, 2));
        return 1;
      }
      if (test_vec3(lua, 2) != nullptr && left.size == 4)
      {
        // a point, carried through a 4 by 4
        const glm::vec4 moved = value * glm::vec4(check_vec3(lua, 2), 1.0f);
        push_vec3(lua, glm::vec3(moved));
        return 1;
      }

      LuaMatrixHandle &right = check_matrix(lua, 2);
      if (right.size != left.size) { return luaL_error(lua, "A mat%d cannot be multiplied with a mat%d", left.size, right.size); }
      const glm::mat4 product = value * matrix_value(lua, right);
      if (left.size == 3) { push_mat3(lua, glm::mat3(product)); }
      else { push_mat4(lua, product); }
      return 1;
    }

    int matrix_eq(lua_State *lua)
    {
      LuaMatrixHandle &left = check_matrix(lua, 1);
      LuaMatrixHandle &right = check_matrix(lua, 2);
      lua_pushboolean(lua, left.size == right.size && matrix_value(lua, left) == matrix_value(lua, right) ? 1 : 0);
      return 1;
    }

    int matrix_tostring(lua_State *lua)
    {
      LuaMatrixHandle &handle = check_matrix(lua, 1);
      const glm::mat4 value = matrix_value(lua, handle);
      std::string text = "mat" + std::to_string(handle.size) + "(";
      for (int row = 0; row < handle.size; row++)
      {
        if (row > 0) { text += ", "; }
        for (int column = 0; column < handle.size; column++)
        {
          if (column > 0) { text += ' '; }
          text += std::to_string(value[column][row]);
        }
      }
      text += ")";
      lua_pushstring(lua, text.c_str());
      return 1;
    }

    int matrix_copy(lua_State *lua)
    {
      LuaMatrixHandle &handle = check_matrix(lua, 1);
      const glm::mat4 value = matrix_value(lua, handle);
      if (handle.size == 3) { push_mat3(lua, glm::mat3(value)); }
      else { push_mat4(lua, value); }
      return 1;
    }

    int matrix_index(lua_State *lua)
    {
      // only the methods; cells are reached with get and set
      luaL_getmetatable(lua, metatable);
      lua_pushvalue(lua, 2);
      lua_rawget(lua, -2);
      if (lua_isnil(lua, -1))
      {
        return luaL_error(lua, "A matrix has get, set, row, and copy; a cell is m:get(row, column)");
      }
      return 1;
    }

    int matrix_newindex(lua_State *lua)
    {
      return luaL_error(lua, "A cell of a matrix is set with m:set(row, column, number)");
    }

    /// `mat3()` or `mat4()` is the identity; with its rows as vectors it is
    /// those rows.
    int make(lua_State *lua, const int size)
    {
      const int count = lua_gettop(lua);
      glm::mat4 value{1.0f};
      if (count == size)
      {
        for (int row = 0; row < size; row++)
        {
          if (size == 3)
          {
            const glm::vec3 given = check_vec3(lua, row + 1);
            for (int column = 0; column < 3; column++) { value[column][row] = given[column]; }
          }
          else
          {
            const glm::vec4 given = check_vec4(lua, row + 1);
            for (int column = 0; column < 4; column++) { value[column][row] = given[column]; }
          }
        }
      }
      else if (count != 0) { return luaL_error(lua, "mat%d takes %d rows as vectors, or nothing for the identity", size, size); }

      if (size == 3) { push_mat3(lua, glm::mat3(value)); }
      else { push_mat4(lua, value); }
      return 1;
    }

    int mat3_new(lua_State *lua)
    {
      return make(lua, 3);
    }

    int mat4_new(lua_State *lua)
    {
      return make(lua, 4);
    }

    constexpr luaL_Reg methods[] = {
      {"__index", matrix_index},
      {"__newindex", matrix_newindex},
      {"__mul", matrix_mul},
      {"__eq", matrix_eq},
      {"__tostring", matrix_tostring},
      {"get", matrix_get},
      {"set", matrix_set},
      {"row", matrix_row},
      {"copy", matrix_copy},
      {nullptr, nullptr}
    };
  }

  void open_matrix_handles(lua_State *lua)
  {
    if (luaL_newmetatable(lua, metatable) != 0) { luaL_setfuncs(lua, methods, 0); }
    lua_pop(lua, 1);

    lua_pushcfunction(lua, mat3_new);
    lua_setglobal(lua, "mat3");
    lua_pushcfunction(lua, mat4_new);
    lua_setglobal(lua, "mat4");
  }

  void push_mat3(lua_State *lua, const glm::mat3 &value)
  {
    new_handle(lua, 3)->value = glm::mat4(value);
  }

  void push_mat4(lua_State *lua, const glm::mat4 &value)
  {
    new_handle(lua, 4)->value = value;
  }

  void push_bound_mat3(lua_State *lua, const glm::mat3 &value, const LuaComponentHandle &target, const FieldInfo &field)
  {
    LuaMatrixHandle *handle = new_handle(lua, 3);
    handle->value = glm::mat4(value);
    handle->target = target;
    handle->field = &field;
  }

  void push_bound_mat4(lua_State *lua, const glm::mat4 &value, const LuaComponentHandle &target, const FieldInfo &field)
  {
    LuaMatrixHandle *handle = new_handle(lua, 4);
    handle->value = value;
    handle->target = target;
    handle->field = &field;
  }

  LuaMatrixHandle &check_matrix(lua_State *lua, const int index)
  {
    return *static_cast<LuaMatrixHandle *>(luaL_checkudata(lua, index, metatable));
  }

  LuaMatrixHandle *test_matrix(lua_State *lua, const int index)
  {
    return static_cast<LuaMatrixHandle *>(luaL_testudata(lua, index, metatable));
  }

  glm::mat4 matrix_value(lua_State *lua, LuaMatrixHandle &handle)
  {
    if (handle.IsBound())
    {
      if (EntityStore *store = host_of(lua).store)
      {
        if (void *object = handle.target.Resolve(*store)) { handle.value = as_mat4(handle.field->get(object)); }
      }
    }
    return handle.value;
  }
} // neon
