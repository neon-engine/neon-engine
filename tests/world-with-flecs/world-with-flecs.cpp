// The world of entities as an application puts it together: EntityWorld with
// its systems, the entity store of Flecs, the forward render pipeline, and
// the window and input systems that need no devices. Only the renderer at the
// very end is a mock, which shows what would have been drawn.
//
// The unit tests of neon-core test each of these by itself, against a store
// that is a fake. This is where they meet the real one.

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/render/forward-render-pipeline.hpp>
#include <neon/testing/mock-input-system.hpp>
#include <neon/testing/mock-render-context.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/window/headless-window-system.hpp>
#include <neon/world-system/ecs/components/camera.hpp>
#include <neon/world-system/ecs/components/light.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>
#include <neon/world-system/ecs/components/spectator.hpp>
#include <neon/world-system/ecs/entity-world.hpp>
#include <neon/world-system/flecs-entity-store.hpp>

namespace
{
  using neon::Action;
  using neon::Camera;
  using neon::Entity;
  using neon::EntityBlock;
  using neon::EntityStore;
  using neon::EntitySystem;
  using neon::EntityWorld;
  using neon::Flecs_EntityStore;
  using neon::Forward_RenderPipeline;
  using neon::Headless_WindowSystem;
  using neon::Light;
  using neon::LightSource;
  using neon::No_Entity;
  using neon::Renderable;
  using neon::RenderInfo;
  using neon::RenderResolution;
  using neon::Scene;
  using neon::Spectator;
  using neon::Transform;
  using neon::testing::FakeInputContext;
  using neon::testing::LogLevel;
  using neon::testing::MockRenderContext;
  using neon::testing::RecordingLogger;
  using ::testing::_;
  using ::testing::AnyNumber;
  using ::testing::ElementsAre;
  using ::testing::Field;
  using ::testing::Invoke;
  using ::testing::IsEmpty;
  using ::testing::NiceMock;
  using ::testing::Return;
  using ::testing::ReturnRef;
  using ::testing::UnorderedElementsAre;

  constexpr float tolerance = 1e-4f;

  /// A scene that is written by the test that uses it.
  class TestScene final : public Scene
  {
  public:
    std::function<void(EntityStore &)> populate;

    void Populate(EntityStore &store) override
    {
      if (populate) { populate(store); }
    }
  };

  /// A system of a game that does what the test tells it to.
  class TestSystem final : public EntitySystem
  {
    std::function<void(EntityStore &)> _on_initialize;
    std::function<void(EntityStore &, double)> _on_update;

  public:
    TestSystem(
      const std::function<void(EntityStore &)> &on_initialize,
      const std::function<void(EntityStore &, double)> &on_update)
    {
      _on_initialize = on_initialize;
      _on_update = on_update;
    }

    void Initialize(EntityStore &store) override
    {
      if (_on_initialize) { _on_initialize(store); }
    }

    void Update(EntityStore &store, const double delta_time) override
    {
      if (_on_update) { _on_update(store, delta_time); }
    }
  };

  /// What the renderer was asked to draw.
  struct Drawn
  {
    std::string model;
    glm::vec3 place;
    glm::mat4 view;
    std::vector<LightSource> lights;
  };

  Transform At(const float x, const float y, const float z)
  {
    Transform transform;
    transform.position = {x, y, z};
    return transform;
  }

  Renderable Model(const std::string &path)
  {
    Renderable renderable;
    renderable.render_info.model_path = path;
    return renderable;
  }

  Entity Create(
    EntityStore &store,
    const std::string &name,
    const Transform &transform,
    const Entity parent = No_Entity)
  {
    const Entity entity = store.CreateEntity(name, parent);
    store.Set(entity, transform);
    return entity;
  }

  class WorldWithFlecsTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();

    // the renderer, which hands out ids and writes down what it draws
    NiceMock<MockRenderContext> _renderer{_logger};
    RenderResolution _resolution{1280, 720};
    std::vector<std::string> _models;
    std::vector<int> _destroyed;
    std::vector<Drawn> _drawn;

    Forward_RenderPipeline _pipeline{&_renderer, 8, _logger};
    Headless_WindowSystem _window{SettingsConfig{.width = 1280, .height = 720, .time_step = 0.5}, _logger};
    NiceMock<FakeInputContext> _input{_logger};
    Flecs_EntityStore _store{_logger};
    TestScene _scene;
    EntityWorld _world{&_store, &_scene, &_pipeline, &_input, &_window, _logger};

