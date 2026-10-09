// A scene in a file, as an application puts it together: SceneFile with the
// document format for YAML and the entity store of Flecs. Files are kept in
// memory.
//
// The unit tests of neon-core test how a component is read and written
// against values that are built in code. This is where a text becomes a
// world and a world becomes a text.

#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/world-system/ecs/components/pool-manager.hpp>
#include <neon/common/transform.hpp>
#include <neon/data/ryml-document-format.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/camera.hpp>
#include <neon/world-system/ecs/components/geometry.hpp>
#include <neon/world-system/ecs/components/light.hpp>
#include <neon/world-system/ecs/components/persistent.hpp>
#include <neon/world-system/ecs/components/scene-exit.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>
#include <neon/world-system/ecs/components/sound-listener.hpp>
#include <neon/world-system/ecs/components/sound-source.hpp>
#include <neon/world-system/ecs/components/spectator.hpp>
#include <neon/world-system/ecs/scene-file/scene-file.hpp>
#include <neon/world-system/flecs-entity-store.hpp>

namespace
{
  using neon::Camera;
  using neon::ComponentFormat;
  using neon::DataReader;
  using neon::DataValue;
  using neon::Entity;
  using neon::EntityStore;
  using neon::Flecs_EntityStore;
  using neon::Light;
  using neon::LightType;
  using neon::No_Entity;
  using neon::Renderable;
  using neon::RYML_DocumentFormat;
  using neon::SceneFile;
  using neon::Spectator;
  using neon::Transform;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using ::testing::ElementsAre;
  using ::testing::HasSubstr;
  using ::testing::IsEmpty;

  struct Health
  {
    int points = 100;
  };

  /// A store with the components of the engine, as EntityWorld and the
  /// systems of the runtime register them.
  class World
  {
    Flecs_EntityStore _flecs;

  public:
    EntityStore &store = _flecs;

    explicit World(const std::shared_ptr<RecordingLogger> &logger) : _flecs(logger)
    {
      store.Initialize();
      store.Register<Transform>("Transform");
      store.Register<Camera>("Camera");
      store.Register<Light>("Light");
      store.Register<Spectator>("Spectator");
      store.Register<Renderable>("Renderable");
      store.Register<neon::SoundSource>("SoundSource");
      store.Register<neon::SoundListener>("SoundListener");
      store.Register<neon::Persistent>("Persistent");
      store.Register<neon::SceneExit>("SceneExit");
      store.Register<Health>("Health");
    }

    ~World()
    {
      store.CleanUp();
    }

    [[nodiscard]] std::vector<std::string> NamesAtTheTop() const
    {
      std::vector<std::string> names;
      for (const auto entity : store.GetChildren(No_Entity)) { names.push_back(store.GetName(entity)); }
      return names;
    }
  };

  class SceneFilesTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _files{SettingsConfig{}, _logger};
    RYML_DocumentFormat _yaml;
    World _world{_logger};
    SceneFile _scene{&_files, &_yaml, "assets://scenes/test.scene.yml", _logger};

    void SetUp() override
    {
      _files.Initialize();
    }

    void Write(const std::string &text)
    {
      _files.AddNativeFile("/assets/scenes/test.scene.yml", text);
    }

    /// Loads a scene that is expected to have problems. Returns the errors
    /// that were logged, one per line.
    std::string ProblemsOf(const std::string &text)
    {
      Write(text);
      _logger->Clear();
      EXPECT_FALSE(_scene.Populate(_world.store)) << text;
      return _logger->Messages(LogLevel::Error);
    }

    static std::string SmallScene()
    {
      const std::ifstream file(NEON_SMALL_SCENE);
      EXPECT_TRUE(file.good()) << NEON_SMALL_SCENE;

      std::stringstream text;
      text << file.rdbuf();
      return text.str();
    }

