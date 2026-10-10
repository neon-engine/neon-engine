#include "flecs-entity-store.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/recording-logger.hpp>

// Everything here goes through EntityStore, the interface the engine sees.
// Flecs itself is never called.

namespace
{
  using neon::ComponentId;
  using neon::ComponentInfo;
  using neon::Entity;
  using neon::EntityBlock;
  using neon::EntityStore;
  using neon::Flecs_EntityStore;
  using neon::No_Component;
  using neon::No_Entity;
  using neon::No_Query;
  using neon::QueryOrder;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;
  using ::testing::UnorderedElementsAre;

  struct Position
  {
    float x = 0.0f;
    float y = 0.0f;
  };

  struct Velocity
  {
    float x = 0.0f;
    float y = 0.0f;
  };

  struct Health
  {
    int points = 100;
  };

  /// Owns memory, so that a component that is copied byte by byte, or
  /// forgotten, shows.
  struct Inventory
  {
    std::string owner;
    std::vector<std::string> items;
  };

  /// Counts how many of it are alive.
  struct Counted
  {
    static inline int alive = 0;

    int value = 0;

    Counted() { alive++; }
    explicit Counted(const int value) : value(value) { alive++; }
    Counted(const Counted &other) : value(other.value) { alive++; }
    Counted(Counted &&other) noexcept : value(other.value) { alive++; }
    Counted &operator=(const Counted &) = default;
    Counted &operator=(Counted &&) noexcept = default;
    ~Counted() { alive--; }
  };

  // long enough that a string keeps them outside of itself
  const std::string long_name = "a name that is too long to be kept inside of the string itself";
  const std::string long_item = "an item with a description that is just as long as the name is";

  struct Removal
  {
    Entity entity;
    int points;

    bool operator==(const Removal &) const = default;
  };

  class FlecsEntityStoreTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    Flecs_EntityStore _flecs{_logger};

    // the tests see what the engine sees
    EntityStore &_store = _flecs;

    void SetUp() override
    {
      _store.Initialize();
      _store.Register<Position>("Position");
      _store.Register<Velocity>("Velocity");
    }

    // The store tells components that they are removed when it is cleaned
    // up. That has to happen while what the tests listen with is still
    // there, which it is not any more when the store is destroyed.
    void TearDown() override
    {
      _store.CleanUp();
    }

    /// The entities a query hands over, in the order it does.
    std::vector<Entity> Visit(const neon::QueryId query) const
    {
      std::vector<Entity> visited;
      _store.Each(query, [&](const EntityBlock &block)
      {
        for (std::size_t i = 0; i < block.count; i++) { visited.push_back(block.entities[i]); }
      });
      return visited;
    }

