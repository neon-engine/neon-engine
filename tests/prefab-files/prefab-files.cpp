// Prefabs placed in a scene, as an application puts it together: SceneFile
// with the document format for YAML and the entity store of Flecs, and the
// prefab recipes next to the scene in a file system kept in memory.
//
// scene-files.cpp is where a text becomes a world. This is where an entity
// described once becomes several, and where what a scene writes on top of
// a prefab lands.

#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/data/ryml-document-format.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/camera.hpp>
#include <neon/world-system/ecs/components/light.hpp>
#include <neon/world-system/ecs/components/prefab.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>
#include <neon/world-system/ecs/components/spectator.hpp>
#include <neon/world-system/ecs/scene-file/prefab-file.hpp>
#include <neon/world-system/ecs/scene-file/prefab-files.hpp>
#include <neon/world-system/ecs/scene-file/scene-file.hpp>
#include <neon/world-system/flecs-entity-store.hpp>

namespace
{
  using neon::Camera;
  using neon::DataValue;
  using neon::Entity;
  using neon::EntityStore;
  using neon::Flecs_EntityStore;
  using neon::Light;
  using neon::No_Entity;
  using neon::Prefab;
  using neon::PrefabFile;
  using neon::PrefabFiles;
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
  using ::testing::Not;

  /// A store with the components of the engine that the tests place.
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
    }

    ~World()
    {
      store.CleanUp();
    }

    [[nodiscard]] std::vector<std::string> NamesBelow(const Entity parent) const
    {
      std::vector<std::string> names;
      for (const auto entity : store.GetChildren(parent)) { names.push_back(store.GetName(entity)); }
      return names;
    }
  };

  class PrefabFilesTest : public ::testing::Test
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

    /// A prefab recipe at `assets://prefabs/<name>.prefab.yml`.
    void WritePrefab(const std::string &name, const std::string &text)
    {
      _files.AddNativeFile("/assets/prefabs/" + name + ".prefab.yml", text);
    }

    void WriteScene(const std::string &text)
    {
      _files.AddNativeFile("/assets/scenes/test.scene.yml", text);
    }

    /// Loads the scene, which is expected to be fine.
    void Load(const std::string &text)
    {
      WriteScene(text);
      EXPECT_TRUE(_scene.Populate(_world.store)) << _logger->Messages(LogLevel::Error);
    }

    /// Loads a scene that is expected to have problems. Returns the errors
    /// that were logged, one per line.
    std::string ProblemsOf(const std::string &text)
    {
      WriteScene(text);
      _logger->Clear();
      EXPECT_FALSE(_scene.Populate(_world.store)) << text;
      return _logger->Messages(LogLevel::Error);
    }

    Entity Find(const std::string &path)
    {
      const auto entity = _world.store.FindEntity(path);
      EXPECT_NE(entity, No_Entity) << path;
      return entity;
    }

    std::string ReadFile(const std::string &path)
    {
      std::string text;
      EXPECT_TRUE(_files.ReadText(path, text)) << path;
      return text;
    }

    /// A wall: something drawn, standing a little above the floor.
    void WriteWall()
    {
      WritePrefab(
        "wall",
        "version: 1\n"
        "prefab: wall\n"
        "entity:\n"
        "  components:\n"
        "    Transform:\n"
        "      position: [0, 0.5, 0]\n"
        "      scale: [1, 2.5, 1]\n"
        "    Renderable:\n"
        "      model: assets://models/wall.obj\n"
        "      shader: assets://shaders/basic-lit\n"
        "      textures:\n"
        "        - assets://textures/brick.png\n"
        "        - assets://textures/moss.png\n"
        "      material:\n"
        "        color: [0.5, 0.5, 0.5]\n"
        "        shininess: 8\n"
        "    Spectator: Default\n");
    }
  };

  // the file

  TEST_F(PrefabFilesTest, ReadsAPrefab)
  {
    WriteWall();
    std::vector<std::string> errors;
    PrefabFile prefab(&_files, &_yaml, "assets://prefabs/wall.prefab.yml");

    EXPECT_TRUE(prefab.Read(errors));

    EXPECT_THAT(errors, ::testing::IsEmpty());
    EXPECT_EQ(prefab.GetName(), "wall");
    EXPECT_TRUE(prefab.GetEntity().IsMap());
    EXPECT_NE(prefab.GetEntity().Find("components"), nullptr);
  }

  TEST_F(PrefabFilesTest, AFileThatIsNotThereCannotBeRead)
  {
    std::vector<std::string> errors;
    PrefabFile prefab(&_files, &_yaml, "assets://prefabs/nope.prefab.yml");

    EXPECT_FALSE(prefab.Read(errors));
    EXPECT_TRUE(prefab.GetEntity().IsEmpty());
  }

  TEST_F(PrefabFilesTest, ANameAtTheTopThatIsNotKnownIsReported)
  {
    WritePrefab("odd", "prefab: odd\nentities: []\n");
    std::vector<std::string> errors;
    PrefabFile prefab(&_files, &_yaml, "assets://prefabs/odd.prefab.yml");

    EXPECT_TRUE(prefab.Read(errors));

    EXPECT_THAT(errors, ElementsAre(
      HasSubstr("odd.prefab.yml:1: 'entity' is missing. It holds the entity the prefab describes"),
      HasSubstr("odd.prefab.yml:2: 'entities' is not known to the prefab. Known are: prefab, version, entity")));
  }

  TEST_F(PrefabFilesTest, AVersionThisEngineDoesNotReadIsReported)
  {
    WritePrefab("new", "version: 2\nentity: {}\n");
    std::vector<std::string> errors;
    PrefabFile prefab(&_files, &_yaml, "assets://prefabs/new.prefab.yml");

    EXPECT_TRUE(prefab.Read(errors));

    EXPECT_THAT(errors, ElementsAre(
      HasSubstr("new.prefab.yml:1: the prefab has version 2, and this engine reads up to version 1")));
  }

  // placing

  TEST_F(PrefabFilesTest, PlacesAPrefabTwiceUnderTwoNames)
  {
    WriteWall();

    Load(
      "entities:\n"
      "  - name: north\n"
      "    prefab: assets://prefabs/wall.prefab.yml\n"
      "  - name: south\n"
      "    prefab: assets://prefabs/wall.prefab.yml\n");

    EXPECT_THAT(_world.NamesBelow(No_Entity), ElementsAre("north", "south"));
    for (const auto *name : {"north", "south"})
    {
      const auto wall = Find(name);
      EXPECT_EQ(_world.store.Get<Transform>(wall)->scale, glm::vec3(1.0f, 2.5f, 1.0f)) << name;
      EXPECT_EQ(_world.store.Get<Renderable>(wall)->render_info.model_path, "assets://models/wall.obj") << name;
      EXPECT_TRUE(_world.store.Has<Spectator>(wall)) << name;
    }
  }

  TEST_F(PrefabFilesTest, ThePrefabComponentHoldsThePath)
  {
    WriteWall();

    Load(
      "entities:\n"
      "  - name: wall\n"
      "    prefab: assets://prefabs/wall.prefab.yml\n"
      "  - name: plain\n");

    EXPECT_EQ(_world.store.Get<Prefab>(Find("wall"))->path, "assets://prefabs/wall.prefab.yml");
    EXPECT_FALSE(_world.store.Has<Prefab>(Find("plain")));
  }

  TEST_F(PrefabFilesTest, TheScenesComponentsLandOnTopOfThePrefabs)
  {
    WriteWall();

    Load(
      "entities:\n"
      "  - name: wall\n"
      "    prefab: assets://prefabs/wall.prefab.yml\n"
      "    components:\n"
      "      Transform:\n"
      "        rotation: [0, 90, 0]\n"
      "      Camera:\n"
      "        fov: 60\n");

    const auto wall = Find("wall");
    const auto *transform = _world.store.Get<Transform>(wall);
    EXPECT_EQ(transform->position, glm::vec3(0.0f, 0.5f, 0.0f)) << "the prefab's position is kept";
    EXPECT_EQ(transform->scale, glm::vec3(1.0f, 2.5f, 1.0f)) << "the prefab's scale is kept";
    EXPECT_EQ(transform->rotation.yaw, 90.0f) << "the scene's rotation is read";
    EXPECT_EQ(_world.store.Get<Camera>(wall)->fov, 60.0f) << "a component the prefab lacks is added";
  }

  TEST_F(PrefabFilesTest, AGroupMergesByField)
  {
    WriteWall();

    Load(
      "entities:\n"
      "  - name: wall\n"
      "    prefab: assets://prefabs/wall.prefab.yml\n"
      "    components:\n"
      "      Renderable:\n"
      "        material:\n"
      "          shininess: 32\n");

    const auto &material = _world.store.Get<Renderable>(Find("wall"))->render_info.material_info;
    EXPECT_EQ(material.shininess, 32.0f);
    EXPECT_EQ(material.color.r, 0.5f) << "the prefab's colour is kept";
  }

  TEST_F(PrefabFilesTest, KeepsThePrefabsListWhenTheSceneNamesNone)
  {
    WriteWall();

    Load(
      "entities:\n"
      "  - name: wall\n"
      "    prefab: assets://prefabs/wall.prefab.yml\n"
      "    components:\n"
      "      Renderable:\n"
      "        scale_textures: true\n");

    const auto &render_info = _world.store.Get<Renderable>(Find("wall"))->render_info;
    EXPECT_TRUE(render_info.scale_textures);
    EXPECT_THAT(render_info.texture_paths, ElementsAre("assets://textures/brick.png", "assets://textures/moss.png"));
  }

  TEST_F(PrefabFilesTest, ReplacesThePrefabsListWholeWhenTheSceneNamesOne)
  {
    WriteWall();

    Load(
      "entities:\n"
      "  - name: wall\n"
      "    prefab: assets://prefabs/wall.prefab.yml\n"
      "    components:\n"
      "      Renderable:\n"
      "        textures:\n"
      "          - assets://textures/steel.png\n");

    EXPECT_THAT(
      _world.store.Get<Renderable>(Find("wall"))->render_info.texture_paths,
      ElementsAre("assets://textures/steel.png"));
  }

  TEST_F(PrefabFilesTest, DefaultOnTopOfAPrefabChangesNothing)
  {
    WriteWall();

    Load(
      "entities:\n"
      "  - name: wall\n"
      "    prefab: assets://prefabs/wall.prefab.yml\n"
      "    components:\n"
      "      Transform: Default\n");

    EXPECT_EQ(_world.store.Get<Transform>(Find("wall"))->scale, glm::vec3(1.0f, 2.5f, 1.0f));
  }

  TEST_F(PrefabFilesTest, NothingTakesAComponentAway)
  {
    WriteWall();

    Load(
      "entities:\n"
      "  - name: wall\n"
      "    prefab: assets://prefabs/wall.prefab.yml\n"
      "    components:\n"
      "      Spectator: ~\n"
      "      Camera: ~\n");

    const auto wall = Find("wall");
    EXPECT_FALSE(_world.store.Has<Spectator>(wall));
    EXPECT_FALSE(_world.store.Has<Camera>(wall)) << "taking away what is not there is fine";
    EXPECT_TRUE(_world.store.Has<Renderable>(wall));
  }

  TEST_F(PrefabFilesTest, NothingForAComponentThatIsNotKnownIsStillReported)
  {
    WriteWall();

    EXPECT_THAT(
      ProblemsOf(
        "entities:\n"
        "  - name: wall\n"
        "    prefab: assets://prefabs/wall.prefab.yml\n"
        "    components:\n"
        "      Spectre: ~\n"),
      HasSubstr("test.scene.yml:5: component 'Spectre' of entity 'wall' is not known"));
  }

  // children

  TEST_F(PrefabFilesTest, OverridesAChildByNameAndAddsANewOne)
  {
    WritePrefab(
      "lamp",
      "prefab: lamp\n"
      "entity:\n"
      "  components:\n"
      "    Transform: Default\n"
      "  children:\n"
      "    - name: bulb\n"
      "      components:\n"
      "        Transform:\n"
      "          position: [0, 2, 0]\n"
      "        Light:\n"
      "          type: point\n"
      "          diffuse: [1, 1, 1]\n"
      "    - name: shade\n"
      "      components:\n"
      "        Transform: Default\n");

    Load(
      "entities:\n"
      "  - name: lamp\n"
      "    prefab: assets://prefabs/lamp.prefab.yml\n"
      "    children:\n"
      "      - name: bulb\n"
      "        components:\n"
      "          Light:\n"
      "            diffuse: [1, 0.5, 0]\n"
      "      - name: switch\n"
      "        components:\n"
      "          Transform: Default\n");

    EXPECT_THAT(_world.NamesBelow(Find("lamp")), ElementsAre("bulb", "shade", "switch"));

    const auto *light = _world.store.Get<Light>(Find("lamp/bulb"));
    EXPECT_EQ(light->source.light_type, neon::LightType::Point) << "the prefab's type is kept";
    EXPECT_EQ(light->source.diffuse.g, 0.5f) << "the scene's colour is read";
    EXPECT_EQ(_world.store.Get<Transform>(Find("lamp/bulb"))->position.y, 2.0f);
  }

  TEST_F(PrefabFilesTest, AChildOfAPlainEntityStillMayNotShareItsName)
  {
    EXPECT_THAT(
      ProblemsOf(
        "entities:\n"
        "  - name: a\n"
        "    children:\n"
        "      - name: x\n"
        "      - name: x\n"),
      HasSubstr("test.scene.yml:5: entity 'x' shares its name with another entity next to it"));
  }

  TEST_F(PrefabFilesTest, ANameTwiceAmongTheChildrenOfAPrefabIsReported)
  {
    WritePrefab(
      "twins",
      "entity:\n"
      "  children:\n"
      "    - name: x\n"
      "    - name: x\n");

    EXPECT_THAT(
      ProblemsOf("entities:\n  - name: a\n    prefab: assets://prefabs/twins.prefab.yml\n"),
      HasSubstr("twins.prefab.yml:4: entity 'x' shares its name with another entity next to it"));
  }

  // prefabs within prefabs

  TEST_F(PrefabFilesTest, APrefabPlacesAPrefab)
  {
    WriteWall();
    WritePrefab(
      "room",
      "prefab: room\n"
      "entity:\n"
      "  children:\n"
      "    - name: north\n"
      "      prefab: assets://prefabs/wall.prefab.yml\n"
      "      components:\n"
      "        Transform:\n"
      "          position: [0, 0, -3]\n"
      "    - name: south\n"
      "      prefab: assets://prefabs/wall.prefab.yml\n"
      "      components:\n"
      "        Transform:\n"
      "          position: [0, 0, 3]\n");

    Load(
      "entities:\n"
      "  - name: room\n"
      "    prefab: assets://prefabs/room.prefab.yml\n"
      "    children:\n"
      "      - name: south\n"
      "        components:\n"
      "          Transform:\n"
      "            rotation: [0, 180, 0]\n");

    EXPECT_THAT(_world.NamesBelow(Find("room")), ElementsAre("north", "south"));
    EXPECT_EQ(_world.store.Get<Prefab>(Find("room"))->path, "assets://prefabs/room.prefab.yml");
    EXPECT_EQ(_world.store.Get<Prefab>(Find("room/north"))->path, "assets://prefabs/wall.prefab.yml");

    const auto *south = _world.store.Get<Transform>(Find("room/south"));
    EXPECT_EQ(south->position.z, 3.0f) << "from the room";
    EXPECT_EQ(south->scale.y, 2.5f) << "from the wall";
    EXPECT_EQ(south->rotation.yaw, 180.0f) << "from the scene";
  }

  TEST_F(PrefabFilesTest, APrefabStartsFromAnotherPrefab)
  {
    WriteWall();
    WritePrefab(
      "tall-wall",
      "entity:\n"
      "  prefab: assets://prefabs/wall.prefab.yml\n"
      "  components:\n"
      "    Transform:\n"
      "      scale: [1, 5, 1]\n");

    Load("entities:\n  - name: wall\n    prefab: assets://prefabs/tall-wall.prefab.yml\n");

    const auto wall = Find("wall");
    EXPECT_EQ(_world.store.Get<Transform>(wall)->scale, glm::vec3(1.0f, 5.0f, 1.0f));
    EXPECT_EQ(_world.store.Get<Transform>(wall)->position.y, 0.5f);
    EXPECT_EQ(_world.store.Get<Prefab>(wall)->path, "assets://prefabs/tall-wall.prefab.yml")
      << "the outer prefab is what the entity came from";
  }

  TEST_F(PrefabFilesTest, ALoopIsRefusedWithEveryFileNamed)
  {
    WritePrefab("p", "entity:\n  children:\n    - name: q\n      prefab: assets://prefabs/q.prefab.yml\n");
    WritePrefab("q", "entity:\n  components:\n    Transform: Default\n  children:\n    - name: p\n      prefab: assets://prefabs/p.prefab.yml\n");

    const auto problems = ProblemsOf("entities:\n  - name: top\n    prefab: assets://prefabs/p.prefab.yml\n");

    EXPECT_THAT(problems, HasSubstr(
      "assets://prefabs/q.prefab.yml:6: 'prefab' of entity 'p' places assets://prefabs/p.prefab.yml within itself: "
      "assets://scenes/test.scene.yml:3 places assets://prefabs/p.prefab.yml, "
      "assets://prefabs/p.prefab.yml:4 places assets://prefabs/q.prefab.yml, "
      "assets://prefabs/q.prefab.yml:6 places assets://prefabs/p.prefab.yml again"));
    EXPECT_TRUE(_world.store.Has<Transform>(Find("top/q"))) << "what is above the loop is placed";
    EXPECT_NE(Find("top/q/p"), No_Entity) << "the entity that closes the loop is there, and empty";
  }

  TEST_F(PrefabFilesTest, APrefabThatPlacesItselfIsRefused)
  {
    WritePrefab("self", "entity:\n  prefab: assets://prefabs/self.prefab.yml\n");

    EXPECT_THAT(
      ProblemsOf("entities:\n  - name: a\n    prefab: assets://prefabs/self.prefab.yml\n"),
      HasSubstr("self.prefab.yml:2: 'prefab' of entity 'a' places assets://prefabs/self.prefab.yml within itself"));
  }

  // what is wrong

  TEST_F(PrefabFilesTest, AMissingPrefabIsReportedWithTheScenesLine)
  {
    const auto problems = ProblemsOf(
      "entities:\n"
      "  - name: a\n"
      "    components:\n"
      "      Transform: Default\n"
      "    prefab: assets://prefabs/nope.prefab.yml\n");

    EXPECT_THAT(problems, HasSubstr(
      "test.scene.yml:5: 'prefab' of entity 'a' is assets://prefabs/nope.prefab.yml, which cannot be read"));
    EXPECT_TRUE(_world.store.Has<Transform>(Find("a"))) << "the rest of the entity is read";
  }

  TEST_F(PrefabFilesTest, APrefabThatIsNotYamlIsReportedWithItsOwnLineAndTheScenes)
  {
    WritePrefab("broken", "entity: [\n");

    const auto problems = ProblemsOf("entities:\n  - name: a\n    prefab: assets://prefabs/broken.prefab.yml\n");

    EXPECT_THAT(problems, HasSubstr("broken.prefab.yml:"));
    EXPECT_THAT(problems, HasSubstr("test.scene.yml:3: 'prefab' of entity 'a' is assets://prefabs/broken.prefab.yml, which cannot be read"));
  }

  TEST_F(PrefabFilesTest, ANameInsideAPrefabThatIsNotKnownIsReportedWithThePrefabsLine)
  {
    WritePrefab("odd", "entity:\n  components:\n    Transform:\n      postion: [1, 2, 3]\n");

    EXPECT_THAT(
      ProblemsOf("entities:\n  - name: a\n    prefab: assets://prefabs/odd.prefab.yml\n"),
      HasSubstr("assets://prefabs/odd.prefab.yml:4: 'postion' is not known to Transform of entity 'a'. "
        "Known are: position, rotation, scale"));
  }

  TEST_F(PrefabFilesTest, TheEntityOfAPrefabHasNoNameOfItsOwn)
  {
    WritePrefab("named", "entity:\n  name: wall\n");

    EXPECT_THAT(
      ProblemsOf("entities:\n  - name: a\n    prefab: assets://prefabs/named.prefab.yml\n"),
      HasSubstr("named.prefab.yml:2: 'name' is not known to entity 'a'. Known are: prefab, components, children"));
  }

  TEST_F(PrefabFilesTest, WhatIsWrongInsideAPrefabIsSaidOnceThoughItIsPlacedTwice)
  {
    WritePrefab("odd", "entity:\n  components:\n    Transform:\n      postion: [1, 2, 3]\n");

    ProblemsOf(
      "entities:\n"
      "  - name: a\n"
      "    prefab: assets://prefabs/odd.prefab.yml\n"
      "  - name: b\n"
      "    prefab: assets://prefabs/odd.prefab.yml\n");

    // the problem, and the line that counts the problems
    EXPECT_EQ(_logger->Count(LogLevel::Error), 2u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_world.store.Has<Transform>(Find("b"))) << "the second is placed all the same";
  }

  TEST_F(PrefabFilesTest, APrefabIsReadOnceHoweverOftenItIsAskedFor)
  {
    WriteWall();
    std::vector<std::string> errors;
    PrefabFiles prefabs(&_files, &_yaml);

    const auto *first = prefabs.Find("assets://prefabs/wall.prefab.yml", errors);
    const auto *second = prefabs.Find("assets://prefabs/wall.prefab.yml", errors);

    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first, second);
    EXPECT_EQ(prefabs.Find("assets://prefabs/nope.prefab.yml", errors), nullptr);
    EXPECT_EQ(prefabs.Find("assets://prefabs/nope.prefab.yml", errors), nullptr);
    EXPECT_THAT(errors, ::testing::IsEmpty()) << "a file that is not there is reported where it is named";
  }

  // saving

  TEST_F(PrefabFilesTest, SavesThePrefabNextToTheName)
  {
    WritePrefab("thing", "entity:\n  components:\n    Spectator: Default\n");
    Load("entities:\n  - name: a\n    prefab: assets://prefabs/thing.prefab.yml\n");

    ASSERT_TRUE(_scene.Save(_world.store, "saved", "user://saved.scene.yml"));

    EXPECT_EQ(
      ReadFile("user://saved.scene.yml"),
      "scene: saved\n"
      "version: 1\n"
      "\n"
      "entities:\n"
      "  - name: a\n"
      "    prefab: assets://prefabs/thing.prefab.yml\n"
      "    components:\n"
      "      Spectator: Default\n");
  }

  TEST_F(PrefabFilesTest, ASceneWithPrefabsIsTheSameAfterSavingAndLoading)
  {
    WriteWall();
    Load(
      "entities:\n"
      "  - name: wall\n"
      "    prefab: assets://prefabs/wall.prefab.yml\n"
      "    components:\n"
      "      Transform:\n"
      "        rotation: [0, 90, 0]\n");
    ASSERT_TRUE(_scene.Save(_world.store, "saved", "user://saved.scene.yml"));

    World second(_logger);
    SceneFile saved(&_files, &_yaml, "user://saved.scene.yml", _logger);
    EXPECT_TRUE(saved.Populate(second.store)) << _logger->Messages(LogLevel::Error);

    const auto wall = second.store.FindEntity("wall");
    ASSERT_NE(wall, No_Entity);
    EXPECT_EQ(second.store.Get<Transform>(wall)->rotation.yaw, 90.0f);
    EXPECT_EQ(second.store.Get<Transform>(wall)->scale.y, 2.5f);
    EXPECT_EQ(second.store.Get<Prefab>(wall)->path, "assets://prefabs/wall.prefab.yml");
  }

  // taking a child away

  TEST_F(PrefabFilesTest, NothingTakesAChildAway)
  {
    WritePrefab(
      "lamp",
      "entity:\n"
      "  children:\n"
      "    - name: bulb\n"
      "      components:\n"
      "        Light: Default\n"
      "      children:\n"
      "        - name: filament\n"
      "    - name: shade\n"
      "    - name: post\n");

    Load(
      "entities:\n"
      "  - name: lamp\n"
      "    prefab: assets://prefabs/lamp.prefab.yml\n"
      "    children:\n"
      "      - bulb: ~\n"
      "      - name: switch\n");

    // in no particular order: the store does not keep the order of creation
    // once an entity among them was destroyed
    EXPECT_THAT(_world.NamesBelow(Find("lamp")), ::testing::UnorderedElementsAre("shade", "post", "switch"));
    EXPECT_EQ(_world.store.FindEntity("lamp/bulb/filament"), No_Entity) << "with everything below it";
  }

  TEST_F(PrefabFilesTest, AChildOfAChildIsTakenAwayThroughItsParent)
  {
    WritePrefab(
      "lamp",
      "entity:\n"
      "  children:\n"
      "    - name: bulb\n"
      "      children:\n"
      "        - name: filament\n"
      "        - name: glass\n");

    Load(
      "entities:\n"
      "  - name: lamp\n"
      "    prefab: assets://prefabs/lamp.prefab.yml\n"
      "    children:\n"
      "      - name: bulb\n"
      "        children:\n"
      "          - filament: ~\n");

    EXPECT_THAT(_world.NamesBelow(Find("lamp/bulb")), ElementsAre("glass"));
  }

  TEST_F(PrefabFilesTest, TakingAwayAChildThePrefabDoesNotHaveIsReportedWithTheOnesItHas)
  {
    WritePrefab("lamp", "entity:\n  children:\n    - name: bulb\n    - name: shade\n");

    EXPECT_THAT(
      ProblemsOf(
        "entities:\n"
        "  - name: lamp\n"
        "    prefab: assets://prefabs/lamp.prefab.yml\n"
        "    children:\n"
        "      - post: ~\n"),
      HasSubstr("test.scene.yml:5: child 'post' of entity 'lamp' is not known. Known are: bulb, shade"));
  }

  TEST_F(PrefabFilesTest, TakingAwayAChildOfAPrefabWithoutChildrenIsReported)
  {
    WritePrefab("thing", "entity:\n  components:\n    Spectator: Default\n");

    EXPECT_THAT(
      ProblemsOf(
        "entities:\n"
        "  - name: a\n"
        "    prefab: assets://prefabs/thing.prefab.yml\n"
        "    children:\n"
        "      - post: ~\n"),
      HasSubstr("test.scene.yml:5: child 'post' of entity 'a' is not known. entity 'a' has no children"));
  }

  TEST_F(PrefabFilesTest, OnlyAChildOfAPrefabCanBeTakenAway)
  {
    const auto problems = ProblemsOf(
      "entities:\n"
      "  - name: a\n"
      "    children:\n"
      "      - name: x\n"
      "      - y: ~\n"
      "  - b: ~\n");

    EXPECT_THAT(problems, HasSubstr(
                  "test.scene.yml:5: child 'y' of entity 'a' is taken away, and only a child of a prefab can be"));
    EXPECT_THAT(problems, HasSubstr(
                  "test.scene.yml:6: entity 'b' is taken away, and only a child of a prefab can be"));
    EXPECT_NE(_world.store.FindEntity("a/x"), No_Entity) << "the child stays";
  }

  TEST_F(PrefabFilesTest, AChildTakenAwayAndWrittenAgainSharesItsName)
  {
    WritePrefab("lamp", "entity:\n  children:\n    - name: bulb\n");

    EXPECT_THAT(
      ProblemsOf(
        "entities:\n"
        "  - name: lamp\n"
        "    prefab: assets://prefabs/lamp.prefab.yml\n"
        "    children:\n"
        "      - bulb: ~\n"
        "      - name: bulb\n"),
      HasSubstr("test.scene.yml:6: entity 'bulb' shares its name with another entity next to it"));
  }

  TEST_F(PrefabFilesTest, SavesAChildThatWasTakenAwayTheSameWay)
  {
    WritePrefab(
      "lamp",
      "entity:\n"
      "  components:\n"
      "    Spectator: Default\n"
      "  children:\n"
      "    - name: bulb\n"
      "      components:\n"
      "        Spectator: Default\n"
      "    - name: shade\n"
      "      components:\n"
      "        Spectator: Default\n");
    Load(
      "entities:\n"
      "  - name: lamp\n"
      "    prefab: assets://prefabs/lamp.prefab.yml\n"
      "    children:\n"
      "      - bulb: ~\n");

    ASSERT_TRUE(_scene.Save(_world.store, "saved", "user://saved.scene.yml"));

    EXPECT_EQ(
      ReadFile("user://saved.scene.yml"),
      "scene: saved\n"
      "version: 1\n"
      "\n"
      "entities:\n"
      "  - name: lamp\n"
      "    prefab: assets://prefabs/lamp.prefab.yml\n"
      "    components:\n"
      "      Spectator: Default\n"
      "    children:\n"
      "      - bulb: ~\n"
      "      - name: shade\n"
      "        components:\n"
      "          Spectator: Default\n");

    World second(_logger);
    SceneFile saved(&_files, &_yaml, "user://saved.scene.yml", _logger);
    EXPECT_TRUE(saved.Populate(second.store)) << _logger->Messages(LogLevel::Error);
    EXPECT_THAT(second.NamesBelow(second.store.FindEntity("lamp")), ElementsAre("shade"));
  }

  TEST_F(PrefabFilesTest, SavesAChildOfTheBasePrefabThatWasTakenAwayAndNotOneTheDerivedPrefabTookAway)
  {
    WritePrefab("lamp", "entity:\n  children:\n    - name: bulb\n    - name: shade\n    - name: post\n");
    WritePrefab(
      "short-lamp",
      "entity:\n"
      "  prefab: assets://prefabs/lamp.prefab.yml\n"
      "  children:\n"
      "    - post: ~\n"
      "    - name: base\n");
    Load(
      "entities:\n"
      "  - name: lamp\n"
      "    prefab: assets://prefabs/short-lamp.prefab.yml\n"
      "    children:\n"
      "      - shade: ~\n");
    EXPECT_THAT(_world.NamesBelow(Find("lamp")), ::testing::UnorderedElementsAre("bulb", "base"));

    ASSERT_TRUE(_scene.Save(_world.store, "saved", "user://saved.scene.yml"));

    // the child the scene took away is written as taken away, before the
    // ones the entity has; the one the derived prefab took away is not,
    // since the prefab takes it away on every load
    const auto saved = ReadFile("user://saved.scene.yml");
    EXPECT_THAT(saved, HasSubstr(
                  "    prefab: assets://prefabs/short-lamp.prefab.yml\n"
                  "    components: {}\n"
                  "    children:\n"
                  "      - shade: ~\n"
                  "      - name: "));
    EXPECT_THAT(saved, HasSubstr("      - name: bulb\n"));
    EXPECT_THAT(saved, HasSubstr("      - name: base\n"));
    EXPECT_THAT(saved, Not(HasSubstr("post")));
  }

  // spawning

  TEST_F(PrefabFilesTest, SpawnsAPrefabAtTheTopAsThePrefabDescribesIt)
  {
    WriteWall();
    Load("entities: []\n");

    const Entity wall = _scene.Spawn(_world.store, "assets://prefabs/wall.prefab.yml", No_Entity, DataValue{});

    ASSERT_NE(wall, No_Entity);
    EXPECT_EQ(_world.store.GetParent(wall), No_Entity);
    EXPECT_EQ(_world.store.GetName(wall), "");
    EXPECT_EQ(_world.store.Get<Transform>(wall)->scale.y, 2.5f);
    EXPECT_EQ(_world.store.Get<Renderable>(wall)->render_info.model_path, "assets://models/wall.obj");
    EXPECT_TRUE(_world.store.Has<Spectator>(wall));
    EXPECT_EQ(_world.store.Get<Prefab>(wall)->path, "assets://prefabs/wall.prefab.yml");
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(PrefabFilesTest, SpawnsBelowAParentWithTheOverridesOnTop)
  {
    WriteWall();
    Load("entities:\n  - name: room\n");
    const Entity room = Find("room");

    auto transform = DataValue::Map();
    auto position = DataValue::List();
    position.Add(DataValue::Number(1));
    position.Add(DataValue::Number(2));
    position.Add(DataValue::Number(3));
    transform.Set("position", position);
    auto overrides = DataValue::Map();
    overrides.Set("Transform", transform);
    overrides.Set("Spectator", DataValue{});

    const Entity wall = _scene.Spawn(_world.store, "assets://prefabs/wall.prefab.yml", room, overrides);

    ASSERT_NE(wall, No_Entity);
    EXPECT_EQ(_world.store.GetParent(wall), room);
    EXPECT_EQ(_world.store.Get<Transform>(wall)->position, glm::vec3(1.0f, 2.0f, 3.0f));
    EXPECT_EQ(_world.store.Get<Transform>(wall)->scale.y, 2.5f) << "what the overrides leave out is the prefab's";
    EXPECT_FALSE(_world.store.Has<Spectator>(wall)) << "nothing takes a component away";
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
  }

  TEST_F(PrefabFilesTest, SpawnsWithTheChildrenOfThePrefab)
  {
    WritePrefab("lamp", "entity:\n  children:\n    - name: bulb\n      components:\n        Light: Default\n");
    Load("entities: []\n");

    const Entity lamp = _scene.Spawn(_world.store, "assets://prefabs/lamp.prefab.yml", No_Entity, DataValue{});

    ASSERT_NE(lamp, No_Entity);
    EXPECT_THAT(_world.NamesBelow(lamp), ElementsAre("bulb"));
  }

  TEST_F(PrefabFilesTest, SpawningAPrefabThatCannotBeReadSpawnsNothingAndSaysSo)
  {
    Load("entities: []\n");
    _logger->Clear();

    const Entity nothing = _scene.Spawn(_world.store, "assets://prefabs/nope.prefab.yml", No_Entity, DataValue{});

    EXPECT_EQ(nothing, No_Entity);
    EXPECT_THAT(_world.NamesBelow(No_Entity), ::testing::IsEmpty());
    EXPECT_THAT(_logger->Messages(LogLevel::Error), HasSubstr(
                  "spawn of assets://prefabs/nope.prefab.yml: 'prefab' of the spawned entity is "
                  "assets://prefabs/nope.prefab.yml, which cannot be read"));
    EXPECT_THAT(_logger->Messages(LogLevel::Error), HasSubstr(
                  "The prefab assets://prefabs/nope.prefab.yml has 1 problem, nothing is spawned"));
  }

  TEST_F(PrefabFilesTest, WhatIsWrongWithTheOverridesIsSaidWithThePrefab)
  {
    WriteWall();
    Load("entities: []\n");
    auto overrides = DataValue::Map();
    overrides.Set("Spectre", DataValue::Text("Default"));

    const Entity wall = _scene.Spawn(_world.store, "assets://prefabs/wall.prefab.yml", No_Entity, overrides);

    EXPECT_NE(wall, No_Entity) << "the entity holds what could be read";
    EXPECT_THAT(_logger->Messages(LogLevel::Error), HasSubstr(
                  "spawn of assets://prefabs/wall.prefab.yml: component 'Spectre' of the spawned entity is not known"));
    EXPECT_THAT(_logger->Messages(LogLevel::Error), HasSubstr(
                  "The spawn of assets://prefabs/wall.prefab.yml has 1 problem, the entity holds what could be read"));
  }

  TEST_F(PrefabFilesTest, ASpawnReadsThePrefabTheSceneLoadedFromMemoryNotFromItsFile)
  {
    WriteWall();
    Load("entities:\n  - name: wall\n    prefab: assets://prefabs/wall.prefab.yml\n");

    // the file changes under the scene, which a spawn does not see
    WritePrefab("wall", "entity:\n  components:\n    Transform:\n      scale: [1, 9, 1]\n");

    const Entity spawned = _scene.Spawn(_world.store, "assets://prefabs/wall.prefab.yml", No_Entity, DataValue{});
    ASSERT_NE(spawned, No_Entity);
    EXPECT_EQ(_world.store.Get<Transform>(spawned)->scale.y, 2.5f);

    // the next load reads the file anew
    World second(_logger);
    ASSERT_TRUE(_scene.Load(second.store, "assets://scenes/test.scene.yml")) << _logger->Messages(LogLevel::Error);
    const Entity again = _scene.Spawn(second.store, "assets://prefabs/wall.prefab.yml", No_Entity, DataValue{});
    ASSERT_NE(again, No_Entity);
    EXPECT_EQ(second.store.Get<Transform>(again)->scale.y, 9.0f);
  }

  TEST_F(PrefabFilesTest, ASpawnedEntityIsSavedAsAPlacedOne)
  {
    WritePrefab("thing", "entity:\n  components:\n    Spectator: Default\n");
    Load("entities: []\n");
    ASSERT_NE(_scene.Spawn(_world.store, "assets://prefabs/thing.prefab.yml", No_Entity, DataValue{}), No_Entity);

    ASSERT_TRUE(_scene.Save(_world.store, "saved", "user://saved.scene.yml"));

    EXPECT_EQ(
      ReadFile("user://saved.scene.yml"),
      "scene: saved\n"
      "version: 1\n"
      "\n"
      "entities:\n"
      "  - prefab: assets://prefabs/thing.prefab.yml\n"
      "    components:\n"
      "      Spectator: Default\n");
  }
}
