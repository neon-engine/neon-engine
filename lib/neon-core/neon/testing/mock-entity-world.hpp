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
    MOCK_METHOD(
      Entity, Spawn, (EntityStore &store, const std::string &path, Entity parent, const DataValue &overrides),
      (override));

    /// What CouldNotBeRead() answers. It is no mocked call, so that a test
    /// sets it once.
    bool could_not_be_read = false;

    [[nodiscard]] bool CouldNotBeRead() const override
    {
      return could_not_be_read;
    }

    /// What ReadAhead() answers, for every path, in the same way.
    bool can_read_ahead = true;

    bool ReadAhead(const std::string &path) override
    {
      return can_read_ahead;
    }
  };
} // neon::testing

#endif //MOCK_ENTITY_WORLD_HPP