    void SetUp() override
    {
      ON_CALL(_renderer, GetRenderResolution()).WillByDefault(ReturnRef(_resolution));

      ON_CALL(_renderer, CreateRenderObject(_)).WillByDefault(Invoke([this](const RenderInfo &render_info)
      {
        // a model of that name stands for one that cannot be loaded
        if (render_info.model_path == "assets://models/missing.obj") { return -1; }

        _models.push_back(render_info.model_path);
        return static_cast<int>(_models.size()) - 1;
      }));

      ON_CALL(_renderer, DestroyRenderObject(_)).WillByDefault(Invoke([this](const int id)
      {
        _destroyed.push_back(id);
      }));

      ON_CALL(_renderer, DrawRenderObject(_, _, _, _, _)).WillByDefault(Invoke(
        [this](
        const int id,
        const Transform &transform,
        const glm::mat4 &view,
        const glm::mat4 &,
        const std::vector<LightSource> &lights)
        {
          _drawn.push_back({_models.at(id), glm::vec3(transform.world_coordinates[3]), view, lights});
        }));
    }

    void TearDown() override
    {
      _world.CleanUp();
    }

    /// Runs one frame and returns what was drawn in it.
    std::vector<Drawn> Frame()
    {
      _drawn.clear();
      _world.Update();
      return _drawn;
    }

    static const Drawn *Find(const std::vector<Drawn> &drawn, const std::string &model)
    {
      for (const auto &one : drawn)
      {
        if (one.model == model) { return &one; }
      }
      return nullptr;
    }

    static void ExpectPlace(const std::vector<Drawn> &drawn, const std::string &model, const glm::vec3 &place)
    {
      const Drawn *found = Find(drawn, model);
      ASSERT_NE(found, nullptr) << model << " was not drawn";
      EXPECT_NEAR(found->place.x, place.x, tolerance) << model;
      EXPECT_NEAR(found->place.y, place.y, tolerance) << model;
      EXPECT_NEAR(found->place.z, place.z, tolerance) << model;
    }

