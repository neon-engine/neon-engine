#ifndef FLECS_ENTITY_STORE_HPP
#define FLECS_ENTITY_STORE_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <neon/logging/logger.hpp>
#include <neon/world-system/ecs/entity-store.hpp>

// Declared here and not included, so that Flecs stays out of everything that
// includes this file.
// ReSharper disable CppInconsistentNaming
struct ecs_world_t;
struct ecs_query_t;
// ReSharper restore CppInconsistentNaming

namespace neon
{
  /// Keeps entities with Flecs.
  ///
  /// Flecs stores the entities that carry the same components next to each
  /// other, in a table of their own. A block that a query hands over is one
  /// such table. Giving a component to an entity or taking one away moves
  /// the entity to another table, which costs more than changing a value.
  // ReSharper disable once CppInconsistentNaming
  class Flecs_EntityStore final : public EntityStore
  {
    struct Component
    {
      ComponentInfo info;
      ComponentId id = No_Component;
    };

    struct Query
    {
      ecs_query_t *query = nullptr;
      std::vector<std::size_t> sizes;
      bool parents_first = false;
    };

    ecs_world_t *_world = nullptr;
    ComponentId _components_scope = No_Entity;

    // components are handed to Flecs by address, so they must not move
    std::vector<std::unique_ptr<Component>> _components;
    std::unordered_map<ComponentId, Component *> _components_by_id;
    std::unordered_map<std::string, Component *> _components_by_name;

    std::vector<Query> _queries;

    // what Flecs puts at the top for itself, which is not part of the world
    std::vector<Entity> _own_entities;

    std::shared_ptr<Logger> _logger;

    [[nodiscard]] const Component *FindComponentById(ComponentId component) const;

  public:
    explicit Flecs_EntityStore(const std::shared_ptr<Logger> &logger);

    ~Flecs_EntityStore();

    void Initialize() override;

    void CleanUp() override;

    ComponentId RegisterComponent(const ComponentInfo &info) override;

    ComponentId FindComponent(const std::string &name) override;

    using EntityStore::CreateEntity;

    Entity CreateEntity(const std::string &name, Entity parent) override;

    void DestroyEntity(Entity entity) override;

    bool IsAlive(Entity entity) override;

    Entity FindEntity(const std::string &path) override;

    std::string GetName(Entity entity) override;

    void SetParent(Entity entity, Entity parent) override;

    Entity GetParent(Entity entity) override;

    std::vector<Entity> GetChildren(Entity entity) override;

    void SetComponent(Entity entity, ComponentId component, const void *value) override;

    void *GetComponent(Entity entity, ComponentId component) override;

    bool HasComponent(Entity entity, ComponentId component) override;

    void RemoveComponent(Entity entity, ComponentId component) override;

    QueryId CreateQuery(const QueryInfo &info) override;

    void Each(QueryId query, const std::function<void(const EntityBlock &)> &visit) override;
  };
} // neon

#endif //FLECS_ENTITY_STORE_HPP
