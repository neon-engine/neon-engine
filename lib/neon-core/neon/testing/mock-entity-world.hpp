#ifndef MOCK_ENTITY_WORLD_HPP
#define MOCK_ENTITY_WORLD_HPP

#include <gmock/gmock.h>

#include <neon/world-system/ecs/entity-system.hpp>
#include <neon/world-system/ecs/scene.hpp>

namespace neon::testing
{
  class MockEntitySystem : public EntitySystem
  {
  public:
    MOCK_METHOD(void, Initialize, (EntityStore &store), (override));

    MOCK_METHOD(void, Update, (EntityStore &store, double delta_time), (override));
  };

  class MockScene : public Scene
  {
  public:
    MOCK_METHOD(bool, Populate, (EntityStore &store), (override));
    MOCK_METHOD(bool, Load, (EntityStore &store, const std::string &path), (override));
  };
} // neon::testing

#endif //MOCK_ENTITY_WORLD_HPP
