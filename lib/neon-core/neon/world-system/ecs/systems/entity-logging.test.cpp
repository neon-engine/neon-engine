#include "entity-logging.hpp"

#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/fake-physics-context.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/character-body.hpp>
#include <neon/world-system/ecs/components/rigid-body.hpp>

namespace
{
  using neon::BodyId;
  using neon::BodyInfo;
  using neon::CharacterBody;
  using neon::CharacterId;
  using neon::CharacterInfo;
  using neon::Entity;
  using neon::EntityLogging;
  using neon::RigidBody;
  using neon::Transform;
  using neon::testing::FakeEntityStore;
  using neon::testing::FakePhysicsContext;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;

  /// A transform that was placed in the world at a position.
  Transform PlacedAt(const glm::vec3 &position)
  {
    Transform transform;
    transform.position = position;
    transform.world_coordinates[3] = glm::vec4(position, 1.0f);
    return transform;
  }

  class EntityLoggingTest : public ::testing::Test
  {
  protected:
    FakeEntityStore _store;
    FakePhysicsContext _physics;
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();

    void SetUp() override
    {
      _store.Initialize();
      _store.Register<Transform>("Transform");
      _store.Register<RigidBody>("RigidBody");
      _store.Register<CharacterBody>("CharacterBody");
    }

    /// A system that logs these paths, initialized on the store.
    std::unique_ptr<EntityLogging> Logging(std::vector<std::string> paths, neon::PhysicsContext *physics)
    {
      auto system = std::make_unique<EntityLogging>(std::move(paths), physics, _logger);
      system->Initialize(_store);
      return system;
    }

    Entity CreateAt(const std::string &name, const glm::vec3 &position, const Entity parent = neon::No_Entity)
    {
      const Entity entity = _store.CreateEntity(name, parent);
      _store.Set(entity, PlacedAt(position));
      return entity;
    }
  };

  TEST_F(EntityLoggingTest, LogsWhereAnEntityIsInEveryFrame)
  {
    CreateAt("player", {0.0f, 0.9f, 7.5f});
    const auto system = Logging({"player"}, &_physics);

    system->Update(_store, 0.016);
    system->Update(_store, 0.016);

    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Frame 1: player is at [0.000, 0.900, 7.500]"));
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Frame 2: player is at [0.000, 0.900, 7.500]"));
    EXPECT_EQ(_logger->Count(LogLevel::Info), 2u);
  }

  TEST_F(EntityLoggingTest, FindsAnEntityByItsPathFromTheTop)
  {
    const Entity crates = CreateAt("crates", {10.0f, 0.0f, 0.0f});
    CreateAt("upper", {10.0f, 3.5f, 0.0f}, crates);
    // the same name at the top is another entity
    CreateAt("upper", {-1.0f, -1.0f, -1.0f});
    const auto system = Logging({"crates/upper"}, &_physics);

    system->Update(_store, 0.016);

    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Frame 1: crates/upper is at [10.000, 3.500, 0.000]"));
  }

  TEST_F(EntityLoggingTest, LogsEveryEntityThatIsNamedInTheOrderGiven)
  {
    CreateAt("player", {1.0f, 2.0f, 3.0f});
    CreateAt("walker", {-1.0f, 1.0f, 4.0f});
    const auto system = Logging({"walker", "player"}, &_physics);

    system->Update(_store, 0.016);

    ASSERT_EQ(_logger->Entries().size(), 2u);
    EXPECT_EQ(_logger->Entries()[0].message, "Frame 1: walker is at [-1.000, 1.000, 4.000]");
    EXPECT_EQ(_logger->Entries()[1].message, "Frame 1: player is at [1.000, 2.000, 3.000]");
  }

  TEST_F(EntityLoggingTest, LogsWhereThePhysicsHasARigidBody)
  {
    const Entity crate = CreateAt("crate", {0.0f, 2.0f, 0.0f});
    BodyId body = neon::No_Body;
    std::string error;
    ASSERT_TRUE(_physics.CreateBody(BodyInfo{.position = {0.0f, 1.5f, 0.0f}}, body, error));
    _store.Set(crate, RigidBody{.body = body});
    const auto system = Logging({"crate"}, &_physics);

    system->Update(_store, 0.016);

    EXPECT_TRUE(_logger->Contains(
      LogLevel::Info, "Frame 1: crate is at [0.000, 2.000, 0.000], its body at [0.000, 1.500, 0.000]"));
  }