    std::string ReadFile(const std::string &path)
    {
      std::string text;
      EXPECT_TRUE(_files.ReadText(path, text)) << path;
      return text;
    }
  };

  // loading

  TEST_F(SceneFilesTest, LoadsASceneFromTheRepository)
  {
    Write(SmallScene());

    _scene.Populate(_world.store);

    EXPECT_THAT(_world.NamesAtTheTop(), ElementsAre("bear", "sphere", "floor", "direction", "player"));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(SceneFilesTest, PutsAChildBelowItsParent)
  {
    Write(SmallScene());

    _scene.Populate(_world.store);

    const auto camera = _world.store.FindEntity("player/camera");
    ASSERT_NE(camera, No_Entity);
    EXPECT_TRUE(_world.store.Has<Camera>(camera));
    EXPECT_EQ(_world.store.GetParent(camera), _world.store.FindEntity("player"));
  }

  TEST_F(SceneFilesTest, ReadsTheValuesOfASceneFromTheRepository)
  {
    Write(SmallScene());

    _scene.Populate(_world.store);

    auto &store = _world.store;
    EXPECT_EQ(store.Get<Transform>(store.FindEntity("sphere"))->scale, glm::vec3(0.2f));
    EXPECT_EQ(store.Get<Transform>(store.FindEntity("floor"))->scale, glm::vec3(100.0f, 0.1f, 100.0f));
    EXPECT_FALSE(store.Get<Renderable>(store.FindEntity("sphere"))->render_info.material_info.use_textures);
    EXPECT_TRUE(store.Get<Renderable>(store.FindEntity("bear"))->render_info.material_info.use_textures);
    EXPECT_EQ(store.Get<Light>(store.FindEntity("direction"))->source.light_type, LightType::Direction);
    EXPECT_EQ(store.Get<Spectator>(store.FindEntity("player"))->move_speed, 2.5f);
  }

  TEST_F(SceneFilesTest, KnowsThePathItWasMadeWith)
  {
    EXPECT_EQ(_scene.GetPath(), "assets://scenes/test.scene.yml");
  }

  TEST_F(SceneFilesTest, AComponentIsWrittenAsTurnedOffAndKeepsWhatWasWrittenForIt)
  {
    Write(
      "entities:\n"
      "  - name: nail\n"
      "    components:\n"
      "      Transform:\n"
      "        position: [1, 2, 3]\n"
      "      Camera:\n"
      "        enabled: false\n"
      "        fov: 70\n");
    ASSERT_TRUE(_scene.Populate(_world.store)) << _logger->Messages(LogLevel::Error);

    auto &store = _world.store;
    const auto nail = store.FindEntity("nail");
    EXPECT_TRUE(store.IsEnabled<neon::Transform>(nail));
    EXPECT_FALSE(store.IsEnabled<Camera>(nail));
    EXPECT_FALSE(store.Has<Camera>(nail));

    const auto *camera = static_cast<const Camera *>(store.GetComponentData(nail, store.IdOf<Camera>()));
    ASSERT_NE(camera, nullptr);
    EXPECT_EQ(camera->fov, 70.0f);
  }

  TEST_F(SceneFilesTest, APoolIsWrittenByWhatItIsToHold)
  {
    _world.store.Register<neon::PoolManager>("PoolManager");
    Write(
      "entities:\n"
      "  - name: projectiles\n"
      "    components:\n"
      "      PoolManager:\n"
      "        _entries:\n"
      "          - source: assets://prefabs/nail.prefab.yml\n"
      "            count: 64\n"
      "          - source: instance://lamp\n"
      "            count: 1\n");
    ASSERT_TRUE(_scene.Populate(_world.store)) << _logger->Messages(LogLevel::Error);

    const auto *pool = _world.store.Get<neon::PoolManager>(_world.store.FindEntity("projectiles"));
    ASSERT_NE(pool, nullptr);
    ASSERT_EQ(pool->entries.size(), 2u);
    EXPECT_EQ(pool->entries[0].source, "assets://prefabs/nail.prefab.yml");
    EXPECT_EQ(pool->entries[0].count, 64);
    EXPECT_EQ(pool->entries[1].source, "instance://lamp");
    EXPECT_FALSE(pool->is_filled);
  }

  TEST_F(SceneFilesTest, SaysWhatIsWrongWithWhatAPoolIsToHold)
  {
    _world.store.Register<neon::PoolManager>("PoolManager");
    Write(
      "entities:\n"
      "  - name: projectiles\n"
      "    components:\n"
      "      PoolManager:\n"
      "        _entries:\n"
      "          - source: assets://prefabs/nail.prefab.yml\n"
      "          - count: 3\n"
      "          - source: assets://prefabs/rocket.prefab.yml\n"
      "            count: 8\n");
    EXPECT_FALSE(_scene.Populate(_world.store));

    const auto *pool = _world.store.Get<neon::PoolManager>(_world.store.FindEntity("projectiles"));
    ASSERT_NE(pool, nullptr);
    ASSERT_EQ(pool->entries.size(), 1u) << "what is right is kept";
    EXPECT_EQ(pool->entries[0].count, 8);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 3u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(SceneFilesTest, WritesThatAComponentIsTurnedOffAndNothingForOneThatIsOn)
  {
    auto &store = _world.store;
    const auto nail = store.CreateEntity("nail");
    store.Set(nail, Transform{});
    store.Set(nail, Spectator{});
    Camera camera;
    camera.fov = 70.0f;
    store.Set(nail, camera);
    store.SetEnabled<Camera>(nail, false);
    store.SetEnabled<Spectator>(nail, false);

    ASSERT_TRUE(_scene.Save(store, "saved", "user://saved.scene.yml"));

    // What is off says so, with what it holds, and one that keeps all of
    // its defaults says that alone. What is on is written as it always was.
    EXPECT_EQ(
      ReadFile("user://saved.scene.yml"),
      "scene: saved\n"
      "version: 1\n"
      "\n"
      "entities:\n"
      "  - name: nail\n"
      "    components:\n"
      "      Transform: Default\n"
      "      Camera:\n"
      "        fov: 70\n"
      "        enabled: false\n"
      "      Spectator:\n"
      "        enabled: false\n");
  }

  TEST_F(SceneFilesTest, LoadsAnotherFileAndIsThatSceneFromThenOn)
  {
    Write("entities:\n  - name: first\n");
    _files.AddNativeFile("/assets/scenes/other.scene.yml", "entities:\n  - name: other\n");

    EXPECT_TRUE(_scene.Load(_world.store, "assets://scenes/other.scene.yml"));

    EXPECT_EQ(_scene.GetPath(), "assets://scenes/other.scene.yml");
    EXPECT_NE(_world.store.FindEntity("other"), No_Entity);
    EXPECT_EQ(_world.store.FindEntity("first"), No_Entity);
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Loading the scene from assets://scenes/other.scene.yml"));
  }

  TEST_F(SceneFilesTest, SaysWhenTheFileItIsAskedToLoadIsMissing)
  {
    EXPECT_FALSE(_scene.Load(_world.store, "assets://scenes/missing.scene.yml"));
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "assets://scenes/missing.scene.yml"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(SceneFilesTest, ReadsAheadAFileThatIsThere)
  {
    _files.AddNativeFile("/assets/scenes/other.scene.yml", "entities:\n  - name: other\n");

    EXPECT_TRUE(_scene.ReadAhead("assets://scenes/other.scene.yml"));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(SceneFilesTest, CannotReadAheadAFileThatIsMissingAndSaysWhy)
  {
    EXPECT_FALSE(_scene.ReadAhead("assets://scenes/missing.scene.yml"));
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The scene assets://scenes/missing.scene.yml cannot be read: there is no such file, or it cannot be opened"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(SceneFilesTest, CannotReadAheadAFileThatHoldsNoDocumentAndSaysWhy)
  {
    _files.AddNativeFile("/assets/scenes/broken.scene.yml", "entities: [unclosed\n");

    EXPECT_FALSE(_scene.ReadAhead("assets://scenes/broken.scene.yml"));
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "The scene assets://scenes/broken.scene.yml has 1 problem, the game stays where it is"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(SceneFilesTest, LoadsWhatWasReadAheadWithoutReadingTheFileAgain)
  {
    _files.AddNativeFile("/assets/scenes/other.scene.yml", "entities:\n  - name: other\n");
    ASSERT_TRUE(_scene.ReadAhead("assets://scenes/other.scene.yml"));
    // the file changes after it was read ahead; what was read is placed
    _files.AddNativeFile("/assets/scenes/other.scene.yml", "entities:\n  - name: changed\n");

    EXPECT_TRUE(_scene.Load(_world.store, "assets://scenes/other.scene.yml"));

    EXPECT_NE(_world.store.FindEntity("other"), No_Entity);
    EXPECT_EQ(_world.store.FindEntity("changed"), No_Entity);
  }

  TEST_F(SceneFilesTest, ReadsTheFileAgainWhenWhatWasReadAheadWasPlacedOnce)
  {
    _files.AddNativeFile("/assets/scenes/other.scene.yml", "entities:\n  - name: other\n");
    ASSERT_TRUE(_scene.ReadAhead("assets://scenes/other.scene.yml"));
    ASSERT_TRUE(_scene.Load(_world.store, "assets://scenes/other.scene.yml"));
    _world.store.DestroyEntity(_world.store.FindEntity("other"));
    _files.AddNativeFile("/assets/scenes/other.scene.yml", "entities:\n  - name: changed\n");

    EXPECT_TRUE(_scene.Load(_world.store, "assets://scenes/other.scene.yml"));

    EXPECT_NE(_world.store.FindEntity("changed"), No_Entity);
  }

  TEST_F(SceneFilesTest, AnEmptyFileIsAnEmptyScene)
  {
    Write("");

    _scene.Populate(_world.store);

    EXPECT_THAT(_world.NamesAtTheTop(), IsEmpty());
  }

  TEST_F(SceneFilesTest, AnEntityWithoutANameIsCreated)
  {
    Write("entities:\n  - components:\n      Spectator: {}\n");

    _scene.Populate(_world.store);

    ASSERT_EQ(_world.store.GetChildren(No_Entity).size(), 1u);
    EXPECT_TRUE(_world.store.Has<Spectator>(_world.store.GetChildren(No_Entity)[0]));
  }

  TEST_F(SceneFilesTest, ReadsAComponentOfAGame)
  {
    _scene.GetComponentFormats().Add(ComponentFormat::Of<Health>(
      "Health",
      [](const DataReader &reader, Health &health)
      {
        float points = 100.0f;
        reader.Read("points", points);
        health.points = static_cast<int>(points);
      },
      [](const Health &health, DataValue &map)
      {
        map.Set("points", DataValue::Number(health.points));
      }));
    Write("entities:\n  - name: hero\n    components:\n      Health:\n        points: 40\n");

    _scene.Populate(_world.store);

    EXPECT_EQ(_world.store.Get<Health>(_world.store.FindEntity("hero"))->points, 40);
  }

  TEST_F(SceneFilesTest, AFileThatDoesNotExistIsReportedAndCouldNotBeRead)
  {
    EXPECT_FALSE(_scene.Populate(_world.store));
    EXPECT_TRUE(_scene.CouldNotBeRead());
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error,
      "The scene assets://scenes/test.scene.yml cannot be read: there is no such file, or it cannot be opened"))
      << _logger->Messages(LogLevel::Error);
  }

  TEST_F(SceneFilesTest, AFileThatHoldsNoDocumentCouldNotBeRead)
  {
    Write("entities: [unclosed\n");

    EXPECT_FALSE(_scene.Populate(_world.store));
    EXPECT_TRUE(_scene.CouldNotBeRead());
  }

  TEST_F(SceneFilesTest, AFileWithAProblemWasReadAllTheSame)
  {
    Write("scene: test\nentities:\n  - name: hero\n    components:\n      NoSuchComponent: Default\n");

    EXPECT_FALSE(_scene.Populate(_world.store));
    EXPECT_FALSE(_scene.CouldNotBeRead());
  }

  TEST_F(SceneFilesTest, AFileWithAProblemGivesWhatCouldBeReadAndSaysSo)
  {
    Write(
      "scene: test\n"
      "entities:\n"
      "  - name: good\n"
      "    components:\n"
      "      Transform: Default\n"
      "  - name: bad\n"
      "    components:\n"
      "      Transform:\n"
      "        postion: [1, 2, 3]\n");

    EXPECT_FALSE(_scene.Populate(_world.store));
    EXPECT_NE(_world.store.FindEntity("good"), neon::No_Entity);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "The scene assets://scenes/test.scene.yml has 1 problem, the world holds what could be read"))
      << _logger->Messages(LogLevel::Error);
  }

  // what is wrong with a file

  TEST_F(SceneFilesTest, AComponentThatIsNotKnownIsReportedWithThoseThatAre)
  {
    EXPECT_THAT(
      ProblemsOf("entities:\n  - name: a\n    components:\n      Transfrom: {}\n"),
      HasSubstr("test.scene.yml:4: component 'Transfrom' of entity 'a' is not known. "
        "Known are: Transform, Renderable, Geometry, Rope, Camera, Light, Sky, Spectator"));
  }

  TEST_F(SceneFilesTest, ANameInAComponentThatIsNotKnownIsReportedWithItsLine)
  {
    EXPECT_THAT(
      ProblemsOf("entities:\n  - name: a\n    components:\n      Transform:\n        postion: [1, 2, 3]\n"),
      HasSubstr("test.scene.yml:5: 'postion' is not known to Transform of entity 'a'. "
        "Known are: position, rotation, scale"));
  }

  TEST_F(SceneFilesTest, AValueOfTheWrongKindIsReportedWithItsLine)
  {
    EXPECT_THAT(
      ProblemsOf("entities:\n  - name: a\n    components:\n      Transform:\n        position: here\n"),
      HasSubstr("test.scene.yml:5: 'position' of Transform of entity 'a' is text, "
        "where a list of 3 numbers was expected"));
  }

  TEST_F(SceneFilesTest, ANameInAnEntityThatIsNotKnownIsReported)
  {
    EXPECT_THAT(
      ProblemsOf("entities:\n  - name: a\n    component: {}\n"),
      HasSubstr("test.scene.yml:3: 'component' is not known to entity 'a'. "
        "Known are: name, prefab, components, children"));
  }

  TEST_F(SceneFilesTest, ANameAtTheTopThatIsNotKnownIsReported)
  {
    EXPECT_THAT(ProblemsOf("entites: []\n"), HasSubstr("'entites' is not known to the scene"));
  }

  TEST_F(SceneFilesTest, TwoEntitiesOfOneNameNextToEachOtherAreReported)
  {
    EXPECT_THAT(
      ProblemsOf("entities:\n  - name: a\n  - name: a\n"),
      HasSubstr("test.scene.yml:3: entity 'a' shares its name with another entity next to it"));
  }

  TEST_F(SceneFilesTest, TwoEntitiesOfOneNameBelowDifferentParentsAreFine)
  {
    Write("entities:\n  - name: a\n    children:\n      - name: x\n  - name: b\n    children:\n      - name: x\n");

    _scene.Populate(_world.store);

    EXPECT_NE(_world.store.FindEntity("a/x"), _world.store.FindEntity("b/x"));
  }

  TEST_F(SceneFilesTest, ASlashInANameIsReported)
  {
    EXPECT_THAT(ProblemsOf("entities:\n  - name: a/b\n"), HasSubstr("holds a '/'"));
  }

  TEST_F(SceneFilesTest, AVersionThisEngineDoesNotReadIsReported)
  {
    EXPECT_THAT(
      ProblemsOf("version: 2\nentities: []\n"),
      HasSubstr("test.scene.yml:1: the scene has version 2, and this engine reads up to version 1"));
  }

  TEST_F(SceneFilesTest, ATextThatIsNotYamlIsReported)
  {
    EXPECT_THAT(ProblemsOf("entities: [\n"), HasSubstr("test.scene.yml:"));
  }

  TEST_F(SceneFilesTest, AnEntityWithoutANameIsCalledByItsPlace)
  {
    EXPECT_THAT(
      ProblemsOf("entities:\n  - name: a\n  - components:\n      Camera:\n        fov: wide\n"),
      HasSubstr("'fov' of Camera of entity 2 is text"));
  }

  TEST_F(SceneFilesTest, EveryProblemOfAFileIsReportedAndNotOnlyTheFirst)
  {
    ProblemsOf(
      "entities:\n  - name: a\n    components:\n      Transform:\n        postion: [1]\n"
      "        scale: big\n      Nope: {}\n");

    // the three problems, and the line that counts them
    EXPECT_EQ(_logger->Count(LogLevel::Error), 4u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(SceneFilesTest, SaysHowManyProblemsAFileHasAndGoesOn)
  {
    Write("entites: []\n");

    EXPECT_FALSE(_scene.Populate(_world.store));

    const std::string said = _logger->Messages(LogLevel::Error);
    EXPECT_THAT(said, HasSubstr("'entites' is not known"));
    EXPECT_THAT(said, HasSubstr("The scene assets://scenes/test.scene.yml has 1 problem, the world holds what could be read"));
  }

  // components that need each other

  TEST_F(SceneFilesTest, ARenderableWithoutATransformIsGivenOneAndTheWarningHasItsLine)
  {
    Write(
      "entities:\n"
      "  - name: bear\n"
      "    components:\n"
      "      Renderable:\n"
      "        model: assets://models/bear.obj\n"
      "        shader: engine://shaders/basic-lit\n");

    // what was mended is no problem of the scene: the scene was read in full
    EXPECT_TRUE(_scene.Populate(_world.store)) << _logger->Messages(LogLevel::Error);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
    EXPECT_THAT(_logger->Messages(LogLevel::Warn), HasSubstr(
                  "test.scene.yml:2: entity 'bear' has a Renderable and no Transform; one was added at the origin"));

    const Entity bear = _world.store.FindEntity("bear");
    ASSERT_NE(bear, No_Entity);
    ASSERT_TRUE(_world.store.Has<Transform>(bear));
    EXPECT_EQ(_world.store.Get<Transform>(bear)->position, glm::vec3(0.0f));
    EXPECT_EQ(_world.store.Get<Transform>(bear)->scale, glm::vec3(1.0f));
  }

  TEST_F(SceneFilesTest, ACameraOrALightBelowAnotherEntityIsGivenATransformWhereItsParentIs)
  {
    Write(
      "entities:\n"
      "  - name: player\n"
      "    components:\n"
      "      Transform: Default\n"
      "    children:\n"
      "      - name: camera\n"
      "        components:\n"
      "          Camera: Default\n"
      "      - name: torch\n"
      "        components:\n"
      "          Light: Default\n");

    EXPECT_TRUE(_scene.Populate(_world.store)) << _logger->Messages(LogLevel::Error);

    const std::string said = _logger->Messages(LogLevel::Warn);
    EXPECT_THAT(said, HasSubstr(
                  "test.scene.yml:6: child 'camera' of entity 'player' has a Camera and no Transform; "
                  "one was added where its parent is"));
    EXPECT_THAT(said, HasSubstr(
                  "test.scene.yml:9: child 'torch' of entity 'player' has a Light and no Transform; "
                  "one was added where its parent is"));
    EXPECT_TRUE(_world.store.Has<Transform>(_world.store.FindEntity("player/camera")));
    EXPECT_TRUE(_world.store.Has<Transform>(_world.store.FindEntity("player/torch")));
  }

  TEST_F(SceneFilesTest, ARenderableThatIsTurnedOffIsGivenATransformToo)
  {
    Write(
      "entities:\n"
      "  - name: hidden\n"
      "    components:\n"
      "      Renderable:\n"
      "        shader: engine://shaders/basic-lit\n"
      "        enabled: false\n");

    EXPECT_TRUE(_scene.Populate(_world.store)) << _logger->Messages(LogLevel::Error);

    // it is drawn once a pool or a script turns it on
    EXPECT_THAT(_logger->Messages(LogLevel::Warn), HasSubstr("entity 'hidden' has a Renderable and no Transform"));
    EXPECT_TRUE(_world.store.Has<Transform>(_world.store.FindEntity("hidden")));
  }

  TEST_F(SceneFilesTest, ARenderableWithATransformIsFine)
  {
    Write(
      "entities:\n"
      "  - name: bear\n"
      "    components:\n"
      "      Transform: Default\n"
      "      Renderable:\n"
      "        shader: engine://shaders/basic-lit\n");

    EXPECT_TRUE(_scene.Populate(_world.store)) << _logger->Messages(LogLevel::Error);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 0u) << _logger->Messages(LogLevel::Warn);
  }

  TEST_F(SceneFilesTest, ATransformThatWasAddedIsSaved)
  {
    Write("entities:\n  - name: bear\n    components:\n      Renderable:\n        shader: engine://shaders/basic-lit\n");
    _scene.Populate(_world.store);

    ASSERT_TRUE(_scene.Save(_world.store, "saved", "user://saved.scene.yml"));

    EXPECT_THAT(ReadFile("user://saved.scene.yml"), HasSubstr("      Transform: Default\n"));
  }

  TEST_F(SceneFilesTest, AGeometryWithoutARenderableIsAProblemAndIsNotMended)
  {
    _world.store.Register<neon::Geometry>("Geometry");

    // a Renderable of its defaults has no shader, which the renderer refuses
    EXPECT_THAT(
      ProblemsOf("entities:\n  - name: box\n    components:\n      Transform: Default\n      Geometry: Default\n"),
      HasSubstr("test.scene.yml:2: entity 'box' has a Geometry and no Renderable, so its shape is drawn nowhere"));
    EXPECT_FALSE(_world.store.Has<Renderable>(_world.store.FindEntity("box")));
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "The scene assets://scenes/test.scene.yml has 1 problem, the world holds what could be read"))
      << _logger->Messages(LogLevel::Error);
  }

  // saving

  TEST_F(SceneFilesTest, SavesEntitiesInTheOrderTheyWereCreatedIn)
  {
    auto &store = _world.store;
    store.Set(store.CreateEntity("zebra"), Spectator{});
    store.Set(store.CreateEntity("apple"), Transform{});
    store.CreateEntity("mango", store.FindEntity("zebra"));

    ASSERT_TRUE(_scene.Save(store, "saved", "user://saved.scene.yml"));

    EXPECT_EQ(
      ReadFile("user://saved.scene.yml"),
      "scene: saved\n"
      "version: 1\n"
      "\n"
      "entities:\n"
      "  - name: zebra\n"
      "    components:\n"
      "      Spectator: Default\n"
      "    children:\n"
      "      - name: mango\n"
      "        components: {}\n"
      "\n"
      "  - name: apple\n"
      "    components:\n"
      "      Transform: Default\n");
  }

  TEST_F(SceneFilesTest, LeavesOutAComponentThatHasNoFormat)
  {
    auto &store = _world.store;
    store.Set(store.CreateEntity("hero"), Health{40});

    ASSERT_TRUE(_scene.Save(store, "saved", "user://saved.scene.yml"));

    EXPECT_THAT(ReadFile("user://saved.scene.yml"), ::testing::Not(HasSubstr("Health")));
  }

  TEST_F(SceneFilesTest, DoesNotSaveToWhereNothingCanBeWritten)
  {
    EXPECT_FALSE(_scene.Save(_world.store, "saved", "assets://saved.scene.yml"));
  }

  TEST_F(SceneFilesTest, ASceneThatIsSavedAndLoadedAndSavedAgainIsTheSameText)
  {
    Write(SmallScene());
    _scene.Populate(_world.store);
    ASSERT_TRUE(_scene.Save(_world.store, "demo", "user://first.scene.yml"));

    World second(_logger);
    SceneFile saved(&_files, &_yaml, "user://first.scene.yml", _logger);
    saved.Populate(second.store);
    ASSERT_TRUE(saved.Save(second.store, "demo", "user://second.scene.yml"));

    EXPECT_EQ(ReadFile("user://first.scene.yml"), ReadFile("user://second.scene.yml"));
  }

  TEST_F(SceneFilesTest, AComponentIsTheSameAfterSavingAndLoading)
  {
    Write(SmallScene());
    _scene.Populate(_world.store);
    ASSERT_TRUE(_scene.Save(_world.store, "demo", "user://saved.scene.yml"));

    World second(_logger);
    SceneFile saved(&_files, &_yaml, "user://saved.scene.yml", _logger);
    saved.Populate(second.store);

    const auto &before = _world.store.Get<Renderable>(_world.store.FindEntity("floor"))->render_info;
    const auto &after = second.store.Get<Renderable>(second.store.FindEntity("floor"))->render_info;
    EXPECT_EQ(before.model_path, after.model_path);
    EXPECT_EQ(before.texture_paths, after.texture_paths);
    EXPECT_EQ(before.scale_textures, after.scale_textures);
    EXPECT_EQ(before.material_info.color.g, after.material_info.color.g);
    EXPECT_EQ(before.material_info.shininess, after.material_info.shininess);
  }
}
