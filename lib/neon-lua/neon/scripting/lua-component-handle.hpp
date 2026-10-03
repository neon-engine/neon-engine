#ifndef LUA_COMPONENT_HANDLE_HPP
#define LUA_COMPONENT_HANDLE_HPP

#include <neon/reflection/type-info.hpp>
#include <neon/world-system/ecs/entity.hpp>
#include <neon/world-system/ecs/entity-store.hpp>

struct lua_State;

namespace neon
{
  /// What a script holds of one component of one entity. A field is read
  /// and changed in place, through the description of the type, so nothing
  /// is copied between the store and the script.
  ///
  /// While a hook runs, `pointer` is the component in the block the query
  /// handed over, which is the fast way. After the hook it is null, and the
  /// handle asks the store for the component again, so that a handle a
  /// script kept still works, and says when the entity lost the component.
  struct LuaComponentHandle
  {
    Entity entity = No_Entity;
    ComponentId component = No_Component;

    /// The description, or null for a component that has none, which a
    /// script can then only see as being there.
    const TypeInfo *type = nullptr;

    void *pointer = nullptr;

    /// The component, or null when the entity no longer has it.
    [[nodiscard]] void *Resolve(EntityStore &store) const
    {
      return pointer != nullptr ? pointer : store.GetComponent(entity, component);
    }
  };

  /// Adds the metatable of components to the state, once.
  void open_component_handles(lua_State *lua);

  /// Pushes a handle of the component of the entity.
  void push_component(lua_State *lua, const LuaComponentHandle &handle);

  /// The handle at the index, or a Lua error when there is none.
  [[nodiscard]] LuaComponentHandle &check_component(lua_State *lua, int index);

  /// The handle at the index, or nullptr when the value is none.
  [[nodiscard]] LuaComponentHandle *test_component(lua_State *lua, int index);

  /// Pushes the value of a field of a component as a script sees it: a
  /// number, a bool, text, a vector or colour that writes back, or a list
  /// reached in place. `owner` is the stack index of the component handle
  /// the field is read from, whose cache keeps the handle so that a read
  /// every frame makes nothing new; 0 for none.
  void push_field(lua_State *lua, const LuaComponentHandle &handle, const FieldInfo &field, void *object, int owner);

  /// Reads the value at the index into a field of a component, or raises a
  /// Lua error that says what the field holds instead.
  void set_field(lua_State *lua, int index, const FieldInfo &field, void *object, const char *owner);

  /// Forgets the private fields, those whose names start with an
  /// underscore, that scripts kept for an entity. Called when the entity
  /// is gone.
  void forget_private_fields(lua_State *lua, Entity entity);
} // neon

#endif //LUA_COMPONENT_HANDLE_HPP
