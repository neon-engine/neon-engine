#include "entity-world.hpp"

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/mock-entity-world.hpp>
#include <neon/testing/mock-input-system.hpp>
#include <neon/testing/mock-render-pipeline.hpp>
#include <neon/testing/mock-window-system.hpp>
#include <neon/testing/recording-logger.hpp>

#include "components/camera.hpp"
#include "components/light.hpp"
#include "components/renderable.hpp"
#include "components/spectator.hpp"

namespace
{
  using neon::Action;
  using neon::Camera;
  using neon::Entity;
  using neon::EntityStore;
  using neon::EntitySystem;
  using neon::EntityWorld;
  using neon::Light;
  using neon::No_Component;
  using neon::Renderable;
  using neon::Spectator;
  using neon::Transform;
  using neon::testing::FakeEntityStore;
  using neon::testing::FakeInputContext;
  using neon::testing::LogLevel;
  using neon::testing::MockEntitySystem;
  using neon::testing::MockRenderPipeline;
  using neon::testing::MockScene;
  using neon::testing::MockWindowContext;
  using neon::testing::RecordingLogger;
  using ::testing::_;
  using ::testing::ElementsAre;
  using ::testing::InSequence;
  using ::testing::Invoke;
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
      ON_CALL(_scene, Populate(_)).WillByDefault(Invoke(populate));
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

    for (const char *name : {"Transform", "Camera", "Light", "Spectator", "Renderable"})
    {
      EXPECT_NE(_store.FindComponent(name), No_Component) << name;
    }
    EXPECT_EQ(_store.IdOf<Transform>(), _store.FindComponent("Transform"));
    EXPECT_EQ(_store.IdOf<Camera>(), _store.FindComponent("Camera"));
    EXPECT_EQ(_store.IdOf<Light>(), _store.FindComponent("Light"));
    EXPECT_EQ(_store.IdOf<Spectator>(), _store.FindComponent("Spectator"));
    EXPECT_EQ(_store.IdOf<Renderable>(), _store.FindComponent("Renderable"));
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

    EXPECT_THAT(_calls, ElementsAre("first initialized", "second initialized", "scene populated"));
  }

  TEST_F(EntityWorldTest, InitializesASystemOfTheGameWithTheStoreAndTheComponentsOfTheEngine)
  {
    auto system = std::make_unique<StrictMock<MockEntitySystem>>();
    EXPECT_CALL(*system, Initialize(Ref(_store))).WillOnce(Invoke([](EntityStore &store)
    {
      // the place to create queries, which needs the components
      EXPECT_NO_THROW((void) (store.Query<Transform, Renderable>()));
    }));
    _world.AddSystem(std::move(system));

    _world.Initialize();
  }

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
    _input.state.SetAction(Action::L_Up);

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
    EXPECT_CALL(_pipeline, EnqueueForRendering(4, _)).WillOnce(Invoke([&](int, const Transform &transform)
    {
      drawn_at = transform.world_coordinates[3];
    }));
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

    EXPECT_THAT(_calls, ElementsAre("game initialized", "game updated"));
  }
}
