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

#include <neon/common/transform.hpp>
#include <neon/data/ryml-document-format.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/camera.hpp>
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

  TEST_F(SceneFilesTest, AFileThatDoesNotExistIsReportedAndTheWorldStartsEmpty)
  {
    EXPECT_FALSE(_scene.Populate(_world.store));
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "The scene assets://scenes/test.scene.yml cannot be read, the world starts empty"))
      << _logger->Messages(LogLevel::Error);
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
        "Known are: Transform, Renderable, Geometry, Camera, Light, Spectator"));
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