    static std::size_t IndexOf(const std::vector<Entity> &entities, const Entity entity)
    {
      return static_cast<std::size_t>(std::ranges::find(entities, entity) - entities.begin());
    }
  };

  // starting and stopping

  TEST(FlecsEntityStore, CanBeCreatedAndDestroyedWithoutEverStarting)
  {
    const auto logger = std::make_shared<RecordingLogger>();

    {
      Flecs_EntityStore store(logger);
    }

    EXPECT_EQ(logger->Count(LogLevel::Error), 0u);
  }

  TEST(FlecsEntityStore, CleanUpBeforeInitializeDoesNothing)
  {
    Flecs_EntityStore store(std::make_shared<RecordingLogger>());

    store.CleanUp();
    store.CleanUp();
  }

  TEST_F(FlecsEntityStoreTest, CleanUpTwiceDoesNothingTheSecondTime)
  {
    _store.CreateEntity("player");

    _store.CleanUp();
    _store.CleanUp();
  }

  TEST_F(FlecsEntityStoreTest, InitializeTwiceKeepsWhatIsThere)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{1.0f, 2.0f});

    _store.Initialize();

    EXPECT_TRUE(_store.IsAlive(player));
    ASSERT_NE(_store.Get<Position>(player), nullptr);
    EXPECT_EQ(_store.Get<Position>(player)->x, 1.0f);
  }

  TEST_F(FlecsEntityStoreTest, IsEmptyAndUsableWhenInitializedAfterCleanUp)
  {
    _store.CreateEntity("player");
    _store.CleanUp();

    _store.Initialize();

    EXPECT_EQ(_store.FindEntity("player"), No_Entity);
    EXPECT_EQ(_store.FindComponent("Position"), No_Component);

    _store.Register<Position>("Position");
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{3.0f, 4.0f});
    const auto query = _store.Query<Position>();

    EXPECT_THAT(Visit(query), ElementsAre(player));
    EXPECT_EQ(_store.Get<Position>(player)->y, 4.0f);
  }

  TEST_F(FlecsEntityStoreTest, ForgetsItsQueriesWhenCleanedUp)
  {
    const auto query = _store.Query<Position>();
    _store.CleanUp();
    _store.Initialize();

    EXPECT_THAT(Visit(query), IsEmpty());
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "A query was used that the store does not know"));
  }

  // components

  // turning a component off and on

  TEST_F(FlecsEntityStoreTest, AComponentThatIsTurnedOffIsNotThereForWhoAsksAndKeepsWhatItHolds)
  {
    const Entity nail = _store.CreateEntity("nail");
    _store.Set(nail, Position{1.0f, 2.0f});
    _store.Set(nail, Velocity{3.0f, 4.0f});
    const auto both = _store.Query<Position, Velocity>();
    const auto positions = _store.Query<Position>();

    EXPECT_TRUE(_store.IsEnabled<Velocity>(nail));
    _store.SetEnabled<Velocity>(nail, false);

    EXPECT_FALSE(_store.IsEnabled<Velocity>(nail));
    EXPECT_FALSE(_store.Has<Velocity>(nail));
    EXPECT_EQ(_store.Get<Velocity>(nail), nullptr);
    EXPECT_TRUE(Visit(both).empty());
    EXPECT_THAT(Visit(positions), ::testing::ElementsAre(nail)) << "the other component is as it was";

    // what it holds is kept, and can be reached
    const auto *kept = static_cast<const Velocity *>(_store.GetComponentData(nail, _store.IdOf<Velocity>()));
    ASSERT_NE(kept, nullptr);
    EXPECT_EQ(kept->x, 3.0f);

    _store.SetEnabled<Velocity>(nail, true);

    EXPECT_TRUE(_store.Has<Velocity>(nail));
    ASSERT_NE(_store.Get<Velocity>(nail), nullptr);
    EXPECT_EQ(_store.Get<Velocity>(nail)->y, 4.0f);
    EXPECT_THAT(Visit(both), ::testing::ElementsAre(nail));
  }

  TEST_F(FlecsEntityStoreTest, AQueryPassesOverOnlyTheEntitiesWhoseComponentIsOff)
  {
    std::vector<Entity> nails;
    for (int i = 0; i < 6; i++)
    {
      nails.push_back(_store.CreateEntity("nail " + std::to_string(i)));
      _store.Set(nails.back(), Position{static_cast<float>(i), 0.0f});
    }
    const auto query = _store.Query<Position>();

    _store.SetEnabled<Position>(nails[1], false);
    _store.SetEnabled<Position>(nails[4], false);

    // and each of the others is handed over with what it holds
    std::vector<float> seen;
    _store.Each(query, [&](const EntityBlock &block)
    {
      const auto *positions = block.Column<Position>(0);
      for (std::size_t i = 0; i < block.count; i++)
      {
        EXPECT_EQ(positions[i].x, static_cast<float>(IndexOf(nails, block.entities[i])));
        seen.push_back(positions[i].x);
      }
    });
    EXPECT_THAT(seen, ::testing::UnorderedElementsAre(0.0f, 2.0f, 3.0f, 5.0f));
  }

  TEST_F(FlecsEntityStoreTest, TellsAComponentThatItWasTurnedOffAndOnAndOnlyWhenThatChanged)
  {
    struct Body
    {
      int id = 0;
    };

    std::vector<std::pair<Entity, bool>> told;
    _store.Register<Body>("Body", {}, [&](const Entity entity, Body &body, const bool enabled)
    {
      EXPECT_EQ(body.id, 7);
      told.emplace_back(entity, enabled);
    });
    const Entity crate = _store.CreateEntity("crate");
    _store.Set(crate, Body{7});

    _store.SetEnabled<Body>(crate, true);
    EXPECT_TRUE(told.empty()) << "it was on already";

    _store.SetEnabled<Body>(crate, false);
    _store.SetEnabled<Body>(crate, false);
    _store.SetEnabled<Body>(crate, true);

    EXPECT_THAT(told, ::testing::ElementsAre(std::pair(crate, false), std::pair(crate, true)));
  }

  TEST_F(FlecsEntityStoreTest, TurningOffWhatAnEntityDoesNotHaveDoesNothing)
  {
    const Entity bare = _store.CreateEntity("bare");

    _store.SetEnabled<Position>(bare, false);
    _store.SetEnabled<Position>(123456, false);

    EXPECT_FALSE(_store.IsEnabled<Position>(bare));
    EXPECT_EQ(_store.GetComponentData(bare, _store.IdOf<Position>()), nullptr);

    // and one it is given afterwards is on
    _store.Set(bare, Position{});
    EXPECT_TRUE(_store.IsEnabled<Position>(bare));
  }

  TEST_F(FlecsEntityStoreTest, RegistersAComponentUnderAnIdOfItsOwn)
  {
    const ComponentId position = _store.FindComponent("Position");
    const ComponentId velocity = _store.FindComponent("Velocity");

    EXPECT_NE(position, No_Component);
    EXPECT_NE(velocity, No_Component);
    EXPECT_NE(position, velocity);
    EXPECT_EQ(_store.IdOf<Position>(), position);
    EXPECT_EQ(_store.IdOf<Velocity>(), velocity);
  }

  TEST_F(FlecsEntityStoreTest, ReturnsTheSameIdWhenANameIsRegisteredAgain)
  {
    const ComponentId first = _store.FindComponent("Position");

    EXPECT_EQ(_store.Register<Position>("Position"), first);
    EXPECT_EQ(_store.RegisterComponent(ComponentInfo::Of<Position>("Position")), first);
  }

  TEST_F(FlecsEntityStoreTest, RefusesANameThatIsRegisteredAgainWithAnotherSize)
  {
    EXPECT_EQ(_store.RegisterComponent(ComponentInfo::Of<Inventory>("Position")), No_Component);
    EXPECT_EQ(_store.RegisterComponent(ComponentInfo::Of<double>("Position")), No_Component);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 2u);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Component 'Position' was registered before with another size"));

    // what was registered first stays as it was
    EXPECT_NE(_store.FindComponent("Position"), No_Component);
    EXPECT_EQ(_store.GetComponentSize(_store.FindComponent("Position")), sizeof(Position));
  }

  TEST_F(FlecsEntityStoreTest, DoesNotFindAComponentThatWasNeverRegistered)
  {
    EXPECT_EQ(_store.FindComponent("Health"), No_Component);
    EXPECT_EQ(_store.FindComponent(""), No_Component);
    EXPECT_EQ(_store.FindComponent("position"), No_Component);
  }

  TEST_F(FlecsEntityStoreTest, RefusesAComponentThatIsNotDescribedCompletely)
  {
    auto without_size = ComponentInfo::Of<Health>("WithoutSize");
    without_size.size = 0;
    auto without_construct = ComponentInfo::Of<Health>("WithoutConstruct");
    without_construct.construct = nullptr;
    auto without_destruct = ComponentInfo::Of<Health>("WithoutDestruct");
    without_destruct.destruct = nullptr;
    auto without_copy = ComponentInfo::Of<Health>("WithoutCopy");
    without_copy.copy = nullptr;
    auto without_move = ComponentInfo::Of<Health>("WithoutMove");
    without_move.move = nullptr;

    for (const auto &info : {without_size, without_construct, without_destruct, without_copy, without_move})
    {
      EXPECT_EQ(_store.RegisterComponent(info), No_Component) << info.name;
      EXPECT_EQ(_store.FindComponent(info.name), No_Component) << info.name;
      EXPECT_TRUE(_logger->Contains(LogLevel::Error, "Component '" + info.name + "' is not described completely")) << info.name;
    }
  }

  TEST_F(FlecsEntityStoreTest, ATypeThatWasNeverRegisteredIsOnNoEntityAndNothingIsThrown)
  {
    const Entity player = _store.CreateEntity("player");

    EXPECT_EQ(_store.IdOf<Health>(), No_Component);

    // what asks about it is told that it is not there, and the game goes on
    EXPECT_EQ(_store.Get<Health>(player), nullptr);
    EXPECT_FALSE(_store.Has<Health>(player));
    EXPECT_FALSE(_store.IsEnabled<Health>(player));
    EXPECT_EQ(_store.GetComponentData(player, No_Component), nullptr);
    _store.Remove<Health>(player);
    _store.SetEnabled<Health>(player, false);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);

    // what would change the world with it is refused and says so
    _store.Set(player, Health{});
    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "A component was used that the store does not know"));

    EXPECT_EQ((_store.Query<Position, Health>()), No_Query);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 2u);
    EXPECT_TRUE(_store.IsAlive(player));
  }

  TEST_F(FlecsEntityStoreTest, RefusesToSetAComponentIdItDoesNotKnow)
  {
    const Entity player = _store.CreateEntity("player");
    const Health health;

    _store.SetComponent(player, 123456789, &health);

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "A component was used that the store does not know"));
    EXPECT_FALSE(_store.HasComponent(player, 123456789));
  }

  TEST_F(FlecsEntityStoreTest, KeepsComponentsApartFromEntitiesOfTheSameName)
  {
    const Entity entity = _store.CreateEntity("Position");

    EXPECT_NE(entity, _store.FindComponent("Position"));
    EXPECT_EQ(_store.FindEntity("Position"), entity);

    _store.Set(entity, Position{1.0f, 2.0f});
    EXPECT_EQ(_store.Get<Position>(entity)->y, 2.0f);
  }

  // what an entity carries

  TEST_F(FlecsEntityStoreTest, CarriesNothingWhenItIsCreated)
  {
    const Entity player = _store.CreateEntity("player");

    EXPECT_FALSE(_store.Has<Position>(player));
    EXPECT_EQ(_store.Get<Position>(player), nullptr);
  }

  TEST_F(FlecsEntityStoreTest, CarriesACopyOfWhatItWasGiven)
  {
    const Entity player = _store.CreateEntity("player");
    Position position{1.0f, 2.0f};

    _store.Set(player, position);
    position.x = 9.0f;

    ASSERT_TRUE(_store.Has<Position>(player));
    ASSERT_NE(_store.Get<Position>(player), nullptr);
    EXPECT_EQ(_store.Get<Position>(player)->x, 1.0f);
    EXPECT_EQ(_store.Get<Position>(player)->y, 2.0f);
    EXPECT_FALSE(_store.Has<Velocity>(player));
  }

  TEST_F(FlecsEntityStoreTest, ReplacesAComponentThatIsSetAgain)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{1.0f, 2.0f});

    _store.Set(player, Position{3.0f, 4.0f});

    EXPECT_EQ(_store.Get<Position>(player)->x, 3.0f);
    EXPECT_EQ(_store.Get<Position>(player)->y, 4.0f);
  }

  TEST_F(FlecsEntityStoreTest, LetsAComponentBeChangedThroughItsPointer)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{1.0f, 2.0f});

    _store.Get<Position>(player)->x = 7.0f;

    EXPECT_EQ(_store.Get<Position>(player)->x, 7.0f);
  }

  TEST_F(FlecsEntityStoreTest, KeepsTheComponentsOfEntitiesApart)
  {
    const Entity first = _store.CreateEntity("first");
    const Entity second = _store.CreateEntity("second");
    _store.Set(first, Position{1.0f, 1.0f});
    _store.Set(second, Position{2.0f, 2.0f});

    _store.Get<Position>(first)->x = 5.0f;

    EXPECT_EQ(_store.Get<Position>(first)->x, 5.0f);
    EXPECT_EQ(_store.Get<Position>(second)->x, 2.0f);
  }

  TEST_F(FlecsEntityStoreTest, KeepsAComponentWhenAnotherIsAddedOrRemoved)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{1.0f, 2.0f});

    _store.Set(player, Velocity{3.0f, 4.0f});
    EXPECT_EQ(_store.Get<Position>(player)->x, 1.0f);
    EXPECT_EQ(_store.Get<Velocity>(player)->y, 4.0f);

    _store.Remove<Velocity>(player);
    EXPECT_EQ(_store.Get<Position>(player)->y, 2.0f);
    EXPECT_FALSE(_store.Has<Velocity>(player));
    EXPECT_EQ(_store.Get<Velocity>(player), nullptr);
  }

  TEST_F(FlecsEntityStoreTest, RemovingAComponentTheEntityDoesNotCarryDoesNothing)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{1.0f, 2.0f});

    _store.Remove<Velocity>(player);

    EXPECT_TRUE(_store.IsAlive(player));
    EXPECT_TRUE(_store.Has<Position>(player));
  }

  TEST_F(FlecsEntityStoreTest, AnEntityThatIsGoneCarriesNothing)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{1.0f, 2.0f});
    _store.DestroyEntity(player);

    EXPECT_FALSE(_store.Has<Position>(player));
    EXPECT_EQ(_store.Get<Position>(player), nullptr);
    _store.Remove<Position>(player);

    EXPECT_FALSE(_store.Has<Position>(No_Entity));
    EXPECT_EQ(_store.Get<Position>(No_Entity), nullptr);
    _store.Remove<Position>(No_Entity);
  }

  // entities

  TEST_F(FlecsEntityStoreTest, CreatesEntitiesThatAreAliveAndDifferent)
  {
    const Entity first = _store.CreateEntity("");
    const Entity second = _store.CreateEntity("");

    EXPECT_NE(first, No_Entity);
    EXPECT_NE(second, No_Entity);
    EXPECT_NE(first, second);
    EXPECT_TRUE(_store.IsAlive(first));
    EXPECT_TRUE(_store.IsAlive(second));
  }

  TEST_F(FlecsEntityStoreTest, NoEntityIsNotAlive)
  {
    EXPECT_FALSE(_store.IsAlive(No_Entity));
    EXPECT_FALSE(_store.IsAlive(987654321));
  }

  TEST_F(FlecsEntityStoreTest, KnowsTheNameOfAnEntity)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity unnamed = _store.CreateEntity("");

    EXPECT_EQ(_store.GetName(player), "player");
    EXPECT_EQ(_store.GetName(unnamed), "");
    EXPECT_EQ(_store.GetName(No_Entity), "");
  }

  TEST_F(FlecsEntityStoreTest, FindsAnEntityAtTheTopByItsName)
  {
    const Entity player = _store.CreateEntity("player");

    EXPECT_EQ(_store.FindEntity("player"), player);
    EXPECT_EQ(_store.FindEntity("Player"), No_Entity);
    EXPECT_EQ(_store.FindEntity("enemy"), No_Entity);
    EXPECT_EQ(_store.FindEntity(""), No_Entity);
  }

  TEST_F(FlecsEntityStoreTest, ReturnsTheEntityThatHasTheNameAlready)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{1.0f, 2.0f});

    const Entity again = _store.CreateEntity("player");

    EXPECT_EQ(again, player);
    EXPECT_TRUE(_store.Has<Position>(again));
  }

  TEST_F(FlecsEntityStoreTest, AllowsTheSameNameUnderDifferentParents)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity enemy = _store.CreateEntity("enemy");

    const Entity camera_of_player = _store.CreateEntity("camera", player);
    const Entity camera_of_enemy = _store.CreateEntity("camera", enemy);
    const Entity camera_at_the_top = _store.CreateEntity("camera");

    EXPECT_NE(camera_of_player, camera_of_enemy);
    EXPECT_NE(camera_of_player, camera_at_the_top);
    EXPECT_NE(camera_of_enemy, camera_at_the_top);
    EXPECT_EQ(_store.CreateEntity("camera", player), camera_of_player);
  }

  TEST_F(FlecsEntityStoreTest, TakesANameWithDotsAsItIs)
  {
    const Entity model = _store.CreateEntity("cube.v2.obj");

    EXPECT_EQ(_store.GetName(model), "cube.v2.obj");
    EXPECT_EQ(_store.GetParent(model), No_Entity);
    EXPECT_EQ(_store.FindEntity("cube.v2.obj"), model);
    EXPECT_EQ(_store.FindEntity("cube"), No_Entity);
    EXPECT_EQ(_store.CreateEntity("cube.v2.obj"), model);
  }

  TEST_F(FlecsEntityStoreTest, TakesANameWithDotsBelowAParentAsItIs)
  {
    const Entity models = _store.CreateEntity("models");
    const Entity model = _store.CreateEntity("cube.obj", models);

    EXPECT_EQ(_store.GetName(model), "cube.obj");
    EXPECT_EQ(_store.GetParent(model), models);
    EXPECT_EQ(_store.FindEntity("models/cube.obj"), model);
  }

  TEST_F(FlecsEntityStoreTest, FindsAnEntityByThePathFromTheTop)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity arm = _store.CreateEntity("arm", player);
    const Entity hand = _store.CreateEntity("hand", arm);

    EXPECT_EQ(_store.FindEntity("player/arm"), arm);
    EXPECT_EQ(_store.FindEntity("player/arm/hand"), hand);
    EXPECT_EQ(_store.FindEntity("arm"), No_Entity);
    EXPECT_EQ(_store.FindEntity("hand"), No_Entity);
    EXPECT_EQ(_store.FindEntity("player/hand"), No_Entity);
    EXPECT_EQ(_store.FindEntity("player/arm/hand/finger"), No_Entity);
    EXPECT_EQ(_store.FindEntity("player.arm"), No_Entity);
  }

  // parents and children

  TEST_F(FlecsEntityStoreTest, KnowsTheParentOfAnEntity)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity arm = _store.CreateEntity("arm", player);
    const Entity hand = _store.CreateEntity("hand", arm);

    EXPECT_EQ(_store.GetParent(player), No_Entity);
    EXPECT_EQ(_store.GetParent(arm), player);
    EXPECT_EQ(_store.GetParent(hand), arm);
    EXPECT_EQ(_store.GetParent(No_Entity), No_Entity);
  }

  TEST_F(FlecsEntityStoreTest, MovesAnEntityUnderAnotherParent)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity enemy = _store.CreateEntity("enemy");
    const Entity sword = _store.CreateEntity("sword", player);
    _store.Set(sword, Position{1.0f, 2.0f});

    _store.SetParent(sword, enemy);

    EXPECT_EQ(_store.GetParent(sword), enemy);
    EXPECT_EQ(_store.FindEntity("enemy/sword"), sword);
    EXPECT_EQ(_store.FindEntity("player/sword"), No_Entity);
    EXPECT_EQ(_store.GetName(sword), "sword");
    EXPECT_EQ(_store.Get<Position>(sword)->x, 1.0f);
  }

  TEST_F(FlecsEntityStoreTest, MovesAnEntityToTheTop)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity sword = _store.CreateEntity("sword", player);

    _store.SetParent(sword, No_Entity);

    EXPECT_EQ(_store.GetParent(sword), No_Entity);
    EXPECT_EQ(_store.FindEntity("sword"), sword);
    EXPECT_EQ(_store.FindEntity("player/sword"), No_Entity);
  }

  TEST_F(FlecsEntityStoreTest, GivesAnEntityFromTheTopAParent)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity sword = _store.CreateEntity("sword");

    _store.SetParent(sword, player);

    EXPECT_EQ(_store.GetParent(sword), player);
    EXPECT_EQ(_store.FindEntity("player/sword"), sword);
  }

  TEST_F(FlecsEntityStoreTest, TakesTheChildrenAlongWhenAnEntityIsMoved)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity enemy = _store.CreateEntity("enemy");
    const Entity arm = _store.CreateEntity("arm", player);
    const Entity hand = _store.CreateEntity("hand", arm);

    _store.SetParent(arm, enemy);

    EXPECT_EQ(_store.GetParent(hand), arm);
    EXPECT_EQ(_store.FindEntity("enemy/arm/hand"), hand);
  }

  TEST_F(FlecsEntityStoreTest, SettingTheParentOfAnEntityThatIsGoneDoesNothing)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity sword = _store.CreateEntity("sword");
    _store.DestroyEntity(sword);

    _store.SetParent(sword, player);
    _store.SetParent(No_Entity, player);

    EXPECT_FALSE(_store.IsAlive(sword));
    EXPECT_EQ(_store.GetParent(sword), No_Entity);
  }

  // destroying

  TEST_F(FlecsEntityStoreTest, DestroysAnEntity)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity enemy = _store.CreateEntity("enemy");

    _store.DestroyEntity(player);

    EXPECT_FALSE(_store.IsAlive(player));
    EXPECT_EQ(_store.FindEntity("player"), No_Entity);
    EXPECT_EQ(_store.GetName(player), "");
    EXPECT_TRUE(_store.IsAlive(enemy));
  }

  TEST_F(FlecsEntityStoreTest, DestroysTheChildrenOfAnEntityWithIt)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity arm = _store.CreateEntity("arm", player);
    const Entity hand = _store.CreateEntity("hand", arm);
    const Entity leg = _store.CreateEntity("leg", player);
    const Entity enemy = _store.CreateEntity("enemy");

    _store.DestroyEntity(player);

    EXPECT_FALSE(_store.IsAlive(player));
    EXPECT_FALSE(_store.IsAlive(arm));
    EXPECT_FALSE(_store.IsAlive(hand));
    EXPECT_FALSE(_store.IsAlive(leg));
    EXPECT_TRUE(_store.IsAlive(enemy));
  }

  TEST_F(FlecsEntityStoreTest, DoesNotCountWhatAQueryNeedsAmongTheEntities)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{1.0f, 2.0f});

    // Flecs keeps an entity for a query that it looks up faster. It is
    // not part of the world, and a scene that is saved must not hold it.
    (void) _store.Query<Position>();
    (void) _store.Query<Position, Velocity>();
    (void) _store.Query<Position>(QueryOrder::ParentsFirst);
    (void) _store.Query<Position, Velocity>(QueryOrder::ParentsFirst);

    EXPECT_THAT(_store.GetChildren(No_Entity), ElementsAre(player));
  }

  TEST_F(FlecsEntityStoreTest, LeavesTheParentWhenAChildIsDestroyed)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity arm = _store.CreateEntity("arm", player);

    _store.DestroyEntity(arm);

    EXPECT_TRUE(_store.IsAlive(player));
    EXPECT_FALSE(_store.IsAlive(arm));
  }

  TEST_F(FlecsEntityStoreTest, DoesNotDestroyAChildThatWasMovedAwayBefore)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity sword = _store.CreateEntity("sword", player);
    _store.SetParent(sword, No_Entity);

    _store.DestroyEntity(player);

    EXPECT_TRUE(_store.IsAlive(sword));
  }

  TEST_F(FlecsEntityStoreTest, DestroyingWhatIsGoneDoesNothing)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity enemy = _store.CreateEntity("enemy");
    _store.DestroyEntity(player);

    _store.DestroyEntity(player);
    _store.DestroyEntity(No_Entity);
    _store.DestroyEntity(987654321);

    EXPECT_TRUE(_store.IsAlive(enemy));
  }

  TEST_F(FlecsEntityStoreTest, DoesNotTakeANewEntityForOneThatIsGone)
  {
    const Entity first = _store.CreateEntity("first");
    _store.DestroyEntity(first);

    const Entity second = _store.CreateEntity("second");

    EXPECT_NE(first, second);
    EXPECT_FALSE(_store.IsAlive(first));
    EXPECT_TRUE(_store.IsAlive(second));
  }

  TEST_F(FlecsEntityStoreTest, LetsANameBeUsedAgainAfterItsEntityIsGone)
  {
    const Entity first = _store.CreateEntity("player");
    _store.Set(first, Position{1.0f, 2.0f});
    _store.DestroyEntity(first);

    const Entity second = _store.CreateEntity("player");

    EXPECT_TRUE(_store.IsAlive(second));
    EXPECT_FALSE(_store.Has<Position>(second));
    EXPECT_EQ(_store.FindEntity("player"), second);
  }

  // queries

  TEST_F(FlecsEntityStoreTest, HandsOverTheEntitiesThatCarryEveryComponentOfTheQuery)
  {
    const Entity moving = _store.CreateEntity("moving");
    _store.Set(moving, Position{});
    _store.Set(moving, Velocity{});
    const Entity standing = _store.CreateEntity("standing");
    _store.Set(standing, Position{});
    const Entity drifting = _store.CreateEntity("drifting");
    _store.Set(drifting, Velocity{});
    _store.CreateEntity("nothing");

    EXPECT_THAT(Visit(_store.Query<Position>()), UnorderedElementsAre(moving, standing));
    EXPECT_THAT(Visit(_store.Query<Velocity>()), UnorderedElementsAre(moving, drifting));
    EXPECT_THAT(Visit(_store.Query<Position, Velocity>()), ElementsAre(moving));
    EXPECT_THAT(Visit(_store.Query<Velocity, Position>()), ElementsAre(moving));
  }

  TEST_F(FlecsEntityStoreTest, HandsOverNothingWhenNothingMatches)
  {
    _store.CreateEntity("nothing");
    int blocks = 0;

    _store.Each(_store.Query<Position>(), [&](const EntityBlock &) { blocks++; });

    EXPECT_EQ(blocks, 0);
  }

  TEST_F(FlecsEntityStoreTest, HandsOverTheComponentsInTheOrderTheQueryNamesThem)
  {
    const Entity first = _store.CreateEntity("first");
    _store.Set(first, Position{1.0f, 2.0f});
    _store.Set(first, Velocity{3.0f, 4.0f});
    const Entity second = _store.CreateEntity("second");
    _store.Set(second, Position{5.0f, 6.0f});
    _store.Set(second, Velocity{7.0f, 8.0f});

    std::size_t visited = 0;
    _store.Each(_store.Query<Velocity, Position>(), [&](const EntityBlock &block)
    {
      const auto *velocities = block.Column<Velocity>(0);
      const auto *positions = block.Column<Position>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        visited++;
        const float offset = block.entities[i] == first ? 0.0f : 4.0f;
        EXPECT_EQ(positions[i].x, 1.0f + offset);
        EXPECT_EQ(positions[i].y, 2.0f + offset);
        EXPECT_EQ(velocities[i].x, 3.0f + offset);
        EXPECT_EQ(velocities[i].y, 4.0f + offset);
      }
    });

    EXPECT_EQ(visited, 2u);
  }

  TEST_F(FlecsEntityStoreTest, PutsTheEntitiesThatCarryTheSameComponentsInOneBlock)
  {
    for (int i = 0; i < 10; i++)
    {
      _store.Set(_store.CreateEntity(""), Position{static_cast<float>(i), 0.0f});
    }

    std::vector<std::size_t> counts;
    float sum = 0.0f;
    _store.Each(_store.Query<Position>(), [&](const EntityBlock &block)
    {
      counts.push_back(block.count);
      for (std::size_t i = 0; i < block.count; i++) { sum += block.Column<Position>(0)[i].x; }
    });

    EXPECT_THAT(counts, ElementsAre(10u));
    EXPECT_EQ(sum, 45.0f);
  }

  TEST_F(FlecsEntityStoreTest, WritesToTheComponentsOfABlockAtOnce)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{1.0f, 2.0f});
    _store.Set(player, Velocity{10.0f, 20.0f});

    _store.Each(_store.Query<Position, Velocity>(), [&](const EntityBlock &block)
    {
      auto *positions = block.Column<Position>(0);
      const auto *velocities = block.Column<Velocity>(1);
      for (std::size_t i = 0; i < block.count; i++)
      {
        positions[i].x += velocities[i].x;
        positions[i].y += velocities[i].y;

        EXPECT_EQ(_store.Get<Position>(block.entities[i])->x, 11.0f);
      }
    });

    EXPECT_EQ(_store.Get<Position>(player)->x, 11.0f);
    EXPECT_EQ(_store.Get<Position>(player)->y, 22.0f);
  }

  TEST_F(FlecsEntityStoreTest, SeesTheEntitiesThatCameAndWentAfterTheQueryWasCreated)
  {
    const auto query = _store.Query<Position>();
    EXPECT_THAT(Visit(query), IsEmpty());

    const Entity first = _store.CreateEntity("first");
    _store.Set(first, Position{});
    const Entity second = _store.CreateEntity("second");
    _store.Set(second, Position{});
    EXPECT_THAT(Visit(query), UnorderedElementsAre(first, second));

    _store.Remove<Position>(first);
    EXPECT_THAT(Visit(query), ElementsAre(second));

    _store.DestroyEntity(second);
    EXPECT_THAT(Visit(query), IsEmpty());
  }

  TEST_F(FlecsEntityStoreTest, KeepsQueriesApart)
  {
    const auto positions = _store.Query<Position>();
    const auto velocities = _store.Query<Velocity>();
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{});

    EXPECT_NE(positions, velocities);
    EXPECT_THAT(Visit(positions), ElementsAre(player));
    EXPECT_THAT(Visit(velocities), IsEmpty());
  }

  TEST_F(FlecsEntityStoreTest, RefusesAQueryWithoutComponents)
  {
    EXPECT_EQ(_store.CreateQuery({}), No_Query);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "A query names between 1 and 8 components, this one names 0"));
  }

  TEST_F(FlecsEntityStoreTest, AcceptsAQueryWithAsManyComponentsAsABlockHolds)
  {
    std::vector<ComponentId> components;
    for (std::size_t i = 0; i < EntityBlock::Max_Components; i++)
    {
      components.push_back(_store.RegisterComponent(ComponentInfo::Of<Health>("Health" + std::to_string(i))));
    }
    const Entity player = _store.CreateEntity("player");
    for (std::size_t i = 0; i < components.size(); i++)
    {
      const Health health{static_cast<int>(i)};
      _store.SetComponent(player, components[i], &health);
    }

    const auto query = _store.CreateQuery({.components = components});

    std::size_t visited = 0;
    _store.Each(query, [&](const EntityBlock &block)
    {
      ASSERT_EQ(block.count, 1u);
      for (std::size_t i = 0; i < components.size(); i++)
      {
        visited++;
        EXPECT_EQ(block.Column<Health>(i)[0].points, static_cast<int>(i));
      }
    });
    EXPECT_EQ(visited, EntityBlock::Max_Components);
  }

  TEST_F(FlecsEntityStoreTest, RefusesAQueryWithMoreComponentsThanABlockHolds)
  {
    std::vector<ComponentId> components;
    for (std::size_t i = 0; i < EntityBlock::Max_Components + 1; i++)
    {
      components.push_back(_store.RegisterComponent(ComponentInfo::Of<Health>("Health" + std::to_string(i))));
    }

    EXPECT_EQ(_store.CreateQuery({.components = components}), No_Query);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "A query names between 1 and 8 components, this one names 9"));
  }

  TEST_F(FlecsEntityStoreTest, RefusesAQueryForAComponentItDoesNotKnow)
  {
    EXPECT_EQ(_store.CreateQuery({.components = {123456789}}), No_Query);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "A component was used that the store does not know"));
  }

  TEST_F(FlecsEntityStoreTest, HandsOverNothingForAQueryItDoesNotKnow)
  {
    _store.Set(_store.CreateEntity("player"), Position{});
    const auto query = _store.Query<Position>();

    // a query that could not be made was logged when it was asked for
    EXPECT_THAT(Visit(No_Query), IsEmpty());
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);

    // one that was never made at all is a mistake of its own
    EXPECT_THAT(Visit(query + 1), IsEmpty());
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "A query was used that the store does not know"));
  }

  TEST_F(FlecsEntityStoreTest, LeavesTheParentOfABlockOutWhenTheOrderIsAny)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{});
    const Entity arm = _store.CreateEntity("arm", player);
    _store.Set(arm, Position{});

    std::size_t visited = 0;
    _store.Each(_store.Query<Position>(), [&](const EntityBlock &block)
    {
      visited += block.count;
      EXPECT_EQ(block.parent, No_Entity);
    });

    EXPECT_EQ(visited, 2u);
  }

  // parents first

  TEST_F(FlecsEntityStoreTest, HandsOverParentsBeforeTheirChildren)
  {
    // created from the bottom up, so that the order of creation does not
    // give the right answer by accident
    const Entity finger = _store.CreateEntity("finger");
    const Entity hand = _store.CreateEntity("hand");
    const Entity arm = _store.CreateEntity("arm");
    const Entity player = _store.CreateEntity("player");
    for (const Entity entity : {finger, hand, arm, player}) { _store.Set(entity, Position{}); }
    _store.SetParent(finger, hand);
    _store.SetParent(hand, arm);
    _store.SetParent(arm, player);

    const auto visited = Visit(_store.Query<Position>(QueryOrder::ParentsFirst));

    EXPECT_THAT(visited, ElementsAre(player, arm, hand, finger));
  }

  TEST_F(FlecsEntityStoreTest, HandsOverEveryParentBeforeItsChildrenInAWideTree)
  {
    const auto query = _store.Query<Position>(QueryOrder::ParentsFirst);

    std::vector<std::pair<Entity, Entity>> child_and_parent;
    std::vector<Entity> parents;
    for (int i = 0; i < 3; i++)
    {
      const Entity top = _store.CreateEntity("");
      _store.Set(top, Position{});
      parents.push_back(top);
    }
    for (int level = 0; level < 3; level++)
    {
      std::vector<Entity> children;
      for (const Entity parent : parents)
      {
        for (int i = 0; i < 2; i++)
        {
          const Entity child = _store.CreateEntity("", parent);
          _store.Set(child, Position{});
          children.push_back(child);
          child_and_parent.emplace_back(child, parent);
        }
      }
      parents = children;
    }

    const auto visited = Visit(query);

    EXPECT_EQ(visited.size(), 3u + 6u + 12u + 24u);
    for (const auto &[child, parent] : child_and_parent)
    {
      EXPECT_LT(IndexOf(visited, parent), IndexOf(visited, child));
    }
  }

  TEST_F(FlecsEntityStoreTest, NamesTheParentOfEveryBlockWhenParentsComeFirst)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity enemy = _store.CreateEntity("enemy");
    const Entity arm = _store.CreateEntity("arm", player);
    const Entity leg = _store.CreateEntity("leg", player);
    const Entity claw = _store.CreateEntity("claw", enemy);
    const Entity hand = _store.CreateEntity("hand", arm);
    for (const Entity entity : {player, enemy, arm, leg, claw, hand}) { _store.Set(entity, Position{}); }

    std::size_t visited = 0;
    _store.Each(_store.Query<Position>(QueryOrder::ParentsFirst), [&](const EntityBlock &block)
    {
      for (std::size_t i = 0; i < block.count; i++)
      {
        visited++;
        EXPECT_EQ(block.parent, _store.GetParent(block.entities[i])) << _store.GetName(block.entities[i]);
      }
    });

    EXPECT_EQ(visited, 6u);
  }

  TEST_F(FlecsEntityStoreTest, HandsOverTheResultOfTheParentToTheChildren)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity arm = _store.CreateEntity("arm", player);
    const Entity hand = _store.CreateEntity("hand", arm);
    _store.Set(player, Position{1.0f, 0.0f});
    _store.Set(arm, Position{10.0f, 0.0f});
    _store.Set(hand, Position{100.0f, 0.0f});

    // every entity adds what its parent worked out to its own
    _store.Each(_store.Query<Position>(QueryOrder::ParentsFirst), [&](const EntityBlock &block)
    {
      const float of_parent = block.parent == No_Entity ? 0.0f : _store.Get<Position>(block.parent)->x;
      for (std::size_t i = 0; i < block.count; i++) { block.Column<Position>(0)[i].x += of_parent; }
    });

    EXPECT_EQ(_store.Get<Position>(player)->x, 1.0f);
    EXPECT_EQ(_store.Get<Position>(arm)->x, 11.0f);
    EXPECT_EQ(_store.Get<Position>(hand)->x, 111.0f);
  }

  TEST_F(FlecsEntityStoreTest, FollowsAnEntityThatIsMovedUnderAnotherParentWhenParentsComeFirst)
  {
    const auto query = _store.Query<Position>(QueryOrder::ParentsFirst);
    const Entity sword = _store.CreateEntity("sword");
    const Entity player = _store.CreateEntity("player");
    _store.Set(sword, Position{});
    _store.Set(player, Position{});
    EXPECT_THAT(Visit(query), UnorderedElementsAre(sword, player));

    _store.SetParent(sword, player);

    EXPECT_THAT(Visit(query), ElementsAre(player, sword));
  }

  TEST_F(FlecsEntityStoreTest, NamesTheNearestEntityAboveThatCarriesTheComponent)
  {
    const Entity world = _store.CreateEntity("world");
    const Entity folder = _store.CreateEntity("folder", world);
    const Entity player = _store.CreateEntity("player", folder);
    _store.Set(world, Position{});
    _store.Set(player, Position{});

    std::vector<std::pair<Entity, Entity>> entity_and_parent;
    _store.Each(_store.Query<Position>(QueryOrder::ParentsFirst), [&](const EntityBlock &block)
    {
      for (std::size_t i = 0; i < block.count; i++) { entity_and_parent.emplace_back(block.entities[i], block.parent); }
    });

    EXPECT_THAT(entity_and_parent, ElementsAre(std::pair{world, No_Entity}, std::pair{player, world}));
  }

  // changes during a query

  TEST_F(FlecsEntityStoreTest, HoldsBackAComponentThatIsSetDuringAQuery)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{});

    _store.Each(_store.Query<Position>(), [&](const EntityBlock &block)
    {
      _store.Set(block.entities[0], Velocity{3.0f, 4.0f});

      EXPECT_FALSE(_store.Has<Velocity>(block.entities[0]));
    });

    ASSERT_TRUE(_store.Has<Velocity>(player));
    EXPECT_EQ(_store.Get<Velocity>(player)->x, 3.0f);
  }

  TEST_F(FlecsEntityStoreTest, HoldsBackAComponentThatIsRemovedDuringAQuery)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{1.0f, 2.0f});
    _store.Set(player, Velocity{});

    _store.Each(_store.Query<Position, Velocity>(), [&](const EntityBlock &block)
    {
      _store.Remove<Velocity>(block.entities[0]);

      EXPECT_TRUE(_store.Has<Velocity>(block.entities[0]));
      EXPECT_EQ(block.Column<Position>(0)[0].x, 1.0f);
    });

    EXPECT_FALSE(_store.Has<Velocity>(player));
    EXPECT_TRUE(_store.Has<Position>(player));
  }

  TEST_F(FlecsEntityStoreTest, HoldsBackAnEntityThatIsDestroyedDuringAQuery)
  {
    const Entity first = _store.CreateEntity("first");
    const Entity second = _store.CreateEntity("second");
    _store.Set(first, Position{1.0f, 0.0f});
    _store.Set(second, Position{2.0f, 0.0f});

    std::vector<Entity> visited;
    _store.Each(_store.Query<Position>(), [&](const EntityBlock &block)
    {
      for (std::size_t i = 0; i < block.count; i++)
      {
        // each destroys the other, and both are still handed over
        _store.DestroyEntity(block.entities[i] == first ? second : first);
        EXPECT_TRUE(_store.IsAlive(first));
        EXPECT_TRUE(_store.IsAlive(second));

        visited.push_back(block.entities[i]);
      }
    });

    EXPECT_THAT(visited, UnorderedElementsAre(first, second));
    EXPECT_FALSE(_store.IsAlive(first));
    EXPECT_FALSE(_store.IsAlive(second));
  }

  TEST_F(FlecsEntityStoreTest, DoesNotHandOverAnEntityThatIsCreatedDuringTheQuery)
  {
    const auto query = _store.Query<Position>();
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{});

    Entity created = No_Entity;
    std::vector<Entity> visited;
    _store.Each(query, [&](const EntityBlock &block)
    {
      for (std::size_t i = 0; i < block.count; i++) { visited.push_back(block.entities[i]); }

      created = _store.CreateEntity("bullet");
      _store.Set(created, Position{5.0f, 6.0f});
    });

    EXPECT_THAT(visited, ElementsAre(player));
    ASSERT_NE(created, No_Entity);
    EXPECT_TRUE(_store.IsAlive(created));
    EXPECT_EQ(_store.FindEntity("bullet"), created);
    ASSERT_TRUE(_store.Has<Position>(created));
    EXPECT_EQ(_store.Get<Position>(created)->x, 5.0f);
    EXPECT_THAT(Visit(query), UnorderedElementsAre(player, created));
  }

  TEST_F(FlecsEntityStoreTest, KeepsTheLastOfSeveralChangesToOneEntityDuringAQuery)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{});

    _store.Each(_store.Query<Position>(), [&](const EntityBlock &block)
    {
      _store.Set(block.entities[0], Velocity{1.0f, 1.0f});
      _store.Set(block.entities[0], Velocity{2.0f, 2.0f});
      _store.Set(block.entities[0], Position{9.0f, 9.0f});
    });

    EXPECT_EQ(_store.Get<Velocity>(player)->x, 2.0f);
    EXPECT_EQ(_store.Get<Position>(player)->x, 9.0f);
  }

  TEST_F(FlecsEntityStoreTest, RunsAQueryInsideAQuery)
  {
    const auto positions = _store.Query<Position>();
    const auto velocities = _store.Query<Velocity>();
    const Entity first = _store.CreateEntity("first");
    const Entity second = _store.CreateEntity("second");
    _store.Set(first, Position{});
    _store.Set(second, Velocity{});

    std::vector<std::pair<Entity, Entity>> pairs;
    _store.Each(positions, [&](const EntityBlock &outer)
    {
      _store.Each(velocities, [&](const EntityBlock &inner)
      {
        pairs.emplace_back(outer.entities[0], inner.entities[0]);
        _store.Set(inner.entities[0], Position{});
      });

      // the outer query is not done yet
      EXPECT_FALSE(_store.Has<Position>(second));
    });

    EXPECT_THAT(pairs, ElementsAre(std::pair{first, second}));
    EXPECT_TRUE(_store.Has<Position>(second));
  }

  TEST_F(FlecsEntityStoreTest, CreatesAQueryDuringAQuery)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{});
    _store.Set(player, Velocity{});

    neon::QueryId created = 0;
    std::size_t visited = 0;
    _store.Each(_store.Query<Position>(), [&](const EntityBlock &block)
    {
      for (int i = 0; i < 20; i++) { created = _store.Query<Velocity>(); }
      visited += block.count;
    });

    EXPECT_EQ(visited, 1u);
    EXPECT_THAT(Visit(created), ElementsAre(player));
  }

  // exceptions

  TEST_F(FlecsEntityStoreTest, LetsAnExceptionFromAQueryThrough)
  {
    _store.Set(_store.CreateEntity("player"), Position{});

    EXPECT_THROW(
      _store.Each(_store.Query<Position>(), [](const EntityBlock &) { throw std::logic_error("broken"); }),
      std::logic_error);
  }

  TEST_F(FlecsEntityStoreTest, WorksAsBeforeAfterAnExceptionFromAQuery)
  {
    const auto query = _store.Query<Position>();
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{});
    const Entity enemy = _store.CreateEntity("enemy");
    _store.Set(enemy, Position{});
    _store.Set(enemy, Velocity{});

    EXPECT_THROW(
      _store.Each(query, [](const EntityBlock &) { throw std::logic_error("broken"); }),
      std::logic_error);

    // nothing is held back any longer
    _store.Set(player, Velocity{1.0f, 2.0f});
    EXPECT_TRUE(_store.Has<Velocity>(player));

    const Entity bullet = _store.CreateEntity("bullet");
    _store.Set(bullet, Position{});
    EXPECT_TRUE(_store.Has<Position>(bullet));

    _store.DestroyEntity(enemy);
    EXPECT_FALSE(_store.IsAlive(enemy));

    EXPECT_THAT(Visit(query), UnorderedElementsAre(player, bullet));
  }

  TEST_F(FlecsEntityStoreTest, AppliesWhatWasChangedBeforeAnExceptionFromAQuery)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{});

    EXPECT_THROW(
      _store.Each(_store.Query<Position>(), [&](const EntityBlock &block)
      {
        _store.Set(block.entities[0], Velocity{1.0f, 2.0f});
        throw std::logic_error("broken");
      }),
      std::logic_error);

    EXPECT_TRUE(_store.Has<Velocity>(player));
  }

  TEST_F(FlecsEntityStoreTest, WorksAsBeforeAfterAnExceptionFromAQueryInsideAQuery)
  {
    const auto query = _store.Query<Position>();
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{});

    EXPECT_THROW(
      _store.Each(query, [&](const EntityBlock &)
      {
        _store.Each(query, [](const EntityBlock &) { throw std::logic_error("broken"); });
      }),
      std::logic_error);

    _store.Set(player, Velocity{});
    EXPECT_TRUE(_store.Has<Velocity>(player));
  }

  // on_remove

  class FlecsEntityStoreRemovalTest : public FlecsEntityStoreTest
  {
  protected:
    std::vector<Removal> _removals;

    void SetUp() override
    {
      FlecsEntityStoreTest::SetUp();
      _store.Register<Health>("Health", [this](const Entity entity, Health &health)
      {
        _removals.push_back({entity, health.points});
      });
    }
  };

  TEST_F(FlecsEntityStoreRemovalTest, IsNotToldWhenAComponentIsSet)
  {
    const Entity player = _store.CreateEntity("player");

    _store.Set(player, Health{50});
    _store.Get<Health>(player)->points = 40;

    EXPECT_THAT(_removals, IsEmpty());
  }

  TEST_F(FlecsEntityStoreRemovalTest, IsToldWhenAComponentIsRemoved)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Health{50});

    _store.Remove<Health>(player);

    EXPECT_THAT(_removals, ElementsAre(Removal{player, 50}));
  }

  TEST_F(FlecsEntityStoreRemovalTest, IsToldOfAComponentThatIsTurnedOffWhenItsEntityIsDestroyed)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Health{50});
    _store.SetEnabled<Health>(player, false);
    EXPECT_THAT(_removals, IsEmpty()) << "turning it off removes nothing";

    // what it holds outside the store is let go of, on or off
    _store.DestroyEntity(player);

    EXPECT_THAT(_removals, ElementsAre(Removal{player, 50}));
  }

  TEST_F(FlecsEntityStoreRemovalTest, IsToldOfAComponentThatIsTurnedOffWhenItIsRemoved)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Health{50});
    _store.SetEnabled<Health>(player, false);

    _store.Remove<Health>(player);

    EXPECT_THAT(_removals, ElementsAre(Removal{player, 50}));
    EXPECT_EQ(_store.GetComponentData(player, _store.IdOf<Health>()), nullptr);

    // and one that is given again is on
    _store.Set(player, Health{10});
    EXPECT_TRUE(_store.IsEnabled<Health>(player));
  }

  TEST_F(FlecsEntityStoreRemovalTest, IsToldOfEveryComponentThatIsTurnedOffWhenTheStoreIsCleanedUp)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity enemy = _store.CreateEntity("enemy");
    _store.Set(player, Health{50});
    _store.Set(enemy, Health{20});
    _store.SetEnabled<Health>(player, false);

    _store.CleanUp();

    EXPECT_THAT(_removals, ::testing::UnorderedElementsAre(Removal{player, 50}, Removal{enemy, 20}));
  }

  TEST_F(FlecsEntityStoreRemovalTest, IsToldTheValueTheComponentHadLast)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Health{50});
    _store.Get<Health>(player)->points = 40;

    _store.Remove<Health>(player);

    EXPECT_THAT(_removals, ElementsAre(Removal{player, 40}));
  }

  TEST_F(FlecsEntityStoreRemovalTest, IsToldOnceWhenAComponentIsRemovedTwice)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Health{50});

    _store.Remove<Health>(player);
    _store.Remove<Health>(player);

    EXPECT_EQ(_removals.size(), 1u);
  }

  TEST_F(FlecsEntityStoreRemovalTest, IsNotToldWhenAnotherComponentIsRemovedOrAdded)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Health{50});

    _store.Set(player, Position{});
    _store.Set(player, Velocity{});
    _store.Remove<Position>(player);

    EXPECT_THAT(_removals, IsEmpty());
    EXPECT_EQ(_store.Get<Health>(player)->points, 50);
  }

  TEST_F(FlecsEntityStoreRemovalTest, IsToldWhenTheEntityIsDestroyed)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Health{50});

    _store.DestroyEntity(player);

    EXPECT_THAT(_removals, ElementsAre(Removal{player, 50}));
  }

  TEST_F(FlecsEntityStoreRemovalTest, IsNotToldWhenAnEntityWithoutTheComponentIsDestroyed)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{});

    _store.DestroyEntity(player);

    EXPECT_THAT(_removals, IsEmpty());
  }

  TEST_F(FlecsEntityStoreRemovalTest, IsToldForEveryChildWhenTheParentIsDestroyed)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity arm = _store.CreateEntity("arm", player);
    const Entity hand = _store.CreateEntity("hand", arm);
    const Entity enemy = _store.CreateEntity("enemy");
    _store.Set(player, Health{1});
    _store.Set(arm, Health{2});
    _store.Set(hand, Health{3});
    _store.Set(enemy, Health{4});

    _store.DestroyEntity(player);

    EXPECT_THAT(_removals, UnorderedElementsAre(Removal{player, 1}, Removal{arm, 2}, Removal{hand, 3}));
  }

  TEST_F(FlecsEntityStoreRemovalTest, IsToldForEveryEntityWhenTheStoreIsCleanedUp)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity arm = _store.CreateEntity("arm", player);
    const Entity enemy = _store.CreateEntity("enemy");
    _store.Set(player, Health{1});
    _store.Set(arm, Health{2});
    _store.Set(enemy, Health{3});
    _store.Set(_store.CreateEntity("rock"), Position{});

    _store.CleanUp();

    EXPECT_THAT(_removals, UnorderedElementsAre(Removal{player, 1}, Removal{arm, 2}, Removal{enemy, 3}));
  }

  TEST_F(FlecsEntityStoreRemovalTest, IsToldOnceWhenTheStoreIsCleanedUpTwice)
  {
    _store.Set(_store.CreateEntity("player"), Health{1});

    _store.CleanUp();
    _store.CleanUp();

    EXPECT_EQ(_removals.size(), 1u);
  }

  TEST(FlecsEntityStore, TellsComponentsThatTheyAreRemovedWhenItIsDestroyed)
  {
    std::vector<Removal> removals;
    Entity player = No_Entity;

    {
      Flecs_EntityStore flecs(std::make_shared<RecordingLogger>());
      EntityStore &store = flecs;
      store.Initialize();
      store.Register<Health>("Health", [&](const Entity entity, Health &health)
      {
        removals.push_back({entity, health.points});
      });
      player = store.CreateEntity("player");
      store.Set(player, Health{50});
    }

    EXPECT_THAT(removals, ElementsAre(Removal{player, 50}));
  }

  TEST_F(FlecsEntityStoreRemovalTest, IsToldAfterTheQueryWhenAComponentIsRemovedDuringIt)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Health{50});

    _store.Each(_store.Query<Health>(), [&](const EntityBlock &block)
    {
      _store.Remove<Health>(block.entities[0]);
      EXPECT_THAT(_removals, IsEmpty());
    });

    EXPECT_THAT(_removals, ElementsAre(Removal{player, 50}));
  }

  TEST_F(FlecsEntityStoreRemovalTest, LetsTheComponentBeChangedWhileItIsToldOfItsRemoval)
  {
    std::vector<int> seen;
    _store.Register<Counted>("Counted", [&](Entity, Counted &counted)
    {
      seen.push_back(counted.value);
      // what a Renderable does with the id of its render object
      counted.value = -1;
    });
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Counted{7});

    _store.Remove<Counted>(player);

    EXPECT_THAT(seen, ElementsAre(7));
  }

  // components that own memory

  class FlecsEntityStoreInventoryTest : public FlecsEntityStoreTest
  {
  protected:
    void SetUp() override
    {
      FlecsEntityStoreTest::SetUp();
      _store.Register<Inventory>("Inventory");
    }

    static Inventory InventoryOf(const int number)
    {
      return {
        long_name + " " + std::to_string(number),
        {long_item + " one " + std::to_string(number), long_item + " two " + std::to_string(number)}
      };
    }

    void ExpectInventoryOf(const Entity entity, const int number)
    {
      const auto *inventory = _store.Get<Inventory>(entity);
      ASSERT_NE(inventory, nullptr) << number;
      EXPECT_EQ(inventory->owner, long_name + " " + std::to_string(number));
      EXPECT_THAT(
        inventory->items,
        ElementsAre(long_item + " one " + std::to_string(number), long_item + " two " + std::to_string(number)));
    }
  };

  TEST_F(FlecsEntityStoreInventoryTest, KeepsACopyThatOwnsItsMemory)
  {
    const Entity player = _store.CreateEntity("player");

    {
      Inventory inventory = InventoryOf(1);
      _store.Set(player, inventory);

      inventory.owner = "changed afterwards, which the store must not see";
      inventory.items.clear();
    }

    ExpectInventoryOf(player, 1);
  }

  TEST_F(FlecsEntityStoreInventoryTest, ReplacesAComponentThatOwnsMemory)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, InventoryOf(1));

    _store.Set(player, InventoryOf(2));

    ExpectInventoryOf(player, 2);
  }

  TEST_F(FlecsEntityStoreInventoryTest, KeepsItWhenTheEntityGainsAndLosesOtherComponents)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, InventoryOf(1));

    _store.Set(player, Position{});
    ExpectInventoryOf(player, 1);

    _store.Set(player, Velocity{});
    ExpectInventoryOf(player, 1);

    _store.Remove<Position>(player);
    ExpectInventoryOf(player, 1);

    _store.Remove<Velocity>(player);
    ExpectInventoryOf(player, 1);
  }

  TEST_F(FlecsEntityStoreInventoryTest, KeepsItWhenTheEntityIsMovedUnderAnotherParent)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity chest = _store.CreateEntity("chest");
    _store.Set(chest, InventoryOf(1));

    _store.SetParent(chest, player);
    ExpectInventoryOf(chest, 1);

    _store.SetParent(chest, No_Entity);
    ExpectInventoryOf(chest, 1);
  }

  TEST_F(FlecsEntityStoreInventoryTest, KeepsItForEveryEntityWhenManyAreCreated)
  {
    std::vector<Entity> entities;
    for (int i = 0; i < 500; i++)
    {
      entities.push_back(_store.CreateEntity(""));
      _store.Set(entities.back(), InventoryOf(i));
    }

    for (int i = 0; i < 500; i++) { ExpectInventoryOf(entities[i], i); }
  }

  TEST_F(FlecsEntityStoreInventoryTest, KeepsItForTheOthersWhenEntitiesAreDestroyedAndChanged)
  {
    std::vector<Entity> entities;
    for (int i = 0; i < 100; i++)
    {
      entities.push_back(_store.CreateEntity(""));
      _store.Set(entities.back(), InventoryOf(i));
    }

    for (int i = 0; i < 100; i++)
    {
      if (i % 3 == 0) { _store.DestroyEntity(entities[i]); }
      if (i % 3 == 1) { _store.Set(entities[i], Position{}); }
    }

    for (int i = 0; i < 100; i++)
    {
      if (i % 3 == 0)
      {
        EXPECT_FALSE(_store.IsAlive(entities[i]));
      } else
      {
        ExpectInventoryOf(entities[i], i);
      }
    }
  }

  TEST_F(FlecsEntityStoreInventoryTest, HandsItOverInAQuery)
  {
    std::vector<Entity> entities;
    for (int i = 0; i < 10; i++)
    {
      entities.push_back(_store.CreateEntity(""));
      _store.Set(entities.back(), InventoryOf(i));
    }

    std::size_t visited = 0;
    _store.Each(_store.Query<Inventory>(), [&](const EntityBlock &block)
    {
      auto *inventories = block.Column<Inventory>(0);
      for (std::size_t i = 0; i < block.count; i++)
      {
        visited++;
        inventories[i].items.push_back(long_item + " three");
      }
    });

    EXPECT_EQ(visited, 10u);
    for (const Entity entity : entities)
    {
      ASSERT_EQ(_store.Get<Inventory>(entity)->items.size(), 3u);
      EXPECT_EQ(_store.Get<Inventory>(entity)->items[2], long_item + " three");
    }
  }

  TEST_F(FlecsEntityStoreInventoryTest, KeepsItWhenItIsSetDuringAQuery)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{});

    _store.Each(_store.Query<Position>(), [&](const EntityBlock &block)
    {
      // the value is gone by the time the change is applied
      const Inventory inventory = InventoryOf(1);
      _store.Set(block.entities[0], inventory);
    });

    ExpectInventoryOf(player, 1);
  }

  // every component that was created is destroyed

  class FlecsEntityStoreCountedTest : public FlecsEntityStoreTest
  {
  protected:
    void SetUp() override
    {
      FlecsEntityStoreTest::SetUp();
      Counted::alive = 0;
      _store.Register<Counted>("Counted");
    }

    void TearDown() override
    {
      _store.CleanUp();
      EXPECT_EQ(Counted::alive, 0);
    }
  };

  TEST_F(FlecsEntityStoreCountedTest, HoldsOneComponentForEveryEntityThatCarriesOne)
  {
    const Entity first = _store.CreateEntity("first");
    const Entity second = _store.CreateEntity("second");
    _store.Set(first, Counted{1});
    _store.Set(second, Counted{2});
    EXPECT_EQ(Counted::alive, 2);

    _store.Set(first, Counted{3});
    EXPECT_EQ(Counted::alive, 2);
    EXPECT_EQ(_store.Get<Counted>(first)->value, 3);
  }

  TEST_F(FlecsEntityStoreCountedTest, DestroysAComponentThatIsRemoved)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Counted{1});

    _store.Remove<Counted>(player);

    EXPECT_EQ(Counted::alive, 0);
  }

  TEST_F(FlecsEntityStoreCountedTest, DestroysTheComponentsOfAnEntityThatIsDestroyedWithItsChildren)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity arm = _store.CreateEntity("arm", player);
    const Entity enemy = _store.CreateEntity("enemy");
    _store.Set(player, Counted{1});
    _store.Set(arm, Counted{2});
    _store.Set(enemy, Counted{3});

    _store.DestroyEntity(player);

    EXPECT_EQ(Counted::alive, 1);
  }

  TEST_F(FlecsEntityStoreCountedTest, HoldsNoMoreWhenEntitiesMoveBetweenTables)
  {
    std::vector<Entity> entities;
    for (int i = 0; i < 50; i++)
    {
      entities.push_back(_store.CreateEntity(""));
      _store.Set(entities.back(), Counted{i});
    }
    for (const Entity entity : entities) { _store.Set(entity, Position{}); }
    for (std::size_t i = 0; i < entities.size(); i += 2) { _store.Remove<Position>(entities[i]); }

    EXPECT_EQ(Counted::alive, 50);
    for (int i = 0; i < 50; i++) { EXPECT_EQ(_store.Get<Counted>(entities[i])->value, i); }
  }

  TEST_F(FlecsEntityStoreCountedTest, DestroysEveryComponentWhenTheStoreIsCleanedUp)
  {
    for (int i = 0; i < 50; i++) { _store.Set(_store.CreateEntity(""), Counted{i}); }
    EXPECT_EQ(Counted::alive, 50);

    _store.CleanUp();

    EXPECT_EQ(Counted::alive, 0);
  }

  TEST_F(FlecsEntityStoreCountedTest, DestroysWhatWasSetDuringAQueryThatThrew)
  {
    const Entity player = _store.CreateEntity("player");
    _store.Set(player, Position{});

    EXPECT_THROW(
      _store.Each(_store.Query<Position>(), [&](const EntityBlock &block)
      {
        _store.Set(block.entities[0], Counted{1});
        throw std::logic_error("broken");
      }),
      std::logic_error);
  }

  // what Flecs reports

  TEST_F(FlecsEntityStoreTest, SaysWhenItStartsAndStops)
  {
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Initializing Flecs"));

    _store.CleanUp();

    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Cleaning up Flecs"));
  }

  TEST_F(FlecsEntityStoreTest, ReportsNoErrorInOrdinaryUse)
  {
    const Entity player = _store.CreateEntity("player");
    const Entity arm = _store.CreateEntity("arm.left", player);
    _store.Set(player, Position{});
    _store.Set(arm, Position{});
    Visit(_store.Query<Position>(QueryOrder::ParentsFirst));
    _store.SetParent(arm, No_Entity);
    _store.DestroyEntity(player);
    _store.CleanUp();

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 0u) << _logger->Messages(LogLevel::Warn);
  }
}
