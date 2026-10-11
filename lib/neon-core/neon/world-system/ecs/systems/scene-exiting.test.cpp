#include "scene-exiting.hpp"

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/mock-world-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/scene-exit.hpp>
#include <neon/world-system/ecs/components/trigger.hpp>

namespace
{
  using neon::Entity;
  using neon::SceneExit;
  using neon::SceneExiting;
  using neon::Trigger;
  using neon::testing::FakeEntityStore;
  using neon::testing::MockWorldSystem;
  using neon::testing::RecordingLogger;
  using ::testing::NiceMock;

  class SceneExitingTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    FakeEntityStore _store;
    NiceMock<MockWorldSystem> _world{_logger};
    SceneExiting _system{&_world};

    void SetUp() override
    {
      _store.Initialize();
      _store.Register<SceneExit>("SceneExit");
    }

    void TearDown() override
    {
      _store.CleanUp();
    }
  };

  TEST_F(SceneExitingTest, AsksForTheSceneOfAnExitWhoseTriggerHasABodyInside)
  {
    // the physics registers Trigger; here a test does
    _store.Register<Trigger>("Trigger");
    _system.Initialize(_store);
    const Entity door = _store.CreateEntity("door");
    _store.Set(door, Trigger{});
    _store.Set(door, SceneExit{.scene = "assets://scenes/end.scene.yml"});

    EXPECT_CALL(_world, LoadScene(::testing::_)).Times(0);
    _system.Update(_store, 0.016);

    EXPECT_CALL(_world, LoadScene("assets://scenes/end.scene.yml"));
    _store.Get<Trigger>(door)->inside = 1;
    _system.Update(_store, 0.016);
  }

  TEST_F(SceneExitingTest, AnExitWithoutATriggerOrWithoutASceneAsksForNothing)
  {
    _store.Register<Trigger>("Trigger");
    _system.Initialize(_store);
    const Entity no_trigger = _store.CreateEntity("no-trigger");
    _store.Set(no_trigger, SceneExit{.scene = "assets://scenes/end.scene.yml"});
    const Entity no_scene = _store.CreateEntity("no-scene");
    _store.Set(no_scene, Trigger{.inside = 1});
    _store.Set(no_scene, SceneExit{});

    EXPECT_CALL(_world, LoadScene(::testing::_)).Times(0);
    _system.Update(_store, 0.016);
  }

  TEST_F(SceneExitingTest, HasExitsThatNeverOpenWithoutPhysics)
  {
    // nothing registered Trigger, as in a world without a physics system
    _system.Initialize(_store);
    const Entity door = _store.CreateEntity("door");
    _store.Set(door, SceneExit{.scene = "assets://scenes/end.scene.yml"});

    EXPECT_CALL(_world, LoadScene(::testing::_)).Times(0);
    _system.Update(_store, 0.016);
  }
}
