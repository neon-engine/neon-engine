#include "entity-store.hpp"

#include <stdexcept>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/mock-entity-store.hpp>

// EntityStore is an interface. What it brings along itself are the functions
// that work with a C++ type, which are tested here against a mock of the
// functions a backend implements.

namespace
{
  using neon::ComponentId;
  using neon::ComponentInfo;
  using neon::Entity;
  using neon::EntityBlock;
  using neon::No_Entity;
  using neon::QueryInfo;
  using neon::QueryOrder;
  using neon::testing::MockEntityStore;
  using ::testing::_;
  using ::testing::AllOf;
  using ::testing::DoAll;
  using ::testing::ElementsAre;
  using ::testing::Field;
  using ::testing::Return;
  using ::testing::SaveArg;
  using ::testing::StrictMock;

  struct Position
  {
    float x = 0.0f;
    float y = 0.0f;
  };

  struct Velocity
  {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
  };

  struct Name
  {
    std::string text;
  };

  constexpr ComponentId position_id = 11;
  constexpr ComponentId velocity_id = 22;
  constexpr Entity player = 7;

  class EntityStoreTest : public ::testing::Test
  {
  protected:
    StrictMock<MockEntityStore> _store;

    void RegisterPositionAndVelocity()
    {
      EXPECT_CALL(_store, RegisterComponent(Field(&ComponentInfo::name, "Position"))).WillOnce(Return(position_id));
      EXPECT_CALL(_store, RegisterComponent(Field(&ComponentInfo::name, "Velocity"))).WillOnce(Return(velocity_id));
      _store.Register<Position>("Position");
      _store.Register<Velocity>("Velocity");
    }
  };

  TEST(EntityStore, NoEntityAndNoComponentAreZero)
  {
    EXPECT_EQ(neon::No_Entity, 0u);
    EXPECT_EQ(neon::No_Component, 0u);
  }

  TEST_F(EntityStoreTest, RegistersATypeWithWhatDescribesIt)
  {
    ComponentInfo info;
    EXPECT_CALL(_store, RegisterComponent(_)).WillOnce(DoAll(SaveArg<0>(&info), Return(position_id)));

    EXPECT_EQ(_store.Register<Position>("Position"), position_id);

    EXPECT_EQ(info.name, "Position");
    EXPECT_EQ(info.size, sizeof(Position));
    EXPECT_EQ(info.alignment, alignof(Position));
    EXPECT_NE(info.construct, nullptr);
    EXPECT_NE(info.destruct, nullptr);
    EXPECT_NE(info.copy, nullptr);
    EXPECT_NE(info.move, nullptr);
    EXPECT_FALSE(info.on_remove);
  }

  TEST_F(EntityStoreTest, HandsTheComponentToOnRemoveAsItsType)
  {
    ComponentInfo info;
    EXPECT_CALL(_store, RegisterComponent(_)).WillOnce(DoAll(SaveArg<0>(&info), Return(position_id)));

    Entity removed_from = No_Entity;
    std::string removed;
    _store.Register<Name>("Name", [&](const Entity entity, Name &name)
    {
      removed_from = entity;
      removed = name.text;
      name.text = "seen";
    });

    ASSERT_TRUE(info.on_remove);
    Name name{"player one"};
    info.on_remove(player, &name);

    EXPECT_EQ(removed_from, player);
    EXPECT_EQ(removed, "player one");
    EXPECT_EQ(name.text, "seen");
  }

  TEST_F(EntityStoreTest, KnowsTheIdATypeWasRegisteredUnder)
  {
    RegisterPositionAndVelocity();

    EXPECT_EQ(_store.IdOf<Position>(), position_id);
    EXPECT_EQ(_store.IdOf<Velocity>(), velocity_id);
  }

  TEST_F(EntityStoreTest, RefusesATypeThatWasNotRegistered)
  {
    RegisterPositionAndVelocity();
    const Name name;

    EXPECT_THROW((void) _store.IdOf<Name>(), std::runtime_error);
    EXPECT_THROW(_store.Set(player, name), std::runtime_error);
    EXPECT_THROW((void) _store.Get<Name>(player), std::runtime_error);
    EXPECT_THROW((void) _store.Has<Name>(player), std::runtime_error);
    EXPECT_THROW(_store.Remove<Name>(player), std::runtime_error);
    EXPECT_THROW((void) (_store.Query<Position, Name>()), std::runtime_error);
  }

  TEST_F(EntityStoreTest, SaysThatTheComponentWasNotRegistered)
  {
    try
    {
      (void) _store.IdOf<Name>();
      FAIL() << "nothing was thrown";
    } catch (const std::runtime_error &error)
    {
      EXPECT_STREQ(error.what(), "A component was used before it was registered");
    }
  }

