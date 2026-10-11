#include "pool-system.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/pool-manager.hpp>
#include <neon/world-system/ecs/components/pool-instance.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>

namespace
{
  using neon::DataValue;
  using neon::Entity;
  using neon::EntityStore;
  using neon::No_Entity;
  using neon::PoolManager;
  using neon::PoolInstance;
  using neon::PoolSystem;
  using neon::Renderable;
  using neon::Transform;
  using neon::testing::FakeEntityStore;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;
  using ::testing::ElementsAre;

  /// A world that spawns an entity with a Renderable and a child for every
  /// prefab it is asked for, and keeps what it was asked.
  class SpawningWorld final : public neon::WorldSystem
  {
    EntityStore *_store = nullptr;

  public:
    std::vector<std::string> spawned;

    /// How many spawns there are left before it spawns nothing.
    int room = 1000;

    SpawningWorld(EntityStore *store, const std::shared_ptr<neon::Logger> &logger)
      : WorldSystem(nullptr, nullptr, nullptr, logger), _store(store) {}

    ~SpawningWorld() = default;

    void Initialize() override {}

    void Update() override {}

    void CleanUp() override {}

    Entity Spawn(const std::string &path, const Entity parent, const DataValue &) override
    {
      if (room-- <= 0) { return No_Entity; }

      spawned.push_back(path);
      const Entity entity = _store->CreateEntity("", parent);
      _store->Set(entity, Transform{});
      _store->Set(entity, Renderable{});
      const Entity tip = _store->CreateEntity("tip", entity);
      _store->Set(tip, Renderable{});
      return entity;
    }
  };

  class PoolSystemTest : public ::testing::Test
  {
  protected:
    static constexpr const char *nail = "assets://prefabs/nail.prefab.yml";

    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    FakeEntityStore _store;
    SpawningWorld _world{&_store, _logger};
    PoolSystem _system{&_world, _logger};

    void SetUp() override
    {
      _store.Initialize();
      _store.Register<Transform>("Transform");
      _store.Register<Renderable>("Renderable");
      _system.Register(_store);
      _system.Initialize(_store);
    }

    void TearDown() override
    {
      _store.CleanUp();
    }

    /// A pool that is to hold these, filled.
    Entity MakePool(const std::vector<neon::PoolEntry> &entries)
    {
      const Entity pool = _store.CreateEntity("projectiles");
      PoolManager held;
      held.entries = entries;
      _store.Set(pool, held);
      _system.Update(_store, 0.016);
      return pool;
    }

    Entity Acquire(const Entity pool, const std::string &kind)
    {
      return PoolSystem::Acquire(_store, pool, kind);
    }
  };

  TEST_F(PoolSystemTest, SpawnsAsManyInstancesOfAPrefabAsItsEntrySaysBelowThePoolAndOnce)
  {
    const Entity pool = MakePool({{nail, 3}});
    _system.Update(_store, 0.016);

    EXPECT_THAT(_world.spawned, ElementsAre(nail, nail, nail));
    EXPECT_THAT(PoolSystem::GetKinds(_store, pool), ElementsAre(nail));
    EXPECT_EQ(PoolSystem::GetCount(_store, pool, nail), 3u);
    EXPECT_EQ(PoolSystem::GetFreeCount(_store, pool, nail), 3u);
    EXPECT_EQ(_store.GetChildren(pool).size(), 3u);
  }

  TEST_F(PoolSystemTest, NumbersTheInstancesSinceANameIsAnAddress)
  {
    const Entity pool = MakePool({{nail, 2}});

    std::vector<std::string> names;
    for (const Entity instance : _store.GetChildren(pool)) { names.push_back(_store.GetName(instance)); }
    EXPECT_THAT(names, ::testing::UnorderedElementsAre("nail 1", "nail 2"));
  }

  TEST_F(PoolSystemTest, WhatWaitsIsTurnedOffWithEverythingBelowIt)
  {
    const Entity pool = MakePool({{nail, 1}});
    const Entity waiting = _store.GetChildren(pool).at(0);
    const Entity tip = _store.GetChildren(waiting).at(0);

    EXPECT_FALSE(_store.IsEnabled<Renderable>(waiting));
    EXPECT_FALSE(_store.IsEnabled<Renderable>(tip));
    EXPECT_TRUE(_store.IsEnabled<Transform>(waiting)) << "it can be placed while it waits";
  }

