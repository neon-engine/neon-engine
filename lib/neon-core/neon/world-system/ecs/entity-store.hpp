#ifndef ENTITY_STORE_HPP
#define ENTITY_STORE_HPP

#include <functional>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "component-info.hpp"
#include "entity-query.hpp"
#include "entity.hpp"

namespace neon
{
  /// Keeps the entities of the world and their components.
  ///
  /// This is the whole of what the engine, a game, and a script see of the
  /// entity component system. The library that does the work sits behind it
  /// in a backend of its own.
  ///
  /// The functions that take a ComponentId work on raw memory and are what a
  /// backend implements. The templates below them do the same for a C++ type
  /// and are what engine code uses.
  ///
  /// Changes that are made while a query hands over its entities take effect
  /// when the query is done: creating and destroying entities, and setting
  /// and removing components. Writing to the components a block hands over
  /// takes effect at once.
  class EntityStore
  {
    std::unordered_map<std::type_index, ComponentId> _component_ids;

  protected:
    ~EntityStore() = default;

  public:
    virtual void Initialize() = 0;

    /// Removes every entity. Components are told through on_remove.
    virtual void CleanUp() = 0;

    /// Makes a kind of component known. Returns the same id when the name is
    /// registered again.
    virtual ComponentId RegisterComponent(const ComponentInfo &info) = 0;

    /// The component registered under this name, or No_Component.
    virtual ComponentId FindComponent(const std::string &name) = 0;

    /// Creates an entity. A name has to be unique among the children of the
    /// parent, and may be empty. When an entity of that name exists under the
    /// parent already, it is returned instead.
    virtual Entity CreateEntity(const std::string &name, Entity parent) = 0;

    /// Destroys an entity together with its children.
    virtual void DestroyEntity(Entity entity) = 0;

    virtual bool IsAlive(Entity entity) = 0;

    /// Finds an entity by its names from the top, separated by forward
    /// slashes, such as `player/camera`. Returns No_Entity when there is none.
    virtual Entity FindEntity(const std::string &path) = 0;

    /// The name of the entity itself, without those of its parents.
    virtual std::string GetName(Entity entity) = 0;

    /// Moves an entity under another. No_Entity moves it to the top.
    virtual void SetParent(Entity entity, Entity parent) = 0;

    virtual Entity GetParent(Entity entity) = 0;

    /// The children of an entity, in the order they were created. No_Entity
    /// gives the entities at the top.
    virtual std::vector<Entity> GetChildren(Entity entity) = 0;

    /// Gives the entity the component, or replaces the one it has. `value`
    /// points to a component that is copied.
    virtual void SetComponent(Entity entity, ComponentId component, const void *value) = 0;

    /// The component of the entity, or nullptr when it has none. The pointer
    /// is valid until the entity gains or loses a component.
    virtual void *GetComponent(Entity entity, ComponentId component) = 0;

    virtual bool HasComponent(Entity entity, ComponentId component) = 0;

    virtual void RemoveComponent(Entity entity, ComponentId component) = 0;

    /// Prepares a query. Create it once and keep the id.
    virtual QueryId CreateQuery(const QueryInfo &info) = 0;

    /// Hands over every entity that matches the query, block by block.
    virtual void Each(QueryId query, const std::function<void(const EntityBlock &)> &visit) = 0;

    Entity CreateEntity(const std::string &name)
    {
      return CreateEntity(name, No_Entity);
    }

    /// Makes a C++ type known as a component.
    template<typename T>
    ComponentId Register(
      const std::string &name,
      const std::function<void(Entity, T &)> &on_remove = {})
    {
      auto info = ComponentInfo::Of<T>(name);
      if (on_remove)
      {
        info.on_remove = [on_remove](const Entity entity, void *component)
        {
          on_remove(entity, *static_cast<T *>(component));
        };
      }

      const auto id = RegisterComponent(info);
      _component_ids[std::type_index(typeid(T))] = id;
      return id;
    }

    /// The id a C++ type was registered under.
    template<typename T>
    [[nodiscard]] ComponentId IdOf() const
    {
      const auto it = _component_ids.find(std::type_index(typeid(T)));
      if (it == _component_ids.end())
      {
        throw std::runtime_error("A component was used before it was registered");
      }
      return it->second;
    }

    template<typename T>
    void Set(const Entity entity, const T &value)
    {
      SetComponent(entity, IdOf<T>(), &value);
    }

    template<typename T>
    [[nodiscard]] T *Get(const Entity entity)
    {
      return static_cast<T *>(GetComponent(entity, IdOf<T>()));
    }

    template<typename T>
    [[nodiscard]] bool Has(const Entity entity)
    {
      return HasComponent(entity, IdOf<T>());
    }

    template<typename T>
    void Remove(const Entity entity)
    {
      RemoveComponent(entity, IdOf<T>());
    }

    /// Prepares a query for entities that carry all of the given types.
    template<typename... Components>
    QueryId Query(const QueryOrder order = QueryOrder::Any)
    {
      return CreateQuery({.components = {IdOf<Components>()...}, .order = order});
    }
  };
} // neon

#endif //ENTITY_STORE_HPP