  TEST_F(EntityStoreTest, TakesTheIdOfTheLastRegistrationOfAType)
  {
    EXPECT_CALL(_store, RegisterComponent(_)).WillOnce(Return(position_id)).WillOnce(Return(velocity_id));

    _store.Register<Position>("Position");
    _store.Register<Position>("Place");

    EXPECT_EQ(_store.IdOf<Position>(), velocity_id);
  }

  TEST_F(EntityStoreTest, CreatesAnEntityAtTheTopWhenNoParentIsGiven)
  {
    EXPECT_CALL(_store, CreateEntity("player", No_Entity)).WillOnce(Return(player));

    EXPECT_EQ(_store.CreateEntity("player"), player);
  }

  TEST_F(EntityStoreTest, SetsTheComponentOfTheType)
  {
    RegisterPositionAndVelocity();
    const Velocity velocity{1.0, 2.0, 3.0};

    EXPECT_CALL(_store, SetComponent(player, velocity_id, &velocity));

    _store.Set(player, velocity);
  }

  TEST_F(EntityStoreTest, GetsTheComponentOfTheType)
  {
    RegisterPositionAndVelocity();
    Position position{1.0f, 2.0f};

    EXPECT_CALL(_store, GetComponent(player, position_id)).WillOnce(Return(&position));
    EXPECT_CALL(_store, GetComponent(player, velocity_id)).WillOnce(Return(nullptr));

    EXPECT_EQ(_store.Get<Position>(player), &position);
    EXPECT_EQ(_store.Get<Velocity>(player), nullptr);
  }

  TEST_F(EntityStoreTest, AsksWhetherAnEntityHasTheComponentOfTheType)
  {
    RegisterPositionAndVelocity();

    EXPECT_CALL(_store, HasComponent(player, position_id)).WillOnce(Return(true));
    EXPECT_CALL(_store, HasComponent(player, velocity_id)).WillOnce(Return(false));

    EXPECT_TRUE(_store.Has<Position>(player));
    EXPECT_FALSE(_store.Has<Velocity>(player));
  }

  TEST_F(EntityStoreTest, RemovesTheComponentOfTheType)
  {
    RegisterPositionAndVelocity();

    EXPECT_CALL(_store, RemoveComponent(player, velocity_id));

    _store.Remove<Velocity>(player);
  }

  TEST_F(EntityStoreTest, CreatesAQueryForTheTypesInTheOrderTheyAreNamed)
  {
    RegisterPositionAndVelocity();

    EXPECT_CALL(
      _store,
      CreateQuery(
        AllOf(
          Field(&QueryInfo::components, ElementsAre(velocity_id, position_id)),
          Field(&QueryInfo::order, QueryOrder::Any))))
      .WillOnce(Return(3));

    EXPECT_EQ((_store.Query<Velocity, Position>()), 3u);
  }

  TEST_F(EntityStoreTest, CreatesAQueryWithTheOrderThatWasAskedFor)
  {
    RegisterPositionAndVelocity();

    EXPECT_CALL(
      _store,
      CreateQuery(
        AllOf(
          Field(&QueryInfo::components, ElementsAre(position_id)),
          Field(&QueryInfo::order, QueryOrder::ParentsFirst))))
      .WillOnce(Return(4));

    EXPECT_EQ(_store.Query<Position>(QueryOrder::ParentsFirst), 4u);
  }

  TEST(EntityBlock, IsEmptyUntilItIsFilledIn)
  {
    const EntityBlock block;

    EXPECT_EQ(block.count, 0u);
    EXPECT_EQ(block.entities, nullptr);
    EXPECT_EQ(block.parent, No_Entity);
    for (std::size_t i = 0; i < EntityBlock::Max_Components; i++) { EXPECT_EQ(block.Column<Position>(i), nullptr); }
  }

  TEST(EntityBlock, HandsOutItsColumnsAsTheTypeAskedFor)
  {
    Position positions[2] = {{1.0f, 2.0f}, {3.0f, 4.0f}};
    Velocity velocities[2] = {{5.0, 6.0, 7.0}, {8.0, 9.0, 10.0}};
    EntityBlock block;
    block.count = 2;
    block.columns[0] = positions;
    block.columns[1] = velocities;

    EXPECT_EQ(block.Column<Position>(0), positions);
    EXPECT_EQ(block.Column<Velocity>(1), velocities);
    EXPECT_EQ(block.Column<Position>(0)[1].x, 3.0f);
    EXPECT_EQ(block.Column<Velocity>(1)[1].z, 10.0);
  }
}