  TEST_F(PoolSystemTest, HandsOutAnInstanceTurnedOnAndTakesItBackTurnedOff)
  {
    const Entity pool = MakePool({{nail, 2}});

    const Entity first = Acquire(pool, nail);
    ASSERT_NE(first, No_Entity);
    EXPECT_TRUE(_store.IsEnabled<Renderable>(first));
    EXPECT_TRUE(_store.IsEnabled<Renderable>(_store.GetChildren(first).at(0)));
    EXPECT_EQ(PoolSystem::GetFreeCount(_store, pool, nail), 1u);

    EXPECT_TRUE(PoolSystem::Release(_store, first));
    EXPECT_FALSE(_store.IsEnabled<Renderable>(first));
    EXPECT_EQ(PoolSystem::GetFreeCount(_store, pool, nail), 2u);

    // given back twice, it waits once
    EXPECT_FALSE(PoolSystem::Release(_store, first));
    EXPECT_EQ(PoolSystem::GetFreeCount(_store, pool, nail), 2u);
  }

  TEST_F(PoolSystemTest, IsAQueueWhatCameBackFirstIsHandedOutFirst)
  {
    const Entity pool = MakePool({{nail, 3}});
    const Entity a = Acquire(pool, nail);
    const Entity b = Acquire(pool, nail);
    const Entity c = Acquire(pool, nail);

    PoolSystem::Release(_store, b);
    PoolSystem::Release(_store, a);
    PoolSystem::Release(_store, c);

    EXPECT_EQ(Acquire(pool, nail), b);
    EXPECT_EQ(Acquire(pool, nail), a);
    EXPECT_EQ(Acquire(pool, nail), c);
  }

