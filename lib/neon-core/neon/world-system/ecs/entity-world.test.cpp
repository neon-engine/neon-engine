#include "entity-world.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/data/data-value.hpp>
#include <neon/data/document-format.hpp>
#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/mock-entity-world.hpp>
#include <neon/testing/mock-file-system-context.hpp>
#include <neon/testing/mock-input-system.hpp>
#include <neon/testing/mock-render-pipeline.hpp>
#include <neon/testing/mock-window-system.hpp>
#include <neon/testing/recording-logger.hpp>

#include "components/camera.hpp"
#include "components/geometry.hpp"
#include "components/light.hpp"
#include "components/prefab.hpp"
#include "components/renderable.hpp"
#include "components/persistent.hpp"
#include "components/first-person-controller.hpp"
#include "components/scene-exit.hpp"
#include "components/trigger.hpp"
#include "components/spectator.hpp"
#include "scene-file/scene-file.hpp"

namespace
{
  using neon::Camera;
  using neon::DataValue;
  using neon::FirstPersonController;
  using neon::Entity;
  using neon::EntityStore;
  using neon::EntitySystem;
  using neon::EntityWorld;
  using neon::Geometry;
  using neon::Light;
  using neon::No_Entity;
  using neon::Persistent;
  using neon::Prefab;
  using neon::SceneFile;
  using neon::SceneExit;
  using neon::Trigger;
  using neon::No_Component;
  using neon::Renderable;
  using neon::Spectator;
  using neon::Transform;
  using neon::testing::FakeEntityStore;
  using neon::testing::FakeInputContext;
  using neon::testing::LogLevel;
  using neon::testing::MockEntitySystem;
  using neon::testing::MockFileSystemContext;
  using neon::testing::MockRenderPipeline;
  using neon::testing::MockScene;
  using neon::testing::MockWindowContext;
  using neon::testing::RecordingLogger;
  using ::testing::_;
  using ::testing::ElementsAre;
  using ::testing::InSequence;
  using ::testing::NiceMock;
  using ::testing::Ref;
  using ::testing::Return;
  using ::testing::StrictMock;

  /// A system of a game that writes down when it is called, and does what a
  /// test tells it to.
  class RecordingSystem final : public EntitySystem
  {
    std::string _name;
    std::vector<std::string> *_calls;
    std::function<void(EntityStore &, double)> _on_update;

  public:
    RecordingSystem(
      const std::string &name,
      std::vector<std::string> *calls,
      const std::function<void(EntityStore &, double)> &on_update = {})
    {
      _name = name;
      _calls = calls;
      _on_update = on_update;
    }

    void Register(EntityStore &) override
    {
      _calls->push_back(_name + " registered");
    }

    void Initialize(EntityStore &) override
    {
      _calls->push_back(_name + " initialized");
    }

    void Update(EntityStore &store, const double delta_time) override
    {
      _calls->push_back(_name + " updated");
      if (_on_update) { _on_update(store, delta_time); }
    }
  };

  class EntityWorldTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    FakeEntityStore _store;
    NiceMock<MockScene> _scene;
    NiceMock<MockRenderPipeline> _pipeline{_logger};
    NiceMock<FakeInputContext> _input{_logger};
    NiceMock<MockWindowContext> _window;
    EntityWorld _world{&_store, &_scene, &_pipeline, &_input, &_window, _logger};

    std::vector<std::string> _calls;

    void SetUp() override
    {
      ON_CALL(_window, GetDeltaTime()).WillByDefault(Return(0.5));
    }

    void TearDown() override
    {
      // The world has to be cleaned up before it goes away, as an application
      // does. The store tells a Renderable that it is removed through a
      // function that belongs to the world, and would call it when the store
      // itself goes away, after the world.
      _world.CleanUp();
    }

