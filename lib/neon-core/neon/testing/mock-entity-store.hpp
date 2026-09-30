#ifndef MOCK_ENTITY_STORE_HPP
#define MOCK_ENTITY_STORE_HPP

#include <functional>
#include <string>

#include <gmock/gmock.h>

#include <neon/world-system/ecs/entity-store.hpp>

namespace neon::testing
{
  /// An entity store that stores nothing, for checking what it is asked to
  /// do. To work with entities, take FakeEntityStore.
  class MockEntityStore : public EntityStore
  {
  public:
    MOCK_METHOD(void, Initialize, (), (override));

    MOCK_METHOD(void, CleanUp, (), (override));

    MOCK_METHOD(ComponentId, RegisterComponent, (const ComponentInfo &info), (override));

    MOCK_METHOD(ComponentId, FindComponent, (const std::string &name), (override));

    using EntityStore::CreateEntity;

    MOCK_METHOD(Entity, CreateEntity, (const std::string &name, Entity parent), (override));

    MOCK_METHOD(void, DestroyEntity, (Entity entity), (override));

    MOCK_METHOD(bool, IsAlive, (Entity entity), (override));

    MOCK_METHOD(Entity, FindEntity, (const std::string &path), (override));

    MOCK_METHOD(std::string, GetName, (Entity entity), (override));

    MOCK_METHOD(void, SetParent, (Entity entity, Entity parent), (override));

    MOCK_METHOD(Entity, GetParent, (Entity entity), (override));
    MOCK_METHOD(std::vector<Entity>, GetChildren, (Entity entity), (override));

    MOCK_METHOD(void, SetComponent, (Entity entity, ComponentId component, const void *value), (override));

    MOCK_METHOD(void *, GetComponent, (Entity entity, ComponentId component), (override));

    MOCK_METHOD(bool, HasComponent, (Entity entity, ComponentId component), (override));

    MOCK_METHOD(void, RemoveComponent, (Entity entity, ComponentId component), (override));

    MOCK_METHOD(QueryId, CreateQuery, (const QueryInfo &info), (override));

    MOCK_METHOD(
      void,
      Each,
      (QueryId query, const std::function<void(const EntityBlock &)> &visit),
      (override));
  };
} // neon::testing

#endif //MOCK_ENTITY_STORE_HPP