  TEST_F(PoolSystemTest, HandsOutNoneWhenAllAreOutAndSaysSoOnceUntilOneComesBack)
  {
    const Entity pool = MakePool({{nail, 1}});
    const Entity only = Acquire(pool, nail);
    ASSERT_NE(only, No_Entity);

    EXPECT_EQ(Acquire(pool, nail), No_Entity);
    EXPECT_EQ(Acquire(pool, nail), No_Entity);
    EXPECT_EQ(_world.spawned.size(), 1u) << "it never makes more than it was told to hold";

    // said by the system, whoever asked, and once
    _system.Update(_store, 0.016);
    EXPECT_EQ(Acquire(pool, nail), No_Entity);
    _system.Update(_store, 0.016);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 1u);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Warn,
      "The pool 'projectiles' ran out of assets://prefabs/nail.prefab.yml: all 1 are out, and it hands out none "
      "until one is given back. It holds as many as it was told to")) << _logger->Messages(LogLevel::Warn);

    PoolSystem::Release(_store, only);
    EXPECT_EQ(Acquire(pool, nail), only);
    EXPECT_EQ(Acquire(pool, nail), No_Entity);
    _system.Update(_store, 0.016);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 2u) << "said again, since one had come back";
  }

  TEST_F(PoolSystemTest, HoldsSeveralKindsApart)
  {
    const Entity pool = MakePool({{nail, 1}, {"assets://prefabs/rocket.prefab.yml", 2}});

    EXPECT_THAT(PoolSystem::GetKinds(_store, pool), ElementsAre(nail, "assets://prefabs/rocket.prefab.yml"));
    EXPECT_NE(Acquire(pool, nail), No_Entity);
    EXPECT_EQ(Acquire(pool, nail), No_Entity);
    EXPECT_NE(Acquire(pool, "assets://prefabs/rocket.prefab.yml"), No_Entity);
    EXPECT_EQ(PoolSystem::GetFreeCount(_store, pool, "assets://prefabs/rocket.prefab.yml"), 1u);
  }

  TEST_F(PoolSystemTest, SaysThatItHoldsNoSuchKind)
  {
    const Entity pool = MakePool({{nail, 1}});

    EXPECT_EQ(Acquire(pool, "assets://prefabs/bomb.prefab.yml"), No_Entity);
    EXPECT_EQ(Acquire(pool, "assets://prefabs/bomb.prefab.yml"), No_Entity);
    _system.Update(_store, 0.016);
    _system.Update(_store, 0.016);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 1u);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Warn, "The pool 'projectiles' was asked for assets://prefabs/bomb.prefab.yml, which it does not hold"));
    EXPECT_EQ(PoolSystem::GetCount(_store, pool, "assets://prefabs/bomb.prefab.yml"), 0u);
  }

  TEST_F(PoolSystemTest, TakesInInstancesThatWereMadeInCodeUnderAName)
  {
    const Entity pool = MakePool({});
    std::vector<Entity> made;
    for (int i = 0; i < 3; i++)
    {
      made.push_back(_store.CreateEntity("made " + std::to_string(i)));
      _store.Set(made.back(), Renderable{});
    }

    ASSERT_TRUE(PoolSystem::Add(_store, pool, "instance://nail", made, _logger.get()));

    EXPECT_THAT(PoolSystem::GetKinds(_store, pool), ElementsAre("instance://nail"));
    EXPECT_EQ(PoolSystem::GetFreeCount(_store, pool, "instance://nail"), 3u);
    EXPECT_FALSE(_store.IsEnabled<Renderable>(made[0]));
    EXPECT_EQ(_store.GetName(made[0]), "made 0") << "they keep the names they were made with";
    EXPECT_EQ(Acquire(pool, "instance://nail"), made[0]);

    // more of a kind it has are taken on top
    const Entity one_more = _store.CreateEntity("made 3");
    ASSERT_TRUE(PoolSystem::Add(_store, pool, "instance://nail", {one_more}, _logger.get()));
    EXPECT_EQ(PoolSystem::GetCount(_store, pool, "instance://nail"), 4u);
  }

  TEST_F(PoolSystemTest, DoesNotTakeInWhatIsGoneOrBelongsToAPoolAlready)
  {
    const Entity pool = MakePool({});
    const Entity made = _store.CreateEntity("made");
    const Entity gone = _store.CreateEntity("gone");
    _store.DestroyEntity(gone);

    ASSERT_TRUE(PoolSystem::Add(_store, pool, "instance://nail", {made, gone, made}, _logger.get()));

    EXPECT_EQ(PoolSystem::GetCount(_store, pool, "instance://nail"), 1u);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 2u);

    // and what is no pool takes nothing
    EXPECT_FALSE(PoolSystem::Add(_store, made, "instance://nail", {}, _logger.get()));
    EXPECT_FALSE(PoolSystem::Add(_store, pool, "", {}, _logger.get()));
    EXPECT_FALSE(PoolSystem::Release(_store, pool));
  }

  TEST_F(PoolSystemTest, TakesAnEntityThatIsThereAsTheOneInstanceOfItsPath)
  {
    const Entity armory = _store.CreateEntity("armory");
    const Entity lamp = _store.CreateEntity("lamp", armory);
    _store.Set(lamp, Renderable{});

    const Entity pool = MakePool({{"instance://armory/lamp", 1}});

    EXPECT_TRUE(_world.spawned.empty());
    EXPECT_EQ(PoolSystem::GetCount(_store, pool, "instance://armory/lamp"), 1u);
    EXPECT_FALSE(_store.IsEnabled<Renderable>(lamp));
    EXPECT_EQ(Acquire(pool, "instance://armory/lamp"), lamp);
  }

  TEST_F(PoolSystemTest, SaysThatMoreThanOneOfAnEntityThatIsThereTakesCloning)
  {
    const Entity lamp = _store.CreateEntity("lamp");
    const Entity pool = MakePool({{"instance://lamp", 4}, {"instance://nothing", 1}});

    EXPECT_EQ(PoolSystem::GetCount(_store, pool, "instance://lamp"), 1u);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 2u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "The pool 'projectiles' finds no entity 'nothing' for instance://nothing"));
    EXPECT_EQ(Acquire(pool, "instance://lamp"), lamp);
  }

  TEST_F(PoolSystemTest, SaysWhenAPrefabCouldNotBeSpawnedAsOftenAsAsked)
  {
    _world.room = 2;
    const Entity pool = MakePool({{nail, 5}});

    EXPECT_EQ(PoolSystem::GetCount(_store, pool, nail), 2u);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The pool 'projectiles' holds 2 of assets://prefabs/nail.prefab.yml and not 5: the prefab could not be spawned"));
  }

  TEST_F(PoolSystemTest, PassesOverAnInstanceThatWasDestroyedWhileItWaited)
  {
    const Entity pool = MakePool({{nail, 2}});
    const std::vector<Entity> waiting = _store.GetChildren(pool);
    const Entity first = Acquire(pool, nail);
    PoolSystem::Release(_store, first);

    // the one at the front of the queue goes
    const Entity front = waiting[0] == first ? waiting[1] : waiting[0];
    _store.DestroyEntity(front);

    EXPECT_EQ(Acquire(pool, nail), first);
    EXPECT_EQ(Acquire(pool, nail), No_Entity);
  }

  TEST_F(PoolSystemTest, TurnsTheBodyOffBeforeItsColliderAndOnAfterIt)
  {
    struct Body
    {
      int unused = 0;
    };
    struct Shape
    {
      int unused = 0;
    };

    std::vector<std::string> order;
    _store.Register<Body>("RigidBody", {}, [&](Entity, Body &, const bool on) { order.push_back(on ? "body on" : "body off"); });
    _store.Register<Shape>("Collider", {}, [&](Entity, Shape &, const bool on) { order.push_back(on ? "shape on" : "shape off"); });
    const Entity crate = _store.CreateEntity("crate");
    _store.Set(crate, Body{});
    _store.Set(crate, Shape{});

    PoolSystem::SetInUse(_store, crate, false);
    PoolSystem::SetInUse(_store, crate, true);

    // so that a body that is out of the physics keeps its shapes
    EXPECT_THAT(order, ElementsAre("body off", "shape off", "shape on", "body on"));
  }
}