    void ExpectNoErrors() const
    {
      EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
      EXPECT_EQ(_logger->Count(LogLevel::Critical), 0u) << _logger->Messages(LogLevel::Critical);
    }
  };

  TEST_F(WorldWithFlecsTest, DrawsNothingInAnEmptyWorld)
  {
    _world.Initialize();

    EXPECT_THAT(Frame(), IsEmpty());
    EXPECT_THAT(Frame(), IsEmpty());
    ExpectNoErrors();
  }

  TEST_F(WorldWithFlecsTest, DrawsEveryVisibleEntityOfTheScene)
  {
    _scene.populate = [](EntityStore &store)
    {
      store.Set(Create(store, "cube", At(1.0f, 0.0f, 0.0f)), Model("cube"));
      store.Set(Create(store, "sphere", At(0.0f, 2.0f, 0.0f)), Model("sphere"));
      Create(store, "marker", At(0.0f, 0.0f, 3.0f));
      store.CreateEntity("folder");
    };
    _world.Initialize();

    const auto drawn = Frame();

    EXPECT_EQ(drawn.size(), 2u);
    ExpectPlace(drawn, "cube", {1.0f, 0.0f, 0.0f});
    ExpectPlace(drawn, "sphere", {0.0f, 2.0f, 0.0f});
    ExpectNoErrors();
  }

  TEST_F(WorldWithFlecsTest, MakesEveryEntityKnownToTheRendererOnce)
  {
    _scene.populate = [](EntityStore &store)
    {
      store.Set(Create(store, "cube", At(1.0f, 0.0f, 0.0f)), Model("cube"));
      store.Set(Create(store, "sphere", At(0.0f, 2.0f, 0.0f)), Model("sphere"));
    };
    _world.Initialize();

    for (int frame = 0; frame < 5; frame++) { EXPECT_EQ(Frame().size(), 2u); }

    EXPECT_THAT(_models, UnorderedElementsAre("cube", "sphere"));
  }

  TEST_F(WorldWithFlecsTest, PlacesChildrenRelativeToTheirParentsOverSeveralLevels)
  {
    _scene.populate = [](EntityStore &store)
    {
      Transform of_body = At(10.0f, 0.0f, 0.0f);
      of_body.rotation.yaw = 90.0f;
      Transform of_arm = At(0.0f, 0.0f, -2.0f);
      of_arm.scale = {3.0f, 3.0f, 3.0f};

      // the children first, so that the order of creation does not give the
      // right answer by accident
      const Entity hand = Create(store, "hand", At(0.0f, 0.0f, -1.0f));
      const Entity arm = Create(store, "arm", of_arm);
      const Entity body = Create(store, "body", of_body);
      store.SetParent(hand, arm);
      store.SetParent(arm, body);

      store.Set(body, Model("body"));
      store.Set(arm, Model("arm"));
      store.Set(hand, Model("hand"));
    };
    _world.Initialize();

    const auto drawn = Frame();

    ExpectPlace(drawn, "body", {10.0f, 0.0f, 0.0f});
    // 2 in front of the body, which looks to the left
    ExpectPlace(drawn, "arm", {8.0f, 0.0f, 0.0f});
    // 1 in front of the arm, in the size of the arm
    ExpectPlace(drawn, "hand", {5.0f, 0.0f, 0.0f});
    ExpectNoErrors();
  }

  TEST_F(WorldWithFlecsTest, PlacesAnEntityBelowAFolderRelativeToWhatIsAboveTheFolder)
  {
    _scene.populate = [](EntityStore &store)
    {
      const Entity room = Create(store, "room", At(100.0f, 0.0f, 0.0f));
      // a folder has no place in the world
      const Entity furniture = store.CreateEntity("furniture", room);
      store.Set(Create(store, "chair", At(1.0f, 2.0f, 3.0f), furniture), Model("chair"));

      const Entity loose = store.CreateEntity("loose");
      store.Set(Create(store, "rock", At(4.0f, 5.0f, 6.0f), loose), Model("rock"));
    };
    _world.Initialize();

    const auto drawn = Frame();

    ExpectPlace(drawn, "chair", {101.0f, 2.0f, 3.0f});
    ExpectPlace(drawn, "rock", {4.0f, 5.0f, 6.0f});
    ExpectNoErrors();
  }

  TEST_F(WorldWithFlecsTest, MovesTheChildrenWithTheirParentInTheSameFrame)
  {
    _scene.populate = [](EntityStore &store)
    {
      const Entity cart = Create(store, "cart", At(0.0f, 0.0f, 0.0f));
      store.Set(cart, Model("cart"));
      store.Set(Create(store, "load", At(0.0f, 1.0f, 0.0f), cart), Model("load"));
    };
    _world.AddSystem(std::make_unique<TestSystem>(nullptr, [](EntityStore &store, const double delta_time)
    {
      // 2 units per second
      store.Get<Transform>(store.FindEntity("cart"))->position.x += 2.0f * static_cast<float>(delta_time);
    }));
    _world.Initialize();

    ExpectPlace(Frame(), "load", {1.0f, 1.0f, 0.0f});
    ExpectPlace(Frame(), "load", {2.0f, 1.0f, 0.0f});

    const auto drawn = Frame();
    ExpectPlace(drawn, "cart", {3.0f, 0.0f, 0.0f});
    ExpectPlace(drawn, "load", {3.0f, 1.0f, 0.0f});
  }

  TEST_F(WorldWithFlecsTest, DrawsWithTheLightsWhereTheyAreInTheWorld)
  {
    _scene.populate = [](EntityStore &store)
    {
      store.Set(Create(store, "cube", At(0.0f, 0.0f, 0.0f)), Model("cube"));

      const Entity post = Create(store, "post", At(5.0f, 0.0f, 0.0f));
      Light lamp;
      lamp.source.id = "lamp";
      store.Set(Create(store, "lamp", At(0.0f, 3.0f, 0.0f), post), lamp);
    };
    _world.Initialize();

    const auto drawn = Frame();

    ASSERT_EQ(drawn.size(), 1u);
    ASSERT_EQ(drawn[0].lights.size(), 1u);
    EXPECT_EQ(drawn[0].lights[0].id, "lamp");
    EXPECT_EQ(drawn[0].lights[0].position, glm::vec3(5.0f, 3.0f, 0.0f));

    // and again in the next frame, no more and no less
    EXPECT_EQ(Frame()[0].lights.size(), 1u);
  }

  TEST_F(WorldWithFlecsTest, LooksThroughTheCameraWhereItIsInTheWorld)
  {
    _scene.populate = [](EntityStore &store)
    {
      store.Set(Create(store, "cube", At(0.0f, 0.0f, 0.0f)), Model("cube"));

      const Entity player = Create(store, "player", At(0.0f, 0.0f, 10.0f));
      store.Set(Create(store, "eyes", At(0.0f, 2.0f, 0.0f), player), Camera{});
    };
    _world.Initialize();

    const auto drawn = Frame();

    ASSERT_EQ(drawn.size(), 1u);
    const auto view =
      lookAt(glm::vec3(0.0f, 2.0f, 10.0f), glm::vec3(0.0f, 2.0f, 9.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    for (int column = 0; column < 4; column++)
    {
      for (int row = 0; row < 4; row++)
      {
        EXPECT_NEAR(drawn[0].view[column][row], view[column][row], tolerance)
          << "column " << column << ", row " << row;
      }
    }
  }

  TEST_F(WorldWithFlecsTest, MovesTheSpectatorByTheInput)
  {
    _scene.populate = [](EntityStore &store)
    {
      const Entity spectator = Create(store, "spectator", At(0.0f, 0.0f, 0.0f));
      store.Set(spectator, Spectator{.move_speed = 2.0f, .look_speed = 0.5f});
      store.Set(spectator, Model("spectator"));
    };
    _world.Initialize();

    // half a second at 2 units per second
    _input.state.SetAction(Action::L_Up);
    ExpectPlace(Frame(), "spectator", {0.0f, 0.0f, -1.0f});
    ExpectPlace(Frame(), "spectator", {0.0f, 0.0f, -2.0f});

    // turns to the left, and walks that way from the next frame on
    _input.state.SetAxisMotion(neon::Axis::Mouse, -180.0, 0.0);
    ExpectPlace(Frame(), "spectator", {0.0f, 0.0f, -3.0f});
    _input.state.Reset();
    _input.state.SetAction(Action::L_Up);
    ExpectPlace(Frame(), "spectator", {-1.0f, 0.0f, -3.0f});

    _input.state.Reset();
    ExpectPlace(Frame(), "spectator", {-1.0f, 0.0f, -3.0f});
  }

  TEST_F(WorldWithFlecsTest, DrawsAnEntityThatASystemCreatesDuringAQuery)
  {
    _scene.populate = [](EntityStore &store)
    {
      store.Set(Create(store, "gun", At(1.0f, 0.0f, 0.0f)), Model("gun"));
    };
    auto query = std::make_shared<neon::QueryId>(0);
    auto fired = std::make_shared<bool>(false);
    _world.AddSystem(std::make_unique<TestSystem>(
      [query](EntityStore &store)
      {
        *query = store.Query<Transform, Renderable>();
      },
      [query, fired](EntityStore &store, double)
      {
        if (*fired) { return; }
        *fired = true;

        store.Each(*query, [&store](const EntityBlock &block)
        {
          for (std::size_t i = 0; i < block.count; i++)
          {
            const Entity bullet = store.CreateEntity("bullet", block.entities[i]);
            store.Set(bullet, At(0.0f, 0.0f, -1.0f));
            store.Set(bullet, Model("bullet"));
          }
        });
      }));
    _world.Initialize();

    const auto drawn = Frame();

    EXPECT_EQ(drawn.size(), 2u);
    ExpectPlace(drawn, "gun", {1.0f, 0.0f, 0.0f});
    ExpectPlace(drawn, "bullet", {1.0f, 0.0f, -1.0f});
    EXPECT_EQ(Frame().size(), 2u);
    ExpectNoErrors();
  }

  TEST_F(WorldWithFlecsTest, StopsDrawingAnEntityThatIsDestroyedAndReleasesItsRenderObject)
  {
    _scene.populate = [](EntityStore &store)
    {
      const Entity table = Create(store, "table", At(0.0f, 0.0f, 0.0f));
      store.Set(table, Model("table"));
      store.Set(Create(store, "cup", At(0.0f, 1.0f, 0.0f), table), Model("cup"));
      store.Set(Create(store, "chair", At(1.0f, 0.0f, 0.0f)), Model("chair"));
    };
    _world.Initialize();
    ASSERT_EQ(Frame().size(), 3u);

    // the cup goes with the table
    _store.DestroyEntity(_store.FindEntity("table"));

    const auto drawn = Frame();
    ASSERT_EQ(drawn.size(), 1u);
    EXPECT_EQ(drawn[0].model, "chair");

    ASSERT_EQ(_destroyed.size(), 2u);
    std::vector<std::string> released;
    for (const int id : _destroyed) { released.push_back(_models.at(id)); }
    EXPECT_THAT(released, UnorderedElementsAre("table", "cup"));
  }

  TEST_F(WorldWithFlecsTest, StopsDrawingAnEntityThatASystemDestroysDuringAQuery)
  {
    _scene.populate = [](EntityStore &store)
    {
      store.Set(Create(store, "cube", At(1.0f, 0.0f, 0.0f)), Model("cube"));
      store.Set(Create(store, "sphere", At(2.0f, 0.0f, 0.0f)), Model("sphere"));
    };
    auto query = std::make_shared<neon::QueryId>(0);
    auto frame = std::make_shared<int>(0);
    _world.AddSystem(std::make_unique<TestSystem>(
      [query](EntityStore &store)
      {
        *query = store.Query<Transform, Renderable>();
      },
      [query, frame](EntityStore &store, double)
      {
        if (++*frame != 2) { return; }

        store.Each(*query, [&store](const EntityBlock &block)
        {
          for (std::size_t i = 0; i < block.count; i++)
          {
            if (store.GetName(block.entities[i]) == "cube") { store.DestroyEntity(block.entities[i]); }
          }
        });
      }));
    _world.Initialize();

    EXPECT_EQ(Frame().size(), 2u);

    const auto drawn = Frame();
    ASSERT_EQ(drawn.size(), 1u);
    EXPECT_EQ(drawn[0].model, "sphere");
    ASSERT_EQ(_destroyed.size(), 1u);
    EXPECT_EQ(_models.at(_destroyed[0]), "cube");
  }

  TEST_F(WorldWithFlecsTest, LeavesOutAnEntityTheRendererCouldNotCreateAndDrawsTheRest)
  {
    _scene.populate = [](EntityStore &store)
    {
      store.Set(Create(store, "broken", At(1.0f, 0.0f, 0.0f)), Model("assets://models/missing.obj"));
      store.Set(Create(store, "cube", At(2.0f, 0.0f, 0.0f)), Model("cube"));
    };
    _world.Initialize();

    for (int frame = 0; frame < 3; frame++)
    {
      const auto drawn = Frame();
      ASSERT_EQ(drawn.size(), 1u);
      EXPECT_EQ(drawn[0].model, "cube");
    }

    _world.CleanUp();
    ASSERT_EQ(_destroyed.size(), 1u);
    EXPECT_EQ(_models.at(_destroyed[0]), "cube");
  }

  TEST_F(WorldWithFlecsTest, ReleasesEveryRenderObjectWhenTheWorldIsCleanedUp)
  {
    _scene.populate = [](EntityStore &store)
    {
      const Entity table = Create(store, "table", At(0.0f, 0.0f, 0.0f));
      store.Set(table, Model("table"));
      store.Set(Create(store, "cup", At(0.0f, 1.0f, 0.0f), table), Model("cup"));
      store.Set(Create(store, "chair", At(1.0f, 0.0f, 0.0f)), Model("chair"));
    };
    _world.Initialize();
    Frame();

    _world.CleanUp();

    EXPECT_THAT(_destroyed, UnorderedElementsAre(0, 1, 2));
    ExpectNoErrors();
  }

  TEST_F(WorldWithFlecsTest, StartsAgainFromTheSceneAfterItWasCleanedUp)
  {
    _scene.populate = [](EntityStore &store)
    {
      store.Set(Create(store, "cube", At(1.0f, 0.0f, 0.0f)), Model("cube"));
    };
    _world.AddSystem(std::make_unique<TestSystem>(nullptr, [](EntityStore &store, double)
    {
      store.Get<Transform>(store.FindEntity("cube"))->position.x += 1.0f;
    }));
    _world.Initialize();
    ExpectPlace(Frame(), "cube", {2.0f, 0.0f, 0.0f});
    ExpectPlace(Frame(), "cube", {3.0f, 0.0f, 0.0f});

    _world.CleanUp();
    _world.Initialize();

    ExpectPlace(Frame(), "cube", {2.0f, 0.0f, 0.0f});
    ExpectNoErrors();
  }

  TEST_F(WorldWithFlecsTest, LetsAnExceptionOfASystemThroughAndCanStillBeCleanedUp)
  {
    _scene.populate = [](EntityStore &store)
    {
      store.Set(Create(store, "cube", At(1.0f, 0.0f, 0.0f)), Model("cube"));
    };
    auto query = std::make_shared<neon::QueryId>(0);
    auto frame = std::make_shared<int>(0);
    _world.AddSystem(std::make_unique<TestSystem>(
      [query](EntityStore &store)
      {
        *query = store.Query<Transform>();
      },
      [query, frame](EntityStore &store, double)
      {
        if (++*frame != 2) { return; }
        store.Each(*query, [](const EntityBlock &) { throw std::runtime_error("the game broke"); });
      }));
    _world.Initialize();
    ASSERT_EQ(Frame().size(), 1u);

    EXPECT_THROW(_world.Update(), std::runtime_error);

    // the next frame works as before
    EXPECT_EQ(Frame().size(), 1u);

    _world.CleanUp();
    EXPECT_THAT(_destroyed, ElementsAre(0));
  }
}