    /// Lets the scene create what a test asks for.
    void PopulateWith(const std::function<void(EntityStore &)> &populate)
    {
      ON_CALL(_scene, Populate(_)).WillByDefault([populate](EntityStore &store)
      {
        populate(store);
        return true;
      });
    }
  };

  // Initialize

  TEST_F(EntityWorldTest, TouchesNothingWhenItIsCreated)
  {
    StrictMock<MockScene> scene;
    StrictMock<MockRenderPipeline> pipeline{_logger};
    StrictMock<FakeInputContext> input{_logger};
    StrictMock<MockWindowContext> window;
    FakeEntityStore store;

    const EntityWorld world(&store, &scene, &pipeline, &input, &window, _logger);

    EXPECT_EQ(store.InitializeCount(), 0);
  }

  TEST_F(EntityWorldTest, InitializesTheStore)
  {
    _world.Initialize();

    EXPECT_EQ(_store.InitializeCount(), 1);
    EXPECT_EQ(_store.CleanUpCount(), 0);
  }

  TEST_F(EntityWorldTest, RegistersTheComponentsOfTheEngine)
  {
    _world.Initialize();

    for (const char *name : {"Transform", "Camera", "Light", "Spectator", "FirstPersonController", "Renderable", "Persistent", "SceneExit", "Geometry"})
    {
      EXPECT_NE(_store.FindComponent(name), No_Component) << name;
    }
    EXPECT_EQ(_store.IdOf<Transform>(), _store.FindComponent("Transform"));
    EXPECT_EQ(_store.IdOf<Camera>(), _store.FindComponent("Camera"));
    EXPECT_EQ(_store.IdOf<Light>(), _store.FindComponent("Light"));
    EXPECT_EQ(_store.IdOf<Spectator>(), _store.FindComponent("Spectator"));
    EXPECT_EQ(_store.IdOf<FirstPersonController>(), _store.FindComponent("FirstPersonController"));
    EXPECT_EQ(_store.IdOf<Renderable>(), _store.FindComponent("Renderable"));
  }

  TEST_F(EntityWorldTest, GoesOnWithWhatCouldBeReadWhenTheSceneHasProblems)
  {
    EXPECT_CALL(_scene, Populate(_)).WillOnce(::testing::Return(false));

    _world.Initialize();

    EXPECT_TRUE(_logger->Contains(
      neon::testing::LogLevel::Error, "The world runs with what could be read of its scene"))
      << _logger->Messages(neon::testing::LogLevel::Error);
  }

  TEST_F(EntityWorldTest, HasASceneWhenItsSceneHasProblems)
  {
    EXPECT_CALL(_scene, Populate(_)).WillOnce(::testing::Return(false));

    _world.Initialize();

    EXPECT_FALSE(_world.HasNoScene());
  }

  TEST_F(EntityWorldTest, HasNoSceneWhenItsSceneCannotBeReadAtAll)
  {
    EXPECT_CALL(_scene, Populate(_)).WillOnce(::testing::Return(false));
    _scene.could_not_be_read = true;

    _world.Initialize();

    EXPECT_TRUE(_world.HasNoScene());
    EXPECT_FALSE(_logger->Contains(neon::testing::LogLevel::Error, "The world runs with what could be read"))
      << "the scene said why, and the world does not run";
  }

  TEST_F(EntityWorldTest, PopulatesTheSceneOnceWithTheStore)
  {
    EXPECT_CALL(_scene, Populate(Ref(_store))).Times(1);

    _world.Initialize();
    _world.Update();
    _world.Update();
  }

  TEST_F(EntityWorldTest, PopulatesTheSceneAfterTheComponentsOfTheEngineAreRegistered)
  {
    bool populated = false;
    PopulateWith([&](EntityStore &store)
    {
      populated = true;
      EXPECT_NE(store.FindComponent("Transform"), No_Component);
      EXPECT_NE(store.FindComponent("Renderable"), No_Component);

      // what every scene does
      const Entity entity = store.CreateEntity("cube");
      store.Set(entity, Transform{});
      store.Set(entity, Renderable{});
      store.Set(entity, Camera{});
      store.Set(entity, Light{});
      store.Set(entity, Spectator{});
    });

    _world.Initialize();

    EXPECT_TRUE(populated);
    EXPECT_TRUE(_store.IsAlive(_store.FindEntity("cube")));
  }

  TEST_F(EntityWorldTest, InitializesTheSystemsOfTheGameBeforeTheSceneIsPopulated)
  {
    _world.AddSystem(std::make_unique<RecordingSystem>("first", &_calls));
    _world.AddSystem(std::make_unique<RecordingSystem>("second", &_calls));
    PopulateWith([&](EntityStore &) { _calls.emplace_back("scene populated"); });

    _world.Initialize();

    EXPECT_THAT(_calls, ElementsAre(
                  "first registered", "second registered", "first initialized", "second initialized",
                  "scene populated"));
  }

  TEST_F(EntityWorldTest, RegistersTheComponentsOfEverySystemBeforeAnyIsInitialized)
  {
    // a system of the game that queries a component another system brings,
    // one that is added after placing and so initialized after it
    _world.AddSystem(std::make_unique<RecordingSystem>("game", &_calls));
    _world.AddSystemAfterPlacing(std::make_unique<RecordingSystem>("after placing", &_calls));

    _world.Initialize();

    EXPECT_THAT(_calls, ElementsAre(
                  "game registered", "after placing registered", "game initialized", "after placing initialized"));
  }

  TEST_F(EntityWorldTest, InitializesASystemAddedAfterPlacingBeforeTheSceneIsPopulated)
  {
    _world.AddSystem(std::make_unique<RecordingSystem>("game", &_calls));
    _world.AddSystemAfterPlacing(std::make_unique<RecordingSystem>("after placing", &_calls));
    PopulateWith([&](EntityStore &) { _calls.emplace_back("scene populated"); });

    _world.Initialize();

    // it can register its components, as the scene may use them
    EXPECT_THAT(_calls, ElementsAre(
                  "game registered", "after placing registered", "game initialized", "after placing initialized",
                  "scene populated"));
  }

  TEST_F(EntityWorldTest, InitializesASystemOfTheGameWithTheStoreAndTheComponentsOfTheEngine)
  {
    auto system = std::make_unique<StrictMock<MockEntitySystem>>();
    EXPECT_CALL(*system, Initialize(Ref(_store))).WillOnce([](EntityStore &store)
    {
      // the place to create queries, which needs the components
      EXPECT_NO_THROW((void) (store.Query<Transform, Renderable>()));
    });
    _world.AddSystem(std::move(system));

    _world.Initialize();
  }

  // changing the scene

  TEST_F(EntityWorldTest, IsNotChangingSceneToBeginWith)
  {
    _world.Initialize();
    EXPECT_FALSE(_world.IsChangingScene());
  }

  TEST_F(EntityWorldTest, ReadsTheSceneAskedForAtTheStartOfTheNextUpdateAndNotBefore)
  {
    PopulateWith([](EntityStore &) {});
    _world.Initialize();
    EXPECT_CALL(_scene, Load(_, _)).Times(0);

    _world.LoadScene("assets://scenes/next.scene.yml");
    EXPECT_TRUE(_world.IsChangingScene());
    ::testing::Mock::VerifyAndClearExpectations(&_scene);

    {
      InSequence in_order;
      EXPECT_CALL(_scene, Load(Ref(_store), "assets://scenes/next.scene.yml")).WillOnce(Return(true));
      EXPECT_CALL(_pipeline, RenderFrame());
    }
    _world.Update();

    EXPECT_FALSE(_world.IsChangingScene());
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Changing the scene to assets://scenes/next.scene.yml"));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(EntityWorldTest, DestroysWhatDoesNotStayAndKeepsWhatIsPersistentWithItsChildren)
  {
    Entity level = 0, crate = 0, player = 0, camera = 0;
    PopulateWith([&](EntityStore &store)
    {
      level = store.CreateEntity("level");
      crate = store.CreateEntity("crate", level);
      player = store.CreateEntity("player");
      store.Set(player, Persistent{});
      camera = store.CreateEntity("camera", player);
    });
    _world.Initialize();
    ON_CALL(_scene, Load(_, _)).WillByDefault(Return(true));

    _world.LoadScene("assets://scenes/next.scene.yml");
    _world.Update();

    EXPECT_FALSE(_store.IsAlive(level));
    EXPECT_FALSE(_store.IsAlive(crate));
    EXPECT_TRUE(_store.IsAlive(player));
    EXPECT_TRUE(_store.IsAlive(camera));
  }

  TEST_F(EntityWorldTest, AsksTheRendererToFreeWhatNothingShowsOnceASceneTookThePlaceOfAnother)
  {
    _world.Initialize();
    ON_CALL(_scene, Load(_, _)).WillByDefault(Return(true));

    // not while one scene is shown
    EXPECT_CALL(_pipeline, FreeUnused()).Times(0);
    _world.Update();
    ::testing::Mock::VerifyAndClearExpectations(&_pipeline);

    // after the first frame of the next one, so that what both show stays
    _world.LoadScene("assets://scenes/next.scene.yml");
    {
      ::testing::InSequence in_order;
      EXPECT_CALL(_pipeline, RenderFrame());
      EXPECT_CALL(_pipeline, FreeUnused());
    }
    _world.Update();
    ::testing::Mock::VerifyAndClearExpectations(&_pipeline);

    // and once
    EXPECT_CALL(_pipeline, FreeUnused()).Times(0);
    _world.Update();
  }

  TEST_F(EntityWorldTest, RefusesASceneWithoutAPath)
  {
    _world.Initialize();
    EXPECT_CALL(_scene, Load(_, _)).Times(0);

    _world.LoadScene("");
    _world.Update();

    EXPECT_FALSE(_world.IsChangingScene());
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "A scene without a path was asked for, nothing changes"));
  }

  TEST_F(EntityWorldTest, TakesTheLastSceneAskedForInAFrame)
  {
    _world.Initialize();
    EXPECT_CALL(_scene, Load(_, "assets://scenes/second.scene.yml")).WillOnce(Return(true));

    _world.LoadScene("assets://scenes/first.scene.yml");
    _world.LoadScene("assets://scenes/second.scene.yml");
    _world.Update();
  }

  TEST_F(EntityWorldTest, GoesOnWithWhatCouldBeReadWhenTheNextSceneHasProblems)
  {
    _world.Initialize();
    EXPECT_CALL(_scene, Load(_, _)).WillOnce(Return(false));

    _world.LoadScene("assets://scenes/next.scene.yml");
    _world.Update();

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "The world runs with what could be read of the scene assets://scenes/next.scene.yml"));
  }

  TEST_F(EntityWorldTest, StaysWhereItIsWhenTheNextSceneCannotBeReadAtAll)
  {
    Entity level = 0;
    PopulateWith([&](EntityStore &store) { level = store.CreateEntity("level"); });
    _world.Initialize();
    _scene.can_read_ahead = false;
    EXPECT_CALL(_scene, Load(_, _)).Times(0);
    EXPECT_CALL(_pipeline, FreeUnused()).Times(0);

    _world.LoadScene("assets://scenes/missing.scene.yml");
    _world.Update();

    EXPECT_TRUE(_store.IsAlive(level)) << "nothing is destroyed for a scene that cannot be read";
    EXPECT_FALSE(_world.IsChangingScene());
    EXPECT_FALSE(_world.HasNoScene()) << "the game goes on in the scene it is in";
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The scene assets://scenes/missing.scene.yml cannot be read, the game stays in the scene it is in"));
  }

  TEST_F(EntityWorldTest, AsksForTheSceneOfAnExitWhoseTriggerHasABodyInside)
  {
    // the physics registers Trigger; here a test does
    _store.Register<Trigger>("Trigger");
    Entity door = 0;
    PopulateWith([&](EntityStore &store)
    {
      door = store.CreateEntity("door");
      store.Set(door, Trigger{});
      store.Set(door, SceneExit{.scene = "assets://scenes/end.scene.yml"});
    });
    _world.Initialize();

    _world.Update();
    EXPECT_FALSE(_world.IsChangingScene());

    _store.Get<Trigger>(door)->inside = 1;
    _world.Update();
    EXPECT_TRUE(_world.IsChangingScene());
  }

  TEST_F(EntityWorldTest, AnExitWithoutATriggerOrWithoutASceneAsksForNothing)
  {
    _store.Register<Trigger>("Trigger");
    PopulateWith([&](EntityStore &store)
    {
      const Entity no_trigger = store.CreateEntity("no-trigger");
      store.Set(no_trigger, SceneExit{.scene = "assets://scenes/end.scene.yml"});

      const Entity no_scene = store.CreateEntity("no-scene");
      store.Set(no_scene, Trigger{.inside = 1});
      store.Set(no_scene, SceneExit{});
    });
    _world.Initialize();

    _world.Update();
    EXPECT_FALSE(_world.IsChangingScene());
  }

  TEST_F(EntityWorldTest, HasExitsThatNeverOpenWithoutPhysics)
  {
    // nothing registered Trigger, as in a world without a physics system
    PopulateWith([&](EntityStore &store)
    {
      const Entity door = store.CreateEntity("door");
      store.Set(door, SceneExit{.scene = "assets://scenes/end.scene.yml"});
    });
    _world.Initialize();

    _world.Update();
    EXPECT_FALSE(_world.IsChangingScene());
  }

  // pausing

  TEST_F(EntityWorldTest, IsNotPausedToBeginWith)
  {
    EXPECT_FALSE(_world.IsPaused());
  }

  TEST_F(EntityWorldTest, HoldsTheSystemsOfTheGameStillWhilePausedAndStillDraws)
  {
    _world.AddSystem(std::make_unique<RecordingSystem>("game", &_calls));
    _world.Initialize();
    _calls.clear();

    // drawn while paused, and drawn again afterwards
    EXPECT_CALL(_pipeline, RenderFrame()).Times(2);

    _world.SetPaused(true);
    EXPECT_TRUE(_world.IsPaused());
    _world.Update();

    EXPECT_TRUE(_calls.empty()) << ::testing::PrintToString(_calls);

    _world.SetPaused(false);
    _world.Update();
    EXPECT_EQ(_calls, std::vector<std::string>{"game updated"});
  }

  TEST_F(EntityWorldTest, UpdatesASystemAddedAfterPlacingWhilePausedWithNoTimePassing)
  {
    std::vector<double> times;
    _world.AddSystemAfterPlacing(std::make_unique<RecordingSystem>("after placing", &_calls, [&](EntityStore &, const double delta_time)
    {
      times.push_back(delta_time);
    }));
    _world.Initialize();

    _world.SetPaused(true);
    _world.Update();
    _world.SetPaused(false);
    _world.Update();

    EXPECT_THAT(times, ElementsAre(0.0, 0.5));
  }

  TEST_F(EntityWorldTest, HidesTheCursorWhenTheWorldIsThere)
  {
    EXPECT_CALL(_input, CenterAndHideCursor()).Times(1);

    _world.Initialize();
  }

  TEST_F(EntityWorldTest, DrawsNothingWhenItIsInitialized)
  {
    StrictMock<MockRenderPipeline> pipeline{_logger};
    PopulateWith([](EntityStore &store)
    {
      const Entity cube = store.CreateEntity("cube");
      store.Set(cube, Transform{});
      store.Set(cube, Renderable{});
    });
    EntityWorld world(&_store, &_scene, &pipeline, &_input, &_window, _logger);

    world.Initialize();

    world.CleanUp();
  }

  TEST_F(EntityWorldTest, SaysWhenItStartsAndStops)
  {
    _world.Initialize();
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Initializing the world"));
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Initialized the world!"));

    _world.CleanUp();
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Cleaning up the world"));
  }

  // Update

  TEST_F(EntityWorldTest, UpdatesTheSystemsOfTheGameInTheOrderTheyWereAdded)
  {
    _world.AddSystem(std::make_unique<RecordingSystem>("first", &_calls));
    _world.AddSystem(std::make_unique<RecordingSystem>("second", &_calls));
    _world.AddSystem(std::make_unique<RecordingSystem>("third", &_calls));
    _world.Initialize();
    _calls.clear();

    _world.Update();
    _world.Update();

    EXPECT_THAT(
      _calls,
      ElementsAre(
        "first updated",
        "second updated",
        "third updated",
        "first updated",
        "second updated",
        "third updated"));
  }

  TEST_F(EntityWorldTest, HandsTheTimeOfTheWindowToTheSystems)
  {
    auto system = std::make_unique<NiceMock<MockEntitySystem>>();
    EXPECT_CALL(_window, GetDeltaTime()).WillOnce(Return(0.25)).WillOnce(Return(0.125));
    {
      InSequence in_order;
      EXPECT_CALL(*system, Update(Ref(_store), 0.25));
      EXPECT_CALL(*system, Update(Ref(_store), 0.125));
    }
    _world.AddSystem(std::move(system));
    _world.Initialize();

    _world.Update();
    _world.Update();
  }

  TEST_F(EntityWorldTest, AsksTheWindowForTheTimeOnceInAFrame)
  {
    _world.AddSystem(std::make_unique<RecordingSystem>("first", &_calls));
    _world.AddSystem(std::make_unique<RecordingSystem>("second", &_calls));
    _world.Initialize();

    // a window measures the time since it was asked last
    EXPECT_CALL(_window, GetDeltaTime()).Times(1).WillOnce(Return(0.5));

    _world.Update();
  }

  TEST_F(EntityWorldTest, MovesTheSpectatorBeforeTheSystemsOfTheGameRun)
  {
    PopulateWith([](EntityStore &store)
    {
      const Entity spectator = store.CreateEntity("spectator");
      store.Set(spectator, Transform{});
      store.Set(spectator, Spectator{.move_speed = 2.0f});
    });
    glm::vec3 seen{0.0f};
    _world.AddSystem(std::make_unique<RecordingSystem>("game", &_calls, [&](EntityStore &store, double)
    {
      seen = store.Get<Transform>(store.FindEntity("spectator"))->position;
    }));
    _world.Initialize();
    _input.state.SetKeyDown(neon::Key::W);
    _input.Refresh();

    _world.Update();

    // 2 units per second for half a second
    EXPECT_EQ(seen, glm::vec3(0.0f, 0.0f, -1.0f));
  }

  TEST_F(EntityWorldTest, PlacesEntitiesInTheWorldAfterTheSystemsOfTheGameRan)
  {
    PopulateWith([](EntityStore &store)
    {
      store.Set(store.CreateEntity("cube"), Transform{});
    });
    glm::mat4 seen{0.0f};
    _world.AddSystem(std::make_unique<RecordingSystem>("game", &_calls, [&](EntityStore &store, double)
    {
      auto *transform = store.Get<Transform>(store.FindEntity("cube"));
      seen = transform->world_coordinates;
      transform->position.x += 5.0f;
    }));
    _world.Initialize();

    _world.Update();

    // the system saw where the cube was before, and the cube is where the
    // system moved it to
    EXPECT_EQ(seen, glm::mat4(1.0f));
    const auto &transform = *_store.Get<Transform>(_store.FindEntity("cube"));
    EXPECT_EQ(glm::vec3(transform.world_coordinates[3]), glm::vec3(5.0f, 0.0f, 0.0f));
  }

  TEST_F(EntityWorldTest, UpdatesTheSystemsAddedAfterPlacingAfterTheSystemsOfTheGame)
  {
    // added first, and still updated last
    _world.AddSystemAfterPlacing(std::make_unique<RecordingSystem>("first after placing", &_calls));
    _world.AddSystemAfterPlacing(std::make_unique<RecordingSystem>("second after placing", &_calls));
    _world.AddSystem(std::make_unique<RecordingSystem>("game", &_calls));
    _world.Initialize();
    _calls.clear();

    _world.Update();

    EXPECT_THAT(_calls, ElementsAre("game updated", "first after placing updated", "second after placing updated"));
  }

  TEST_F(EntityWorldTest, ShowsASystemAddedAfterPlacingEveryEntityWhereItIsInTheFirstFrame)
  {
    PopulateWith([](EntityStore &store)
    {
      const Entity parent = store.CreateEntity("player");
      store.Set(parent, Transform{.position = {0.0f, 0.0f, 2.0f}});
      const Entity child = store.CreateEntity("camera", parent);
      store.Set(child, Transform{.position = {0.0f, 1.0f, 0.0f}});
    });
    glm::vec3 seen{0.0f};
    _world.AddSystemAfterPlacing(std::make_unique<RecordingSystem>("after placing", &_calls, [&](EntityStore &store, double)
    {
      seen = store.Get<Transform>(store.FindEntity("player/camera"))->world_coordinates[3];
    }));
    _world.Initialize();

    _world.Update();

    // a system of the game would still see the camera at the origin here,
    // which is where a sound was heard in the first frame
    EXPECT_EQ(seen, glm::vec3(0.0f, 1.0f, 2.0f));
  }

  TEST_F(EntityWorldTest, DrawsAnEntityWhereASystemOfTheGameMovedItTo)
  {
    PopulateWith([](EntityStore &store)
    {
      const Entity cube = store.CreateEntity("cube");
      store.Set(cube, Transform{});
      Renderable renderable;
      renderable.render_info.model_path = "assets://models/cube.obj";
      store.Set(cube, renderable);
    });
    _world.AddSystem(std::make_unique<RecordingSystem>("game", &_calls, [](EntityStore &store, double)
    {
      store.Get<Transform>(store.FindEntity("cube"))->position = {1.0f, 2.0f, 3.0f};
    }));
    _world.Initialize();

    Transform drawn;
    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(4));
    EXPECT_CALL(_pipeline, EnqueueForRendering(4, _)).WillOnce(::testing::SaveArg<1>(&drawn));

    _world.Update();

    EXPECT_EQ(glm::vec3(drawn.world_coordinates[3]), glm::vec3(1.0f, 2.0f, 3.0f));
  }

  TEST_F(EntityWorldTest, DrawsTheGeometryOfAnEntityASystemOfTheGameSpawnedInTheSameFrame)
  {
    _world.AddSystem(std::make_unique<RecordingSystem>("game", &_calls, [](EntityStore &store, double)
    {
      if (store.FindEntity("crate") != No_Entity) { return; }

      const Entity crate = store.CreateEntity("crate");
      store.Set(crate, Transform{});
      store.Set(crate, Geometry{});
      store.Set(crate, Renderable{});
    }));
    _world.Initialize();

    neon::RenderInfo created;
    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(::testing::DoAll(::testing::SaveArg<0>(&created), Return(3)));
    EXPECT_CALL(_pipeline, EnqueueForRendering(3, _));

    _world.Update();

    ASSERT_NE(created.mesh, nullptr);
    EXPECT_FALSE(created.mesh->IsEmpty());
    EXPECT_FALSE(created.mesh_key.empty());
  }

  TEST_F(EntityWorldTest, RendersTheFrameAfterEverythingWasHandedOver)
  {
    PopulateWith([](EntityStore &store)
    {
      const Entity entity = store.CreateEntity("lamp");
      store.Set(entity, Transform{});
      store.Set(entity, Renderable{});
      store.Set(entity, Light{});
      store.Set(entity, Camera{});
    });
    _world.AddSystem(std::make_unique<RecordingSystem>("game", &_calls, [this](EntityStore &, double)
    {
      ::testing::Mock::VerifyAndClearExpectations(&_pipeline);

      InSequence in_order;
      EXPECT_CALL(_pipeline, SetCameraInfo(_));
      EXPECT_CALL(_pipeline, EnqueueLightSource(_));
      EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(1));
      EXPECT_CALL(_pipeline, EnqueueForRendering(1, _));
      EXPECT_CALL(_pipeline, RenderFrame());
    }));
    _world.Initialize();

    // nothing reaches the pipeline before the systems of the game ran
    EXPECT_CALL(_pipeline, SetCameraInfo(_)).Times(0);
    EXPECT_CALL(_pipeline, EnqueueLightSource(_)).Times(0);
    EXPECT_CALL(_pipeline, EnqueueForRendering(_, _)).Times(0);
    EXPECT_CALL(_pipeline, RenderFrame()).Times(0);

    _world.Update();

    EXPECT_THAT(_calls, ::testing::Contains("game updated"));
  }

  TEST_F(EntityWorldTest, RendersOneFrameForEveryUpdate)
  {
    _world.Initialize();

    EXPECT_CALL(_pipeline, RenderFrame()).Times(3);

    _world.Update();
    _world.Update();
    _world.Update();
  }

  TEST_F(EntityWorldTest, RendersAFrameOfAnEmptyWorld)
  {
    StrictMock<MockRenderPipeline> pipeline{_logger};
    EntityWorld world(&_store, &_scene, &pipeline, &_input, &_window, _logger);
    world.Initialize();

    // an empty world still has a time, which the shaders are told
    EXPECT_CALL(pipeline, SetTime(_, _));
    EXPECT_CALL(pipeline, RenderFrame());

    world.Update();

    world.CleanUp();
  }

  // CleanUp

  TEST_F(EntityWorldTest, CleansUpTheStore)
  {
    PopulateWith([](EntityStore &store) { store.CreateEntity("cube"); });
    _world.Initialize();

    _world.CleanUp();

    EXPECT_EQ(_store.CleanUpCount(), 1);
    EXPECT_EQ(_store.EntityCount(), 0u);
  }

  TEST_F(EntityWorldTest, CleansUpOnceWhenAskedTwice)
  {
    _world.Initialize();

    _world.CleanUp();
    _world.CleanUp();

    EXPECT_EQ(_store.CleanUpCount(), 1);
  }

  TEST_F(EntityWorldTest, CleansUpWhenItIsDestroyed)
  {
    {
      EntityWorld world(&_store, &_scene, &_pipeline, &_input, &_window, _logger);
      world.Initialize();
    }

    EXPECT_EQ(_store.CleanUpCount(), 1);
  }

  TEST_F(EntityWorldTest, DoesNotCleanUpWhatWasNeverInitialized)
  {
    _world.CleanUp();

    EXPECT_EQ(_store.CleanUpCount(), 0);
  }

  TEST_F(EntityWorldTest, CanBeInitializedAgainAfterItWasCleanedUp)
  {
    EXPECT_CALL(_scene, Populate(_)).Times(2);

    _world.Initialize();
    _world.CleanUp();
    _world.Initialize();
    _world.Update();
    _world.CleanUp();

    EXPECT_EQ(_store.InitializeCount(), 2);
    EXPECT_EQ(_store.CleanUpCount(), 2);
  }

  TEST_F(EntityWorldTest, ReleasesTheRenderObjectsOfEverythingThatWasDrawnWhenCleanedUp)
  {
    PopulateWith([](EntityStore &store)
    {
      for (const char *name : {"cube", "sphere"})
      {
        const Entity entity = store.CreateEntity(name);
        store.Set(entity, Transform{});
        store.Set(entity, Renderable{});
      }
    });
    _world.Initialize();
    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(4)).WillOnce(Return(5));
    _world.Update();

    EXPECT_CALL(_pipeline, DestroyRenderObject(4));
    EXPECT_CALL(_pipeline, DestroyRenderObject(5));

    _world.CleanUp();
  }

  TEST_F(EntityWorldTest, ReleasesNothingForWhatWasNeverDrawn)
  {
    PopulateWith([](EntityStore &store)
    {
      const Entity entity = store.CreateEntity("cube");
      store.Set(entity, Transform{});
      store.Set(entity, Renderable{});
    });
    _world.Initialize();

    EXPECT_CALL(_pipeline, DestroyRenderObject(_)).Times(0);

    _world.CleanUp();
  }

  TEST_F(EntityWorldTest, ReleasesTheRenderObjectOfAnEntityWhoseRenderableIsTurnedOffWhenItIsDestroyed)
  {
    PopulateWith([](EntityStore &store)
    {
      const Entity entity = store.CreateEntity("cube");
      store.Set(entity, Transform{});
      store.Set(entity, Renderable{});
    });
    _world.Initialize();
    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(4));
    _world.Update();

    // turned off, it keeps its render object
    EXPECT_CALL(_pipeline, DestroyRenderObject(_)).Times(0);
    _store.SetEnabled<Renderable>(_store.FindEntity("cube"), false);
    _world.Update();
    ::testing::Mock::VerifyAndClearExpectations(&_pipeline);

    // and it goes with the entity all the same
    EXPECT_CALL(_pipeline, DestroyRenderObject(4)).Times(1);
    _store.DestroyEntity(_store.FindEntity("cube"));
    ::testing::Mock::VerifyAndClearExpectations(&_pipeline);

    EXPECT_CALL(_pipeline, DestroyRenderObject(_)).Times(0);
    _world.CleanUp();
  }

  TEST_F(EntityWorldTest, ReleasesTheRenderObjectsOfWhatIsTurnedOffWhenTheSceneChangesAndWhenTheWorldIsCleanedUp)
  {
    PopulateWith([](EntityStore &store)
    {
      for (const char *name : {"cube", "sphere"})
      {
        const Entity entity = store.CreateEntity(name);
        store.Set(entity, Transform{});
        store.Set(entity, Renderable{});
      }
      store.Set(store.FindEntity("sphere"), Persistent{});
    });
    _world.Initialize();
    ON_CALL(_scene, Load(_, _)).WillByDefault(Return(true));
    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(4)).WillOnce(Return(5));
    _world.Update();
    const int of_cube = _store.Get<Renderable>(_store.FindEntity("cube"))->render_object_id;
    const int of_sphere = _store.Get<Renderable>(_store.FindEntity("sphere"))->render_object_id;

    _store.SetEnabled<Renderable>(_store.FindEntity("cube"), false);
    _store.SetEnabled<Renderable>(_store.FindEntity("sphere"), false);

    // the scene goes: what does not stay is let go of, though it is off
    EXPECT_CALL(_pipeline, DestroyRenderObject(of_cube)).Times(1);
    EXPECT_CALL(_pipeline, DestroyRenderObject(of_sphere)).Times(0);
    _world.LoadScene("assets://scenes/next.scene.yml");
    _world.Update();
    ::testing::Mock::VerifyAndClearExpectations(&_pipeline);

    // and what stayed is let go of when the world is cleaned up
    EXPECT_CALL(_pipeline, DestroyRenderObject(of_sphere)).Times(1);
    _world.CleanUp();
  }

  TEST_F(EntityWorldTest, ReleasesTheRenderObjectOfAnEntityThatIsDestroyed)
  {
    PopulateWith([](EntityStore &store)
    {
      const Entity entity = store.CreateEntity("cube");
      store.Set(entity, Transform{});
      store.Set(entity, Renderable{});
    });
    _world.Initialize();
    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(4));
    _world.Update();

    EXPECT_CALL(_pipeline, DestroyRenderObject(4)).Times(1);
    _store.DestroyEntity(_store.FindEntity("cube"));
    ::testing::Mock::VerifyAndClearExpectations(&_pipeline);

    // and not again when the world is cleaned up
    EXPECT_CALL(_pipeline, DestroyRenderObject(_)).Times(0);
    _world.CleanUp();
  }

  TEST_F(EntityWorldTest, ReleasesTheRenderObjectOfAnEntityThatStopsBeingVisible)
  {
    PopulateWith([](EntityStore &store)
    {
      const Entity entity = store.CreateEntity("cube");
      store.Set(entity, Transform{});
      store.Set(entity, Renderable{});
    });
    _world.Initialize();
    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillOnce(Return(4));
    _world.Update();

    EXPECT_CALL(_pipeline, DestroyRenderObject(4)).Times(1);
    EXPECT_CALL(_pipeline, EnqueueForRendering(_, _)).Times(0);

    _store.Remove<Renderable>(_store.FindEntity("cube"));
    _world.Update();
  }

  TEST_F(EntityWorldTest, ReleasesNothingForAnEntityTheRendererCouldNotCreate)
  {
    PopulateWith([](EntityStore &store)
    {
      const Entity entity = store.CreateEntity("cube");
      store.Set(entity, Transform{});
      store.Set(entity, Renderable{});
    });
    _world.Initialize();
    EXPECT_CALL(_pipeline, CreateRenderObject(_)).WillRepeatedly(Return(-1));
    _world.Update();

    EXPECT_CALL(_pipeline, DestroyRenderObject(_)).Times(0);

    _world.CleanUp();
  }

  // steps of the world

  /// A system that writes down every call with what it was handed.
  class SteppingSystem final : public EntitySystem
  {
    std::string _name;
    std::vector<std::string> *_calls;

  public:
    /// What the system knows of its calls once the world owns it.
    struct Record
    {
      std::vector<double> frame_times;
      std::vector<double> step_times;
      std::vector<double> blends;
    };

    Record *record = nullptr;

    SteppingSystem(const std::string &name, std::vector<std::string> *calls, Record *record = nullptr)
    {
      _name = name;
      _calls = calls;
      this->record = record;
    }

    void Initialize(EntityStore &) override {}

    void Update(EntityStore &, const double delta_time) override
    {
      _calls->push_back(_name + " updated");
      if (record != nullptr) { record->frame_times.push_back(delta_time); }
    }

    void FixedUpdate(EntityStore &, const double fixed_delta_time) override
    {
      _calls->push_back(_name + " stepped");
      if (record != nullptr) { record->step_times.push_back(fixed_delta_time); }
    }

    void Interpolate(EntityStore &, const double blend) override
    {
      _calls->push_back(_name + " interpolated");
      if (record != nullptr) { record->blends.push_back(blend); }
    }
  };

  TEST_F(EntityWorldTest, StepsBeforeTheFrameIsUpdatedAndInterpolatesAfter)
  {
    ON_CALL(_window, GetDeltaTime()).WillByDefault(Return(1.0 / 30.0));
    _world.AddSystem(std::make_unique<SteppingSystem>("first", &_calls));
    _world.AddSystem(std::make_unique<SteppingSystem>("second", &_calls));
    _world.Initialize();

    _world.Update();

    EXPECT_THAT(_calls, ElementsAre(
                  "first stepped", "second stepped",
                  "first stepped", "second stepped",
                  "first updated", "second updated",
                  "first interpolated", "second interpolated"));
  }

  TEST_F(EntityWorldTest, StepsByTheLengthOfAStepWhateverTheFrameTook)
  {
    SteppingSystem::Record record;
    _world.AddSystem(std::make_unique<SteppingSystem>("game", &_calls, &record));
    _world.Initialize();

    for (const double frame : {0.001, 0.02, 0.1, 0.0167, 0.05})
    {
      ON_CALL(_window, GetDeltaTime()).WillByDefault(Return(frame));
      _world.Update();
    }

    EXPECT_THAT(record.frame_times, ElementsAre(0.001, 0.02, 0.1, 0.0167, 0.05));
    ASSERT_FALSE(record.step_times.empty());
    for (const double step : record.step_times) { EXPECT_EQ(step, 1.0 / 60.0); }
  }

  TEST_F(EntityWorldTest, TakesTheStepsOfTheTimeThatPassedAtEveryFrameRate)
  {
    for (const int frames_per_second : {30, 60, 144, 1000})
    {
      FakeEntityStore store;
      EntityWorld world{&store, &_scene, &_pipeline, &_input, &_window, _logger};
      SteppingSystem::Record record;
      world.AddSystem(std::make_unique<SteppingSystem>("game", &_calls, &record));
      world.Initialize();
      ON_CALL(_window, GetDeltaTime()).WillByDefault(Return(1.0 / frames_per_second));

      for (int frame = 0; frame < 2 * frames_per_second; frame++) { world.Update(); }

      EXPECT_EQ(record.step_times.size(), 120u) << frames_per_second << " frames per second";
      EXPECT_EQ(record.frame_times.size(), static_cast<std::size_t>(2 * frames_per_second));
      world.CleanUp();
    }
  }

  TEST_F(EntityWorldTest, UpdatesAFrameThatTakesNoStep)
  {
    ON_CALL(_window, GetDeltaTime()).WillByDefault(Return(0.001));
    _world.AddSystem(std::make_unique<SteppingSystem>("game", &_calls));
    _world.Initialize();

    _world.Update();

    EXPECT_THAT(_calls, ElementsAre("game updated", "game interpolated"));
  }

  TEST_F(EntityWorldTest, TakesNoMoreThanTheMostStepsForAFrameThatTookLong)
  {
    ON_CALL(_window, GetDeltaTime()).WillByDefault(Return(10.0));
    SteppingSystem::Record record;
    _world.AddSystem(std::make_unique<SteppingSystem>("game", &_calls, &record));
    _world.Initialize();

    _world.Update();

    EXPECT_EQ(record.step_times.size(), 8u);
    EXPECT_EQ(record.frame_times.size(), 1u);

    // and the frame after it does not make up for it
    ON_CALL(_window, GetDeltaTime()).WillByDefault(Return(1.0 / 60.0));
    _world.Update();

    EXPECT_EQ(record.step_times.size(), 9u);
  }

  TEST_F(EntityWorldTest, StepsAsOftenAsItsClockIsSetTo)
  {
    ON_CALL(_window, GetDeltaTime()).WillByDefault(Return(0.1));
    SteppingSystem::Record record;
    _world.AddSystem(std::make_unique<SteppingSystem>("game", &_calls, &record));
    _world.GetFixedClock().SetStepsPerSecond(30.0);
    _world.GetFixedClock().SetMostStepsPerFrame(2);
    _world.Initialize();

    _world.Update();

    EXPECT_THAT(record.step_times, ElementsAre(1.0 / 30.0, 1.0 / 30.0));
  }

  TEST_F(EntityWorldTest, HandsTheBlendOfItsClockToInterpolate)
  {
    SteppingSystem::Record record;
    _world.AddSystem(std::make_unique<SteppingSystem>("game", &_calls, &record));
    _world.Initialize();

    ON_CALL(_window, GetDeltaTime()).WillByDefault(Return(0.25 / 60.0));
    _world.Update();
    ON_CALL(_window, GetDeltaTime()).WillByDefault(Return(1.0 / 60.0));
    _world.Update();

    ASSERT_EQ(record.blends.size(), 2u);
    EXPECT_NEAR(record.blends[0], 0.25, 1e-9);
    EXPECT_NEAR(record.blends[1], 0.25, 1e-9);
    EXPECT_EQ(record.blends[1], _world.GetFixedClock().GetBlend());
  }

  TEST_F(EntityWorldTest, DrawsWhatInterpolatePlaced)
  {
    /// Places what is drawn somewhere else than where the entity is.
    class Placing final : public EntitySystem
    {
    public:
      void Initialize(EntityStore &) override {}

      void Update(EntityStore &, double) override {}

      void Interpolate(EntityStore &store, double) override
      {
        store.Get<Transform>(store.FindEntity("cube"))->world_coordinates[3] = {7.0f, 8.0f, 9.0f, 1.0f};
      }
    };

    PopulateWith([](EntityStore &store)
    {
      const Entity entity = store.CreateEntity("cube");
      store.Set(entity, Transform{.position = {1.0f, 2.0f, 3.0f}});
      store.Set(entity, Renderable{});
    });
    _world.AddSystem(std::make_unique<Placing>());
    _world.Initialize();
    ON_CALL(_pipeline, CreateRenderObject(_)).WillByDefault(Return(4));

    glm::vec3 drawn_at{0.0f};
    EXPECT_CALL(_pipeline, EnqueueForRendering(4, _)).WillOnce([&](int, const Transform &transform)
    {
      drawn_at = transform.world_coordinates[3];
    });
    _world.Update();

    EXPECT_EQ(drawn_at, glm::vec3(7.0f, 8.0f, 9.0f));
    // where the entity is has not changed
    EXPECT_EQ(_store.Get<Transform>(_store.FindEntity("cube"))->position, glm::vec3(1.0f, 2.0f, 3.0f));
  }

  TEST_F(EntityWorldTest, LeavesASystemWithoutStepsAsItWas)
  {
    ON_CALL(_window, GetDeltaTime()).WillByDefault(Return(1.0 / 30.0));
    _world.AddSystem(std::make_unique<RecordingSystem>("game", &_calls));
    _world.Initialize();

    _world.Update();

    EXPECT_THAT(_calls, ElementsAre("game registered", "game initialized", "game updated"));
  }

  // spawning

  TEST_F(EntityWorldTest, SpawnsAPrefabThroughTheSceneBelowTheParentWithTheOverrides)
  {
    Entity room = 0;
    PopulateWith([&](EntityStore &store) { room = store.CreateEntity("room"); });
    _world.Initialize();

    auto overrides = DataValue::Map();
    overrides.Set("Transform", DataValue::Text("Default"));
    EXPECT_CALL(_scene, Spawn(Ref(_store), "assets://prefabs/wall.prefab.yml", room, ::testing::Truly(
                  [](const DataValue &value) { return value.Find("Transform") != nullptr; })))
      .WillOnce(Return(42));

    EXPECT_EQ(_world.Spawn("assets://prefabs/wall.prefab.yml", room, overrides), 42u);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(EntityWorldTest, SpawnsNothingBeforeItIsInitialized)
  {
    EXPECT_CALL(_scene, Spawn(_, _, _, _)).Times(0);

    EXPECT_EQ(_world.Spawn("assets://prefabs/wall.prefab.yml", No_Entity, DataValue{}), No_Entity);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The prefab assets://prefabs/wall.prefab.yml was asked for before the world was initialized, nothing is spawned"));
  }

  TEST_F(EntityWorldTest, SpawnsNothingWithoutAPath)
  {
    _world.Initialize();
    EXPECT_CALL(_scene, Spawn(_, _, _, _)).Times(0);

    EXPECT_EQ(_world.Spawn("", No_Entity, DataValue{}), No_Entity);
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "A prefab without a path was asked for, nothing is spawned"));
  }

  TEST_F(EntityWorldTest, SpawnsNothingBelowAnEntityThatIsGone)
  {
    Entity crate = 0;
    PopulateWith([&](EntityStore &store) { crate = store.CreateEntity("crate"); });
    _world.Initialize();
    _store.DestroyEntity(crate);
    EXPECT_CALL(_scene, Spawn(_, _, _, _)).Times(0);

    EXPECT_EQ(_world.Spawn("assets://prefabs/wall.prefab.yml", crate, DataValue{}), No_Entity);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The prefab assets://prefabs/wall.prefab.yml was asked for below an entity that is gone, nothing is spawned"));
  }

  /// A format whose documents are values the test put in, under the name
  /// they are read by, so that the world reads a scene and its prefabs
  /// without a format of text.
  class FakeDocumentFormat final : public neon::DocumentFormat
  {
  public:
    std::map<std::string, DataValue> documents;

    bool Read(const std::string &name, const std::string &, DataValue &document, std::string &error) override
    {
      const auto found = documents.find(name);
      if (found == documents.end())
      {
        error = name + ": not a document the test wrote";
        return false;
      }
      document = found->second;
      return true;
    }

    std::string Write(const DataValue &) override
    {
      return "";
    }
  };

  /// The world with the scene an application gives it, a SceneFile, over a
  /// file system that counts what is read.
  class EntityWorldSpawnTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    FakeEntityStore _store;
    NiceMock<MockFileSystemContext> _files;
    FakeDocumentFormat _format;
    SceneFile _scene{&_files, &_format, "assets://scenes/test.scene.yml", _logger};
    NiceMock<MockRenderPipeline> _pipeline{_logger};
    NiceMock<FakeInputContext> _input{_logger};
    NiceMock<MockWindowContext> _window;
    EntityWorld _world{&_store, &_scene, &_pipeline, &_input, &_window, _logger};

    const std::string _wall = "assets://prefabs/wall.prefab.yml";

    void SetUp() override
    {
      ON_CALL(_window, GetDeltaTime()).WillByDefault(Return(0.5));

      // a file is there when the test wrote its document
      ON_CALL(_files, ReadText(_, _)).WillByDefault([this](const std::string &path, std::string &text)
      {
        text = path;
        return _format.documents.contains(path);
      });

      auto scene = DataValue::Map();
      scene.Set("entities", DataValue::List());
      _format.documents["assets://scenes/test.scene.yml"] = scene;
      _format.documents["assets://scenes/next.scene.yml"] = scene;

      // a wall: a Transform with a scale, and a Spectator
      auto scale = DataValue::List();
      scale.Add(DataValue::Number(1));
      scale.Add(DataValue::Number(2.5));
      scale.Add(DataValue::Number(1));
      auto transform = DataValue::Map();
      transform.Set("scale", scale);
      auto components = DataValue::Map();
      components.Set("Transform", transform);
      components.Set("Spectator", DataValue::Text("Default"));
      auto entity = DataValue::Map();
      entity.Set("components", components);
      auto wall = DataValue::Map();
      wall.Set("entity", entity);
      _format.documents[_wall] = wall;
    }

    void TearDown() override
    {
      _world.CleanUp();
    }
  };

  TEST_F(EntityWorldSpawnTest, SpawnsTwiceFromOnePathAndReadsTheFileOnce)
  {
    _world.Initialize();
    EXPECT_CALL(_files, ReadText(_, _)).Times(::testing::AnyNumber());
    EXPECT_CALL(_files, ReadText(_wall, _)).Times(1);

    const Entity first = _world.Spawn(_wall, No_Entity, DataValue{});
    const Entity second = _world.Spawn(_wall, No_Entity, DataValue{});

    ASSERT_NE(first, No_Entity);
    ASSERT_NE(second, No_Entity);
    EXPECT_NE(first, second);
    for (const Entity wall : {first, second})
    {
      EXPECT_EQ(_store.GetParent(wall), No_Entity);
      EXPECT_EQ(_store.Get<Transform>(wall)->scale.y, 2.5f);
      EXPECT_TRUE(_store.Has<Spectator>(wall));
      EXPECT_EQ(_store.Get<Prefab>(wall)->path, _wall) << "as a placed entity carries it";
    }
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(EntityWorldSpawnTest, SpawnsWithTheOverridesOnTopOfThePrefab)
  {
    Entity room = 0;
    _world.Initialize();
    room = _store.CreateEntity("room");

    auto position = DataValue::List();
    position.Add(DataValue::Number(1));
    position.Add(DataValue::Number(2));
    position.Add(DataValue::Number(3));
    auto transform = DataValue::Map();
    transform.Set("position", position);
    auto overrides = DataValue::Map();
    overrides.Set("Transform", transform);
    overrides.Set("Spectator", DataValue{});

    const Entity wall = _world.Spawn(_wall, room, overrides);

    ASSERT_NE(wall, No_Entity);
    EXPECT_EQ(_store.GetParent(wall), room);
    EXPECT_EQ(_store.Get<Transform>(wall)->position, glm::vec3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(_store.Get<Transform>(wall)->scale.y, 2.5f) << "what the overrides leave out stays the prefab's";
    EXPECT_FALSE(_store.Has<Spectator>(wall)) << "nothing takes a component away";
  }

  TEST_F(EntityWorldSpawnTest, SpawnsNothingFromAFileThatIsNotThereAndSaysSo)
  {
    _world.Initialize();

    EXPECT_EQ(_world.Spawn("assets://prefabs/nope.prefab.yml", No_Entity, DataValue{}), No_Entity);

    EXPECT_TRUE(_store.GetChildren(No_Entity).empty());
    EXPECT_THAT(_logger->Messages(LogLevel::Error), ::testing::HasSubstr(
                  "The prefab assets://prefabs/nope.prefab.yml has 1 problem, nothing is spawned"));
  }

  TEST_F(EntityWorldSpawnTest, ReadsThePrefabAgainAfterTheSceneChanged)
  {
    _world.Initialize();

    // the scene is read as well, which is not what is counted
    EXPECT_CALL(_files, ReadText(_, _)).Times(::testing::AnyNumber());
    EXPECT_CALL(_files, ReadText(_wall, _)).Times(2);

    ASSERT_NE(_world.Spawn(_wall, No_Entity, DataValue{}), No_Entity);

    _world.LoadScene("assets://scenes/next.scene.yml");
    _world.Update();

    ASSERT_NE(_world.Spawn(_wall, No_Entity, DataValue{}), No_Entity);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }
}