  TEST_F(EntityLoggingTest, LogsWhereThePhysicsHasACharacter)
  {
    const Entity player = CreateAt("player", {0.0f, 0.9f, 7.5f});
    CharacterId character = neon::No_Character;
    std::string error;
    ASSERT_TRUE(_physics.CreateCharacter(CharacterInfo{.position = {0.0f, 0.9f, 7.0f}}, character, error));
    _store.Set(player, CharacterBody{.character = character});
    const auto system = Logging({"player"}, &_physics);

    system->Update(_store, 0.016);

    EXPECT_TRUE(_logger->Contains(
      LogLevel::Info, "Frame 1: player is at [0.000, 0.900, 7.500], its body at [0.000, 0.900, 7.000]"));
  }

  TEST_F(EntityLoggingTest, LogsNoBodyBeforeThePhysicsHasMadeIt)
  {
    const Entity crate = CreateAt("crate", {0.0f, 2.0f, 0.0f});
    _store.Set(crate, RigidBody{});
    const auto system = Logging({"crate"}, &_physics);

    system->Update(_store, 0.016);

    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Frame 1: crate is at [0.000, 2.000, 0.000]"));
    EXPECT_FALSE(_logger->Contains(LogLevel::Info, "its body"));
  }

  TEST_F(EntityLoggingTest, LogsOnlyWhereTheEntityIsDrawnWithoutPhysics)
  {
    const Entity crate = CreateAt("crate", {0.0f, 2.0f, 0.0f});
    _store.Set(crate, RigidBody{.body = 1});
    const auto system = Logging({"crate"}, nullptr);

    system->Update(_store, 0.016);

    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Frame 1: crate is at [0.000, 2.000, 0.000]"));
    EXPECT_FALSE(_logger->Contains(LogLevel::Info, "its body"));
  }

  TEST_F(EntityLoggingTest, SaysOnceThatThereIsNoSuchEntity)
  {
    const auto system = Logging({"ghost"}, &_physics);

    system->Update(_store, 0.016);
    system->Update(_store, 0.016);
    system->Update(_store, 0.016);

    EXPECT_EQ(_logger->Count(LogLevel::Warn), 1u);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Warn, "Frame 1: there is no entity ghost to log the place of. It is logged once it is there"));
    EXPECT_EQ(_logger->Count(LogLevel::Info), 0u);
  }

  TEST_F(EntityLoggingTest, LogsAnEntityFromTheFrameItIsThereAndSaysAgainWhenItIsGone)
  {
    const auto system = Logging({"target"}, &_physics);

    system->Update(_store, 0.016);
    const Entity target = CreateAt("target", {1.0f, 1.0f, 1.0f});
    system->Update(_store, 0.016);
    _store.DestroyEntity(target);
    system->Update(_store, 0.016);
    system->Update(_store, 0.016);

    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "Frame 1: there is no entity target"));
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Frame 2: target is at [1.000, 1.000, 1.000]"));
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "Frame 3: there is no entity target"));
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 2u);
  }

  TEST_F(EntityLoggingTest, SaysOnceThatAnEntityHasNoPlace)
  {
    _store.CreateEntity("music");
    const auto system = Logging({"music"}, &_physics);

    system->Update(_store, 0.016);
    system->Update(_store, 0.016);

    EXPECT_EQ(_logger->Count(LogLevel::Warn), 1u);
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "Frame 1: the entity music has no Transform, so it has no place to log"));
    EXPECT_EQ(_logger->Count(LogLevel::Info), 0u);
  }

  TEST_F(EntityLoggingTest, LogsAWorldWithoutPhysicsComponents)
  {
    FakeEntityStore store;
    store.Initialize();
    store.Register<Transform>("Transform");
    const Entity player = store.CreateEntity("player");
    store.Set(player, PlacedAt({1.0f, 0.0f, 0.0f}));
    EntityLogging system({"player"}, &_physics, _logger);
    system.Initialize(store);

    system.Update(store, 0.016);

    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Frame 1: player is at [1.000, 0.000, 0.000]"));
  }
} // namespace
