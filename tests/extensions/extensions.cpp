// Extensions, as an application finds and starts them: ExtensionHost with
// the document format for YAML. The libraries are real ones, built next to
// the test and opened through SDL2, see CMakeLists.txt. What needs no
// library is read from a file system in memory.

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/data/ryml-document-format.hpp>
#include <neon/extension/extension-host.hpp>
#include <neon/extension/sdl2-library-loader.hpp>
#include <neon/filesystem/sdl2-file-system.hpp>
#include <neon/layout/flex-layout-engine.hpp>
#include <neon/testing/fake-font-rasterizer.hpp>
#include <neon/testing/fake-physics-context.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>
#include <neon/world-system/ecs/systems/extension-running.hpp>
#include <neon/world-system/flecs-entity-store.hpp>
#include <neon/common/transform.hpp>
#include <neon/testing/mock-input-context.hpp>
#include <neon/testing/mock-input-system.hpp>
#include <neon/testing/mock-library-loader.hpp>
#include <neon/testing/mock-audio-context.hpp>
#include <neon/testing/mock-render-2d-context.hpp>
#include <neon/testing/mock-render-context.hpp>
#include <neon/testing/mock-ui-system.hpp>
#include <neon/testing/mock-window-system.hpp>
#include <neon/testing/mock-world-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/testing/recording-logging-context.hpp>
#include <neon/ui/tree-ui-system.hpp>

namespace
{
  using neon::ExtensionHost;
  using neon::RYML_DocumentFormat;
  using neon::SDL2_FileSystem;
  using neon::SDL2_LibraryLoader;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::MockLibraryLoader;
  using neon::testing::RecordingLogger;
  using neon::testing::RecordingLoggingContext;
  using ::testing::_;
  using ::testing::ElementsAre;
  using ::testing::IsEmpty;
  using ::testing::NiceMock;
  using ::testing::Return;

  /// The files of the disk, with user:// in memory: what an extension
  /// writes for the player stays out of the folder of whoever runs the
  /// test, and is gone with it.
  class PlayerFilesInMemory final : public neon::FileSystemContext
  {
    neon::FileSystemContext *_disk;
    neon::FileSystemContext *_memory;

    [[nodiscard]] neon::FileSystemContext *For(const std::string &path) const
    {
      return path.starts_with(neon::FileSystem::user_scheme) ? _memory : _disk;
    }

  public:
    PlayerFilesInMemory(neon::FileSystemContext *disk, neon::FileSystemContext *memory)
    {
      _disk = disk;
      _memory = memory;
    }

    bool Exists(const std::string &path) override
    {
      return For(path)->Exists(path);
    }

    bool ReadBytes(const std::string &path, std::vector<unsigned char> &contents) override
    {
      return For(path)->ReadBytes(path, contents);
    }

    bool ReadText(const std::string &path, std::string &contents) override
    {
      return For(path)->ReadText(path, contents);
    }

    bool WriteBytes(const std::string &path, const std::vector<unsigned char> &contents) override
    {
      return For(path)->WriteBytes(path, contents);
    }

    bool WriteText(const std::string &path, const std::string &contents) override
    {
      return For(path)->WriteText(path, contents);
    }

    bool ListFiles(const std::string &directory, std::vector<std::string> &paths) override
    {
      return For(directory)->ListFiles(directory, paths);
    }
  };

  /// The extensions the build put next to the test, started for real.
  class ExtensionsTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    RecordingLoggingContext _logging;
    SDL2_FileSystem _file_system{SettingsConfig{}, std::make_shared<RecordingLogger>()};

    // user://, which the file system of the disk has no folder for here
    MemoryFileSystem _player_files{SettingsConfig{}, std::make_shared<RecordingLogger>()};
    PlayerFilesInMemory _files{&_file_system, &_player_files};

    SDL2_LibraryLoader _library_loader{&_file_system};
    RYML_DocumentFormat _yaml;
    ExtensionHost _host{&_files, &_yaml, &_library_loader, &_logging, _logger};

    void SetUp() override
    {
      _file_system.Initialize();
      _player_files.Initialize();
      _host.Initialize();
    }

    void TearDown() override
    {
      _host.CleanUp();
      _file_system.CleanUp();
    }

    void ExpectError(const std::string &text) const
    {
      EXPECT_TRUE(_logger->Contains(LogLevel::Error, text)) << _logger->Messages(LogLevel::Error);
    }
  };

  TEST_F(ExtensionsTest, StartsTheExtensionsThatCanStartInTheOrderOfTheirNames)
  {
    EXPECT_THAT(_host.GetLoaded(), ElementsAre(
      "eager", "hello", "keeper", "mislaid", "old", "painter", "polite", "sign", "spinner", "tumbler", "visitor"));
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Started the extension 'hello' from extensions://hello/hello-"))
      << _logger->Messages(LogLevel::Info);
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Started 11 of 14 extensions")) << _logger->Messages(LogLevel::Info);
  }

  TEST_F(ExtensionsTest, NamesTheAssetsOfTheExtensionsThatAreThereToBeUsed)
  {
    const auto folders = _host.GetAssetFolders();
    ASSERT_FALSE(folders.empty());
    EXPECT_EQ(folders[1], "extensions://hello/assets/");
    EXPECT_THAT(folders, ::testing::Not(::testing::Contains("extensions://refuses/assets/")));

    // the build copies what an extension brings next to its library
    std::string greeting;
    EXPECT_TRUE(_file_system.ReadText("extensions://hello/assets/greeting.txt", greeting));
    EXPECT_EQ(greeting, "Hello from the assets of an extension\n");
  }

  TEST_F(ExtensionsTest, LetsAnExtensionLogUnderItsNameAtTheLevelItChooses)
  {
    EXPECT_TRUE(_logging.Of("extension:hello")->Contains(LogLevel::Info, "Hello from an extension in C"));
    EXPECT_TRUE(_logging.Of("extension:polite")->Contains(LogLevel::Warn, "Hello from an extension in C++"));
  }

  TEST_F(ExtensionsTest, TellsAnExtensionToCleanUpWithWhatItKept)
  {
    const auto polite = _logging.Of("extension:polite");
    EXPECT_FALSE(polite->Contains(LogLevel::Info, "Goodbye from an extension in C++"));

    _host.CleanUp();

    EXPECT_TRUE(polite->Contains(LogLevel::Info, "Goodbye from an extension in C++"));
    EXPECT_THAT(_host.GetLoaded(), IsEmpty());
  }

  TEST_F(ExtensionsTest, CanBeCleanedUpTwice)
  {
    _host.CleanUp();
    _host.CleanUp();

    EXPECT_EQ(_logging.Of("extension:polite")->Count(LogLevel::Info), 1u);
  }

  TEST_F(ExtensionsTest, LeavesOutAnExtensionThatDoesNotStartAndKeepsWhatItSaid)
  {
    ExpectError("The extension 'refuses' did not start");
    EXPECT_TRUE(_logging.Of("extension:refuses")->Contains(LogLevel::Error, "The data of the game is not here"));
  }

  TEST_F(ExtensionsTest, LeavesOutAnExtensionThatWasBuiltWithALaterVersionOfTheInterface)
  {
    ExpectError("The extension 'from-the-future' was built with version 99 of the interface of extensions, "
      "and this application has version " + std::to_string(NEON_EXTENSION_ABI_VERSION));
  }

  TEST_F(ExtensionsTest, LeavesOutALibraryThatExportsNoFunctionToStartItBy)
  {
    ExpectError("exports no function 'neon_extension_initialize', so it is no extension");
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "extensions://no-entry/no-entry-"));
  }

  /// A physics whose rays hit what the test says, and remembers what was
  /// asked.
  class RayPhysics final : public neon::testing::FakePhysicsContext
  {
  public:
    bool hits = false;
    neon::Ray asked;
    neon::QueryFilter asked_filter;

    bool CastRay(const neon::Ray &ray, const neon::QueryFilter &filter, neon::RayHit &hit) override
    {
      asked = ray;
      asked_filter = filter;
      hit.entity = 42;
      hit.distance = 5.0f;
      return hits;
    }
  };

  /// The extensions in a world: a store of Flecs, the formats a scene reads
  /// components by, and the system that lets the extensions take part.
  class ExtensionsInTheWorldTest : public ExtensionsTest
  {
  protected:
    neon::Flecs_EntityStore _store{std::make_shared<RecordingLogger>()};
    neon::ComponentFormats _formats;
    neon::ExtensionRunning _running{&_host, &_formats};
    NiceMock<neon::testing::MockInputContext> _input;
    RayPhysics _physics;
    NiceMock<neon::testing::MockUiSystem> _ui;
    NiceMock<neon::testing::MockRenderContext> _render{std::make_shared<RecordingLogger>()};

    // what the renderer draws to, which is the view an extension asks the
    // size of
    neon::RenderResolution _resolution{640, 360};

    // the pictures the extensions made, by their names
    std::map<std::string, neon::ImagePixels> _images;

    // the numbers the extensions set for their shaders, by their places
    std::map<int, glm::vec4> _shader_numbers;

    NiceMock<neon::testing::MockAudioContext> _audio;

    // the sounds the extensions handed over, by their names
    std::map<std::string, std::vector<std::uint8_t>> _sounds;

    // how loud the extensions set the groups of sounds, by the name of each
    std::map<std::string, float> _volumes;
    NiceMock<neon::testing::MockWorldSystem> _world{std::make_shared<RecordingLogger>()};

    // what an extension that asks the application to close tells
    NiceMock<neon::testing::MockWindowContext> _window;

    /// The user interface the extensions are given. One that takes every
    /// call and does nothing, unless a test brings another.
    [[nodiscard]] virtual neon::UiContext *Ui()
    {
      return &_ui;
    }

    void SetUp() override
    {
      ExtensionsTest::SetUp();

      _store.Initialize();
      // a component of the engine, whose name is taken, and whose fields an
      // extension reaches by their names
      _store.Register<neon::Transform>("Transform");
      _formats.Add(neon::ComponentFormat::Of<neon::Transform>());

      _host.SetInput(&_input);
      _host.SetWorld(&_world);
      _host.SetPhysics(&_physics);
      _host.SetUi(Ui());
      _host.SetRender(&_render);
      ON_CALL(_render, GetRenderResolution()).WillByDefault(::testing::ReturnRef(_resolution));
      _host.SetAudio(&_audio);
      _host.SetWindow(&_window);
      ON_CALL(_audio, SetSound(_, _)).WillByDefault([this](const std::string &name, std::vector<std::uint8_t> bytes)
      {
        _sounds[name] = std::move(bytes);
        return true;
      });
      ON_CALL(_audio, SetGroupVolume(_, _)).WillByDefault([this](const std::string &group, const float volume)
      {
        _volumes[group] = volume;
      });
      ON_CALL(_audio, GetGroupVolume(_)).WillByDefault([this](const std::string &group)
      {
        const auto known = _volumes.find(group);
        return known == _volumes.end() ? 0.0f : known->second;
      });
      ON_CALL(_render, SetImage(_, _)).WillByDefault([this](const std::string &name, const neon::ImagePixels &pixels)
      {
        _images[name] = pixels;
        return true;
      });
      ON_CALL(_render, SetShaderNumbers(_, _)).WillByDefault([this](const int place, const glm::vec4 &numbers)
      {
        if (place < 0 || place >= neon::RenderContext::kShader_Number_Places) { return false; }
        _shader_numbers[place] = numbers;
        return true;
      });

      // what draws an entity, which an extension gives a mesh and textures
      _store.Register<neon::Renderable>("Renderable");
      _formats.Add(neon::ComponentFormat::Of<neon::Renderable>());

      _running.Register(_store);
      _running.Initialize(_store);
    }

    void TearDown() override
    {
      _host.LeaveWorld();
      _store.CleanUp();
      ExtensionsTest::TearDown();
    }

    [[nodiscard]] std::shared_ptr<RecordingLogger> LogOf(const std::string &extension)
    {
      return _logging.Of("extension:" + extension);
    }

    void ExpectErrorOf(const std::string &extension, const std::string &text)
    {
      EXPECT_TRUE(LogOf(extension)->Contains(LogLevel::Error, text)) << LogOf(extension)->Messages(LogLevel::Error);
    }
  };

  TEST_F(ExtensionsInTheWorldTest, RegistersTheComponentOfAnExtensionWithTheStoreAndForRecipes)
  {
    EXPECT_NE(_store.FindComponent("Spinner"), neon::No_Component);

    const neon::ComponentFormat *format = _formats.Find("Spinner");
    ASSERT_NE(format, nullptr);
    ASSERT_NE(format->type, nullptr);
    EXPECT_EQ(format->type->description, "Turns what carries it");
    ASSERT_EQ(format->type->fields.size(), 4u);
    EXPECT_EQ(format->type->fields[0].name, "speed");
    EXPECT_EQ(format->type->fields[0].description, "How far it turns in a second, in degrees");
    EXPECT_EQ(format->type->fields[1].kind, neon::FieldKind::Vector3);
  }

  TEST_F(ExtensionsInTheWorldTest, LetsAnExtensionCreateEntitiesSetComponentsAndQueryThem)
  {
    EXPECT_TRUE(LogOf("spinner")->Contains(
      LogLevel::Info,
      "1 entity spins, at 360 degrees, after 4 turns, under its parent: 1, without a Spinner at the top: 1"))
      << LogOf("spinner")->Messages(LogLevel::Info) << LogOf("spinner")->Messages(LogLevel::Error);

    EXPECT_NE(_store.FindEntity("made-by-spinner/wheel"), neon::No_Entity);
  }

  TEST_F(ExtensionsInTheWorldTest, ReadsTheComponentOfAnExtensionFromARecipeWithItsDefaultsAndWritesItBack)
  {
    neon::DataValue written;
    std::string error;
    ASSERT_TRUE(_yaml.Read("a recipe", "speed: 45\nturns: 7\n", written, error)) << error;

    const neon::Entity entity = _store.CreateEntity("from-a-recipe");
    const neon::ComponentFormat *format = _formats.Find("Spinner");
    ASSERT_NE(format, nullptr);

    std::vector<std::string> errors;
    format->read(neon::DataReader(written, "a recipe", "Spinner", errors), _store, entity);
    EXPECT_THAT(errors, IsEmpty());

    // as the struct of the extension has it
    struct Spinner
    {
      float speed;
      float axis[3];
      int turns;
      bool enabled;
    };
    const auto *spinner = static_cast<const Spinner *>(_store.GetComponent(entity, _store.FindComponent("Spinner")));
    ASSERT_NE(spinner, nullptr);
    EXPECT_EQ(spinner->speed, 45.0f);
    EXPECT_EQ(spinner->axis[1], 1.0f);
    EXPECT_EQ(spinner->turns, 7);
    EXPECT_TRUE(spinner->enabled);

    neon::DataValue back;
    ASSERT_TRUE(format->write(_store, entity, back));
    EXPECT_NE(back.Find("speed"), nullptr);
    EXPECT_EQ(back.Find("axis"), nullptr) << "what holds its default is left out";
  }

  TEST_F(ExtensionsInTheWorldTest, RefusesAComponentWhoseStructIsNotWhatItDescribes)
  {
    ExpectErrorOf("mislaid", "The component 'Padded' cannot be registered: the struct of the extension has 16 bytes, "
      "and 4 by its description");
    ExpectErrorOf("mislaid", "The component 'Swapped' cannot be registered: its field 'speed' lies at byte 8 of the "
      "struct of the extension, and at byte 0 by its description");
    ExpectErrorOf("mislaid", "The field 'what' of the component 'Unknown' is of kind 999");
    ExpectErrorOf("mislaid", "The component 'Transform' cannot be registered: there is a component of that name");
    ExpectErrorOf("mislaid", "A component was registered without a name");

    EXPECT_EQ(LogOf("mislaid")->Count(LogLevel::Critical), 0u);
    EXPECT_EQ(_store.FindComponent("Padded"), neon::No_Component);
    EXPECT_EQ(_formats.Find("Swapped"), nullptr);
  }

  TEST_F(ExtensionsInTheWorldTest, RefusesWhatAnExtensionDoesAtTheWrongTime)
  {
    ExpectErrorOf("eager", "register_component was called outside of 'register_components'");
    ExpectErrorOf("eager", "create_entity was called while there is no world");
    EXPECT_EQ(LogOf("eager")->Count(LogLevel::Critical), 0u);
    EXPECT_EQ(_store.FindComponent("Marker"), neon::No_Component);
    EXPECT_EQ(_store.FindEntity("too-early"), neon::No_Entity);
  }

  TEST_F(ExtensionsInTheWorldTest, RefusesCallsWithoutAComponentAnEntityOrAQuery)
  {
    ExpectErrorOf("eager", "set_component was called with no component");
    ExpectErrorOf("eager", "get_component was called with an entity that is not alive");
    ExpectErrorOf("eager", "create_query takes from 1 to 8 components, and was given 0");
    ExpectErrorOf("eager", "each was called without a query");
  }

  TEST_F(ExtensionsInTheWorldTest, CallsNothingPastWhatTheVersionOfAnExtensionHolds)
  {
    EXPECT_TRUE(LogOf("old")->Contains(LogLevel::Info, "Hello from an extension of version 1"));
    EXPECT_EQ(LogOf("old")->Count(LogLevel::Error), 0u) << LogOf("old")->Messages(LogLevel::Error);
  }

  // as the struct of the extension has it
  struct Tumbler
  {
    float speed;
    float axis[3];
    float angle;
    int frames;
    bool enabled;
  };

  TEST_F(ExtensionsInTheWorldTest, RunsTheSystemOfAnExtensionInCInEveryFrame)
  {
    struct Spinner
    {
      float speed;
      float axis[3];
      int turns;
      bool enabled;
    };
    const auto wheel = _store.FindEntity("made-by-spinner/wheel");
    const auto id = _store.FindComponent("Spinner");

    // once by the extension when it started
    ASSERT_EQ(static_cast<const Spinner *>(_store.GetComponent(wheel, id))->turns, 4);

    _running.Update(_store, 0.016);
    _running.Update(_store, 0.016);

    EXPECT_EQ(static_cast<const Spinner *>(_store.GetComponent(wheel, id))->turns, 6);
    EXPECT_TRUE(LogOf("spinner")->Contains(LogLevel::Info, "Added the system 'Turning'"));
  }

  TEST_F(ExtensionsInTheWorldTest, RunsTheSystemOfAnExtensionInCppInEveryStepFrameAndBetween)
  {
    const auto id = _store.FindComponent("Tumbler");
    ASSERT_NE(id, neon::No_Component);

    // one from a recipe, with the defaults the struct of the extension has
    neon::DataValue written;
    std::string error;
    ASSERT_TRUE(_yaml.Read("a recipe", "speed: 45\n", written, error)) << error;
    const neon::Entity entity = _store.CreateEntity("from-a-recipe");
    std::vector<std::string> errors;
    _formats.Find("Tumbler")->read(neon::DataReader(written, "a recipe", "Tumbler", errors), _store, entity);
    ASSERT_THAT(errors, IsEmpty());

    const auto *tumbler = static_cast<const Tumbler *>(_store.GetComponent(entity, id));
    ASSERT_NE(tumbler, nullptr);
    EXPECT_EQ(tumbler->axis[1], 1.0f);
    EXPECT_TRUE(tumbler->enabled);

    _running.FixedUpdate(_store, 0.5);
    _running.FixedUpdate(_store, 0.5);
    _running.Update(_store, 0.016);
    _running.Interpolate(_store, 0.25);

    // 45 degrees a second, for a second, at the pace of 2 the system was added with
    tumbler = static_cast<const Tumbler *>(_store.GetComponent(entity, id));
    EXPECT_EQ(tumbler->angle, 90.0f);
    EXPECT_EQ(tumbler->frames, 1);

    // the one the extension made stands still
    const auto still = _store.FindEntity("made-by-tumbler");
    ASSERT_NE(still, neon::No_Entity);
    EXPECT_EQ(static_cast<const Tumbler *>(_store.GetComponent(still, id))->angle, 0.0f);
    EXPECT_EQ(static_cast<const Tumbler *>(_store.GetComponent(still, id))->speed, 10.0f);

    EXPECT_TRUE(LogOf("tumbler")->Contains(LogLevel::Info, "A tumbler that stands still was made: yes"));
    EXPECT_TRUE(LogOf("tumbler")->Contains(LogLevel::Info, "Between two steps, a quarter of the way"));
    EXPECT_EQ(LogOf("tumbler")->Count(LogLevel::Error), 0u) << LogOf("tumbler")->Messages(LogLevel::Error);
  }

  TEST_F(ExtensionsInTheWorldTest, CleansUpAnExtensionInCpp)
  {
    _host.CleanUp();

    EXPECT_TRUE(LogOf("tumbler")->Contains(LogLevel::Info, "The tumblers are put away"));
  }

  TEST_F(ExtensionsInTheWorldTest, RefusesASystemThatIsNoneOrComesTooLate)
  {
    ExpectErrorOf("eager", "The system 'Empty' has no function to call");
    ExpectErrorOf("eager", "A system was added without a name");
    ExpectErrorOf("eager", "The system 'Late' was added after the world came up");
    ExpectErrorOf("eager", "listen_to_physics was called after the world came up");
    ExpectErrorOf("eager", "listen_to_physics was called without a function to tell");
    ExpectErrorOf("eager", "cast_ray was called with a direction of no length");
    EXPECT_EQ(LogOf("eager")->Count(LogLevel::Critical), 0u);
  }

  TEST_F(ExtensionsInTheWorldTest, FindsTheFieldsOfAComponentOfTheEngineByTheirNamesAndReadsAFile)
  {
    EXPECT_TRUE(LogOf("visitor")->Contains(
      LogLevel::Info, "Found position and scale of Transform: yes, and what is not there: no"))
      << LogOf("visitor")->Messages(LogLevel::Info);
    EXPECT_TRUE(LogOf("visitor")->Contains(LogLevel::Info, "Read its own recipe: yes, a file that is not there: no"))
      << LogOf("visitor")->Messages(LogLevel::Info);
  }

  TEST_F(ExtensionsInTheWorldTest, MovesAnEntityByTheInputThroughTheFieldsOfItsTransform)
  {
    const neon::Entity visited = _store.CreateEntity("visited");
    neon::Transform transform;
    transform.position = {1.0f, 2.0f, 3.0f};
    _store.Set(visited, transform);

    ON_CALL(_input, ActionAxis2("move")).WillByDefault(Return(glm::vec2(0.5f, -1.0f)));
    ON_CALL(_input, WasActionPressed("jump")).WillByDefault(Return(true));
    ON_CALL(_input, IsActionDown("grow")).WillByDefault(Return(true));

    _running.Update(_store, 0.016);

    const auto *moved = _store.Get<neon::Transform>(visited);
    EXPECT_EQ(moved->position, glm::vec3(1.5f, 3.0f, 2.0f));
    EXPECT_EQ(moved->scale, glm::vec3(2.0f));
    EXPECT_EQ(LogOf("visitor")->Count(LogLevel::Error), 0u) << LogOf("visitor")->Messages(LogLevel::Error);
  }

  TEST_F(ExtensionsInTheWorldTest, RefusesAValueAFieldDoesNotTake)
  {
    const neon::Entity visited = _store.CreateEntity("visited");
    _store.Set(visited, neon::Transform{});
    ON_CALL(_input, IsActionDown("vanish")).WillByDefault(Return(true));

    _running.Update(_store, 0.016);

    EXPECT_EQ(_store.Get<neon::Transform>(visited)->scale, glm::vec3(1.0f));
    ExpectErrorOf("visitor", "set_field_text: ");
    EXPECT_FALSE(LogOf("visitor")->Contains(LogLevel::Error, "A scale of text was taken"));
  }

  TEST_F(ExtensionsInTheWorldTest, SaysWhenAnEntityHasNoComponentToSetAFieldOf)
  {
    _store.CreateEntity("visited");

    _running.Update(_store, 0.016);

    ExpectErrorOf("visitor", "set_field of Transform.position: the entity has no such component");
  }

  TEST_F(ExtensionsInTheWorldTest, SpawnsAPrefabAndAsksForAnotherSceneThroughTheWorld)
  {
    const neon::Entity visited = _store.CreateEntity("visited");
    _store.Set(visited, neon::Transform{});
    ON_CALL(_input, WasActionPressed("spawn")).WillByDefault(Return(true));
    ON_CALL(_input, WasActionPressed("leave")).WillByDefault(Return(true));

    EXPECT_CALL(_world, Spawn("assets://prefabs/crate.prefab.yml", visited, _)).WillOnce(Return(neon::Entity{77}));
    EXPECT_CALL(_world, LoadScene("assets://scenes/next.scene.yml")).Times(1);

    _running.Update(_store, 0.016);

    EXPECT_TRUE(LogOf("visitor")->Contains(LogLevel::Info, "Spawned 77"));
  }

  TEST_F(ExtensionsInTheWorldTest, SaysWhenTheApplicationHasNoInputOrWorldForItsExtensions)
  {
    const neon::Entity visited = _store.CreateEntity("visited");
    _store.Set(visited, neon::Transform{});
    _host.SetInput(nullptr);

    _running.Update(_store, 0.016);

    ExpectErrorOf("visitor", "action_axis2 was called, and this application has no input for its extensions");
  }

  TEST_F(ExtensionsInTheWorldTest, TellsASystemWhatBeganAndEndedToTouchBeforeItsUpdate)
  {
    _physics.next_events.push_back({
      .kind = neon::PhysicsEventKind::Began, .trigger = true, .first = 7, .second = 8, .point = {0.0f, 3.0f, 0.0f}
    });
    _physics.next_events.push_back({.kind = neon::PhysicsEventKind::Ended, .first = 5, .second = 6});
    _physics.Step(1.0 / 60.0);

    _running.Update(_store, 0.016);

    EXPECT_TRUE(LogOf("visitor")->Contains(LogLevel::Info, "A trigger 7 was entered by 8 at height 3"))
      << LogOf("visitor")->Messages(LogLevel::Info);
    EXPECT_TRUE(LogOf("visitor")->Contains(LogLevel::Info, "A body 5 was left by 6 at height 0"));
  }

  TEST_F(ExtensionsInTheWorldTest, CastsARayThroughThePhysics)
  {
    _store.Set(_store.CreateEntity("visited"), neon::Transform{});
    ON_CALL(_input, WasActionPressed("look")).WillByDefault(Return(true));

    _running.Update(_store, 0.016);
    EXPECT_TRUE(LogOf("visitor")->Contains(LogLevel::Info, "Looked down and saw nothing"));

    _physics.hits = true;
    _running.Update(_store, 0.016);

    EXPECT_TRUE(LogOf("visitor")->Contains(LogLevel::Info, "Looked down and saw 42 at a distance of 5"));
    EXPECT_EQ(_physics.asked.origin, glm::vec3(0.0f, 5.0f, 0.0f));
    EXPECT_EQ(_physics.asked.direction, glm::vec3(0.0f, -1.0f, 0.0f));
    EXPECT_EQ(_physics.asked.distance, 100.0f);
    EXPECT_EQ(_physics.asked_filter.ignore, neon::Entity{9});
    EXPECT_FALSE(_physics.asked_filter.triggers);
  }

  TEST_F(ExtensionsInTheWorldTest, ReadsAndWritesAFieldOfTheEngineInPlaceForEveryEntityAndSetsValuesOfTheUi)
  {
    EXPECT_TRUE(LogOf("visitor")->Contains(LogLevel::Info, "The position has a place: yes, and what has none: no"))
      << LogOf("visitor")->Messages(LogLevel::Info);

    const neon::Entity visited = _store.CreateEntity("visited");
    const neon::Entity other = _store.CreateEntity("other");
    neon::Transform transform;
    transform.position = {1.0f, 2.0f, 3.0f};
    _store.Set(visited, transform);
    _store.Set(other, neon::Transform{});
    ON_CALL(_input, WasActionPressed("lift")).WillByDefault(Return(true));

    // the two of the test, and the one the painter made
    EXPECT_CALL(_ui, SetNumber("lifted", 3.0)).Times(1);
    EXPECT_CALL(_ui, SetText("who", "visitor")).Times(1);

    _running.Update(_store, 0.016);

    EXPECT_EQ(_store.Get<neon::Transform>(visited)->position, glm::vec3(1.0f, 3.0f, 3.0f));
    EXPECT_EQ(_store.Get<neon::Transform>(other)->position, glm::vec3(0.0f, 1.0f, 0.0f));
  }

  TEST_F(ExtensionsInTheWorldTest, SpawnsAPrefabAtAPlaceWrittenOnTopOfItsTransform)
  {
    _store.Set(_store.CreateEntity("visited"), neon::Transform{});
    ON_CALL(_input, WasActionPressed("drop")).WillByDefault(Return(true));

    neon::DataValue overrides;
    EXPECT_CALL(_world, Spawn("assets://prefabs/crate.prefab.yml", neon::No_Entity, _))
      .WillOnce([&overrides](const std::string &, neon::Entity, const neon::DataValue &given)
      {
        overrides = given;
        return neon::Entity{88};
      });

    _running.Update(_store, 0.016);

    EXPECT_TRUE(LogOf("visitor")->Contains(LogLevel::Info, "Dropped 88"));
    const neon::DataValue *transform = overrides.Find("Transform");
    ASSERT_NE(transform, nullptr);
    ASSERT_NE(transform->Find("position"), nullptr);
    float height = 0.0f;
    ASSERT_TRUE(transform->Find("position")->GetItems()[1].GetNumber(height));
    EXPECT_EQ(height, 30.0f);
    float yaw = 0.0f;
    ASSERT_TRUE(transform->Find("rotation")->GetItems()[1].GetNumber(yaw));
    EXPECT_EQ(yaw, 90.0f);
  }

  TEST_F(ExtensionsInTheWorldTest, AnExtensionSetsAndReadsHowLoudAGroupOfSoundsIs)
  {
    EXPECT_TRUE(LogOf("painter")->Contains(
      LogLevel::Info, "Turned the music down: yes, to a quarter: yes, and no group without a name: yes"))
      << LogOf("painter")->Messages(LogLevel::Info);
    EXPECT_TRUE(LogOf("painter")->Contains(
      LogLevel::Error, "set_group_volume was called without the name of a group"));

    // the audio was told, by the name the extension gave
    ASSERT_TRUE(_volumes.contains("music"));
    EXPECT_EQ(_volumes["music"], 0.25f);
    EXPECT_EQ(_volumes.size(), 1u);
  }

  TEST_F(ExtensionsInTheWorldTest, ShowsAMeshAndAPictureAnExtensionMadeOnAnEntityWithTheEnginesComponents)
  {
    EXPECT_TRUE(LogOf("painter")->Contains(
      LogLevel::Info, "Painted image://painter/checker: components yes, textures yes, mesh yes"))
      << LogOf("painter")->Messages(LogLevel::Info) << LogOf("painter")->Messages(LogLevel::Error);

    // the picture reached the renderer under the name of the extension
    ASSERT_TRUE(_images.contains("painter/checker"));
    EXPECT_EQ(_images["painter/checker"].width, 2);
    EXPECT_EQ(_images["painter/checker"].height, 2);
    EXPECT_THAT(_images["painter/checker"].pixels, ::testing::SizeIs(16));
    EXPECT_EQ(_images["painter/checker"].pixels[5], 255);

    const neon::Entity painted = _store.FindEntity("painted");
    ASSERT_NE(painted, neon::No_Entity);
    EXPECT_EQ(_store.Get<neon::Transform>(painted)->position, glm::vec3(0.0f, 0.0f, -3.0f));

    const auto *renderable = _store.Get<neon::Renderable>(painted);
    ASSERT_NE(renderable, nullptr);
    EXPECT_EQ(renderable->render_info.shader_path, "assets://shaders/unlit");
    EXPECT_THAT(renderable->render_info.texture_paths, ElementsAre("image://painter/checker"));

    ASSERT_NE(renderable->render_info.mesh, nullptr);
    EXPECT_THAT(renderable->render_info.mesh->indices, ElementsAre(0u, 1u, 2u, 0u, 2u, 3u));
    ASSERT_THAT(renderable->render_info.mesh->vertices, ::testing::SizeIs(4));
    EXPECT_EQ(renderable->render_info.mesh->vertices[2].position, glm::vec3(1.0f, 1.0f, 0.0f));
    EXPECT_EQ(renderable->render_info.mesh->vertices[2].normal, glm::vec3(0.0f, 0.0f, 1.0f));
    EXPECT_EQ(renderable->render_info.mesh->vertices[2].tex_coords, glm::vec2(1.0f, 0.0f));
    EXPECT_EQ(renderable->render_info.mesh->vertices[2].color, glm::vec4(1.0f));

    // the second set of coordinates, and the lightmap they are for
    EXPECT_TRUE(LogOf("painter")->Contains(LogLevel::Info, "Lightmap: yes"));
    EXPECT_EQ(renderable->render_info.mesh->vertices[0].lightmap_coords, glm::vec2(0.1f, 0.9f));
    EXPECT_EQ(renderable->render_info.mesh->vertices[2].lightmap_coords, glm::vec2(0.9f, 0.1f));
    EXPECT_EQ(renderable->render_info.material_info.lightmap, "image://painter/checker");

    // counted up with each, so that an entity that is drawn already is
    // handed the mesh
    EXPECT_EQ(renderable->render_info.mesh_version, 2u);
  }

  TEST_F(ExtensionsInTheWorldTest, AnExtensionSetsNumbersForTheShadersItBrings)
  {
    EXPECT_TRUE(LogOf("painter")->Contains(LogLevel::Info, "Numbers for the shaders: yes, and none at place 8: yes"))
      << LogOf("painter")->Messages(LogLevel::Info);

    ASSERT_TRUE(_shader_numbers.contains(2));
    EXPECT_EQ(_shader_numbers[2], glm::vec4(0.5f, 0.25f, 1.0f, 8.0f));
    EXPECT_FALSE(_shader_numbers.contains(8));
    ExpectErrorOf(
      "painter", "set_shader_numbers: there is no place 8 for the numbers of a game, the places are 0 to 7");
  }

  TEST_F(ExtensionsInTheWorldTest, AnExtensionHandsOverASoundFromMemory)
  {
    EXPECT_TRUE(LogOf("painter")->Contains(
      LogLevel::Info, "Handed over sound://painter/beep, and nothing without bytes: yes"))
      << LogOf("painter")->Messages(LogLevel::Info);

    // under the name of the extension, so that two extensions do not take
    // each other's
    ASSERT_TRUE(_sounds.contains("painter/beep"));
    EXPECT_THAT(_sounds["painter/beep"], ElementsAre(82, 73, 70, 70));
    EXPECT_FALSE(_sounds.contains("painter/silence"));
  }

  TEST_F(ExtensionsInTheWorldTest, RefusesAMeshAPictureOrAComponentThatIsNone)
  {
    EXPECT_TRUE(LogOf("painter")->Contains(
      LogLevel::Info, "What is no mesh, picture, or component was refused: yes"))
      << LogOf("painter")->Messages(LogLevel::Info);

    ExpectErrorOf("painter", "set_mesh: the entity has no Renderable to draw the mesh with");
    ExpectErrorOf("painter", "set_mesh: index 2 names corner 4, and there are 4 corners");
    ExpectErrorOf("painter", "set_mesh takes corners, and three indices for every triangle; it was given 4 corners and 2");
    ExpectErrorOf("painter", "add_component: there is no component 'Nothing' that a recipe could write");
    ExpectErrorOf("painter", "set_mesh_lightmap takes one pair of coordinates for every corner; the mesh has 4 corners and 1 were given");
    ExpectErrorOf("painter", "set_mesh_lightmap: the entity has no mesh; one is set with set_mesh first");
    EXPECT_FALSE(_images.contains("painter/short"));
  }

  TEST_F(ExtensionsInTheWorldTest, WritesAFileOfThePlayerAndFindsItAgain)
  {
    EXPECT_TRUE(LogOf("keeper")->Contains(
      LogLevel::Info, "Saved: yes, found slot-1.sav, slot-2.sav, and read back: shamblers: 2"))
      << LogOf("keeper")->Messages(LogLevel::Info) << LogOf("keeper")->Messages(LogLevel::Error);

    // what the file system holds of it, the folders that were not there included
    std::string saved;
    EXPECT_TRUE(_player_files.ReadText("user://saves/slot-1.sav", saved));
    EXPECT_EQ(saved, "shamblers: 2");
    EXPECT_TRUE(_player_files.ReadText("user://saves/old/slot-0.sav", saved));
    EXPECT_EQ(saved, "");
  }

  TEST_F(ExtensionsInTheWorldTest, ListsTheFilesDirectlyInAFolderThatCanBeRead)
  {
    EXPECT_TRUE(LogOf("keeper")->Contains(LogLevel::Info, "a folder that is not there holds 0 files"))
      << LogOf("keeper")->Messages(LogLevel::Info);

    // the recipe and the library, not what a folder below would hold
    EXPECT_TRUE(LogOf("keeper")->Contains(LogLevel::Info, "Its own folder holds 2 files, the first extension.yml"))
      << LogOf("keeper")->Messages(LogLevel::Info);
  }

  TEST_F(ExtensionsInTheWorldTest, RefusesToWriteAFileThatIsNotThePlayers)
  {
    EXPECT_TRUE(LogOf("keeper")->Contains(LogLevel::Info, "What is not the player's was refused: yes"))
      << LogOf("keeper")->Messages(LogLevel::Info);

    ExpectErrorOf("keeper", "write_file: 'assets://saves/slot-1.sav' is not under user://, which is the one place "
      "an extension writes");
    ExpectErrorOf("keeper", "write_file: 'output://slot-1.sav' is not under user://");
    ExpectErrorOf("keeper", "write_file: 'saves/slot-1.sav' is not under user://");
    ExpectErrorOf("keeper", "write_file: 'user://../outside.sav' has '..' in it, which would leave the folder of "
      "the player");
    ExpectErrorOf("keeper", "write_file: 'user://saves/../../outside.sav' has '..' in it");
    ExpectErrorOf("keeper", "write_file was called without a path");

    EXPECT_FALSE(_player_files.Exists("user://outside.sav"));
    EXPECT_FALSE(_file_system.Exists("assets://saves/slot-1.sav"));
  }

  TEST_F(ExtensionsInTheWorldTest, TellsTheWindowToCloseWhenAnExtensionAsksToQuit)
  {
    EXPECT_CALL(_window, SignalToClose()).Times(0);
    _running.Update(_store, 0.016);
    ::testing::Mock::VerifyAndClearExpectations(&_window);

    ON_CALL(_input, WasActionPressed("quit")).WillByDefault(Return(true));
    EXPECT_CALL(_window, SignalToClose()).Times(1);

    _running.Update(_store, 0.016);

    EXPECT_TRUE(LogOf("keeper")->Contains(LogLevel::Info, "The extension asked the application to close"));
  }

  TEST_F(ExtensionsInTheWorldTest, SaysWhenTheApplicationHasNoWindowForItsExtensions)
  {
    _host.SetWindow(nullptr);
    ON_CALL(_input, WasActionPressed("quit")).WillByDefault(Return(true));

    _running.Update(_store, 0.016);

    ExpectErrorOf("keeper", "request_quit was called, and this application has no window for its extensions");
  }

  TEST_F(ExtensionsInTheWorldTest, TakesTheStoreFromTheExtensionsWhenTheWorldIsLeft)
  {
    _host.LeaveWorld();
    _host.Start(_store);
    _host.LeaveWorld();

    // started twice by the test, which a world never does; what matters is
    // that the store was there again and is gone again
    EXPECT_EQ(LogOf("spinner")->Count(LogLevel::Error), 0u) << LogOf("spinner")->Messages(LogLevel::Error);
  }

  /// A user interface that hands every call an extension makes on to
  /// another, and keeps what listens: what On() returned and Off() has not
  /// taken away.
  class WatchedUi final : public neon::UiContext
  {
    neon::UiContext *_shown;

  public:
    std::vector<int> listening;

    explicit WatchedUi(neon::UiContext *shown)
    {
      _shown = shown;
    }

    int Load(const std::string &path) override { return _shown->Load(path); }

    void Unload(const int document) override { _shown->Unload(document); }

    [[nodiscard]] bool IsShown(const int document) const override { return _shown->IsShown(document); }

    [[nodiscard]] bool IsAlive(const neon::UiHandle element) const override { return _shown->IsAlive(element); }

    [[nodiscard]] neon::UiHandle GetRoot(const int document) const override { return _shown->GetRoot(document); }

    [[nodiscard]] neon::UiHandle FindByName(const std::string &name, const neon::UiHandle from) const override
    {
      return _shown->FindByName(name, from);
    }

    [[nodiscard]] std::string GetElementType(const neon::UiHandle element) const override
    {
      return _shown->GetElementType(element);
    }

    [[nodiscard]] bool DescribeElement(const std::string &type, neon::TypeInfo &description) const override
    {
      return _shown->DescribeElement(type, description);
    }

    bool Set(const neon::UiHandle element, const std::string &property, const std::string &value) override
    {
      return _shown->Set(element, property, value);
    }

    bool SetField(const neon::UiHandle element, const std::string &name, const neon::FieldValue &value) override
    {
      return _shown->SetField(element, name, value);
    }

    bool SetVisible(const neon::UiHandle element, const bool visible) override
    {
      return _shown->SetVisible(element, visible);
    }

    neon::UiHandle Create(const std::string &description, const neon::UiHandle parent, const int index) override
    {
      return _shown->Create(description, parent, index);
    }

    bool Remove(const neon::UiHandle element) override { return _shown->Remove(element); }

    int On(const neon::UiHandle element, const std::string &event, const neon::UiListener &listener) override
    {
      const int subscription = _shown->On(element, event, listener);
      if (subscription != 0) { listening.push_back(subscription); }
      return subscription;
    }

    void Off(const int subscription) override
    {
      std::erase(listening, subscription);
      _shown->Off(subscription);
    }

    void SetNumber(const std::string &name, const double number) override { _shown->SetNumber(name, number); }

    void SetText(const std::string &name, const std::string &text) override { _shown->SetText(name, text); }

    void SetFlag(const std::string &name, const bool flag) override { _shown->SetFlag(name, flag); }

    void OnClick(const std::string &element, const std::function<void()> &callback) override
    {
      _shown->OnClick(element, callback);
    }

    [[nodiscard]] const std::vector<neon::UiEvent> &GetEvents() const override { return _shown->GetEvents(); }

    [[nodiscard]] bool WasClicked(const std::string &element) const override { return _shown->WasClicked(element); }

    bool Focus(const std::string &element) override { return _shown->Focus(element); }

    [[nodiscard]] std::string GetFocused() const override { return _shown->GetFocused(); }
  };

  /// The extensions with the user interface of the engine: its files in
  /// memory, drawn to a renderer that keeps what it is asked to draw, and
  /// pointed at through an input the test sets.
  class ExtensionsOnTheUiTest : public ExtensionsInTheWorldTest
  {
  protected:
    std::shared_ptr<RecordingLogger> _ui_logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _ui_files{SettingsConfig{}, _ui_logger};
    neon::testing::FakeFontRasterizer _rasterizer;
    neon::Flex_LayoutEngine _layout;
    neon::testing::RecordingRenderer2D _renderer;
    NiceMock<neon::testing::FakeInputContext> _pointer{_ui_logger};
    std::unique_ptr<neon::Tree_UiSystem> _tree;
    std::unique_ptr<WatchedUi> _watched;

    [[nodiscard]] neon::UiContext *Ui() override
    {
      return _watched.get();
    }

    void SetUp() override
    {
      _ui_files.Initialize();
      _ui_files.AddNativeFile("/assets/fonts/regular.ttf", "a font");

      // the file the extension `sign` shows when it starts
      _ui_files.AddNativeFile(
        "/assets/ui/board.ui.yml",
        "ui: board\n"
        "root:\n"
        "  type: panel\n"
        "  width: 100%\n"
        "  height: 100%\n"
        "  align_items: flex-start\n"
        "  children:\n"
        "    - type: panel\n"
        "      name: board\n"
        "      width: 200\n"
        "      height: 100\n");

      _tree = std::make_unique<neon::Tree_UiSystem>(
        &_renderer,
        &_rasterizer,
        &_layout,
        &_pointer,
        &_ui_files,
        &_yaml,
        neon::UiSettings{.fonts = {{"sans-serif", 400, "assets://fonts/regular.ttf"}}, .start_path = ""},
        _ui_logger);
      _watched = std::make_unique<WatchedUi>(_tree.get());

      ExtensionsInTheWorldTest::SetUp();
    }

    void TearDown() override
    {
      ExtensionsInTheWorldTest::TearDown();
      _tree->CleanUp();
    }

    /// A frame of the user interface, as the runtime runs it.
    void Frame()
    {
      _tree->Update();
      _tree->Draw();
    }

    /// A frame with the button of the pointer held down in the middle of an
    /// element, and one with it released.
    void Click(const std::string &element)
    {
      // placed first, so that there is a middle to point at
      Frame();

      const neon::UiElement *found = _tree->Find(element);
      ASSERT_NE(found, nullptr) << element;
      const auto &box = found->GetBox();

      _pointer.state.Reset();
      _pointer.state.SetPointer(box.left + box.Width() / 2.0, box.top + box.Height() / 2.0);
      _pointer.state.SetAction(neon::Action::Pointer_Primary);
      Frame();

      _pointer.state.Reset();
      Frame();
    }

    /// A field of an element by its name, as the user interface holds it.
    template<typename T>
    [[nodiscard]] T FieldOf(const std::string &element, const std::string &field) const
    {
      neon::FieldValue value;
      EXPECT_TRUE(_tree->GetField(_tree->FindByName(element), field, value)) << field << " of " << element;

      const T *held = std::get_if<T>(&value);
      EXPECT_NE(held, nullptr) << field << " of " << element;
      return held != nullptr ? *held : T{};
    }

    /// A frame of the world with one action pressed.
    void Press(const std::string &action)
    {
      ON_CALL(_input, WasActionPressed(action)).WillByDefault(Return(true));
      _running.Update(_store, 0.016);
      ON_CALL(_input, WasActionPressed(action)).WillByDefault(Return(false));
    }
  };

  TEST_F(ExtensionsOnTheUiTest, AnExtensionShowsAFileAndMakesElementsInItThatAreFoundByTheirNames)
  {
    EXPECT_TRUE(LogOf("sign")->Contains(
      LogLevel::Info,
      "Showed the board: yes, made a sign and a button: yes, wrote on it: yes, styled it: yes, found it by its "
      "name: yes"))
      << LogOf("sign")->Messages(LogLevel::Info) << LogOf("sign")->Messages(LogLevel::Error);

    // the file once, though it was shown twice
    EXPECT_TRUE(_tree->IsShown(0));
    EXPECT_FALSE(_tree->IsShown(1));

    // the sign at the top of the file, and the button inside what the file
    // calls `board`
    const neon::UiHandle sign = _tree->FindByName("sign");
    ASSERT_TRUE(_tree->IsAlive(sign));
    EXPECT_EQ(_tree->GetElementType(sign), "label");
    EXPECT_EQ(_tree->GetParent(sign), _tree->GetRoot());

    const neon::UiHandle button = _tree->FindByName("knock");
    ASSERT_TRUE(_tree->IsAlive(button));
    EXPECT_EQ(_tree->GetParent(button), _tree->FindByName("board"));
  }

  TEST_F(ExtensionsOnTheUiTest, AnExtensionSetsTheFieldsAndTheStyleOfAnElementFromText)
  {
    // what an element shows is worked out in a frame
    Frame();

    EXPECT_EQ(_tree->GetElementText(_tree->FindByName("sign")), "Closed");
    EXPECT_EQ(_tree->GetComputed(_tree->FindByName("sign"), "color"), "rgb(255, 128, 0)");
    EXPECT_EQ(_tree->GetComputed(_tree->FindByName("sign"), "margin-left"), "12px");

    // a number and a flag are read from the text, as the field holds them
    EXPECT_TRUE(LogOf("sign")->Contains(LogLevel::Info, "A number from text: yes, a flag from text: yes"))
      << LogOf("sign")->Messages(LogLevel::Error);
    EXPECT_EQ(FieldOf<float>("filled", "value"), 75.0f);
    EXPECT_TRUE(FieldOf<bool>("lit", "checked"));
  }

  TEST_F(ExtensionsOnTheUiTest, AnExtensionIsToldOfAClickOnAnElementItListensTo)
  {
    EXPECT_TRUE(LogOf("sign")->Contains(LogLevel::Info, "Listens to the button: yes"));
    EXPECT_THAT(_watched->listening, ::testing::SizeIs(1));

    Click("knock");
    Click("knock");

    EXPECT_TRUE(LogOf("sign")->Contains(LogLevel::Info, "Heard a click on the button, knock 1"))
      << LogOf("sign")->Messages(LogLevel::Info);
    EXPECT_TRUE(LogOf("sign")->Contains(LogLevel::Info, "Heard a click on the button, knock 2"));

    // what the listener did to another element
    EXPECT_EQ(_tree->GetElementText(_tree->FindByName("sign")), "Knocked 2");
  }

  TEST_F(ExtensionsOnTheUiTest, AnExtensionIsToldNoMoreOnceItStopsListening)
  {
    Click("knock");
    Press("stop_listening");
    Click("knock");

    EXPECT_THAT(_watched->listening, IsEmpty());
    EXPECT_TRUE(LogOf("sign")->Contains(LogLevel::Info, "knock 1"));
    EXPECT_FALSE(LogOf("sign")->Contains(LogLevel::Info, "knock 2"));

    // a second time there is nothing of the extension's to take away
    Press("stop_listening");
    ExpectErrorOf("sign", "ui_unlisten was called with what the extension does not listen with");
  }

  TEST_F(ExtensionsOnTheUiTest, WhatAnExtensionListensWithIsTakenAwayWhenItIsCleanedUp)
  {
    ASSERT_THAT(_watched->listening, ::testing::SizeIs(1));

    _host.LeaveWorld();
    _host.CleanUp();

    // nothing of the user interface calls into a library that is closed
    EXPECT_THAT(_watched->listening, IsEmpty());
    Click("knock");
    EXPECT_FALSE(LogOf("sign")->Contains(LogLevel::Info, "Heard a click"));
  }

  TEST_F(ExtensionsOnTheUiTest, AnExtensionHidesAndRemovesAnElementAndItsHandleNamesNothingAfterwards)
  {
    const neon::UiHandle sign = _tree->FindByName("sign");
    Frame();
    ASSERT_TRUE(_tree->IsVisible(sign));

    Press("hide_sign");
    Frame();
    EXPECT_FALSE(_tree->IsVisible(sign));

    Press("take_down");
    EXPECT_TRUE(LogOf("sign")->Contains(LogLevel::Info, "Took the sign down: yes"));
    EXPECT_FALSE(_tree->IsAlive(sign));
    EXPECT_FALSE(_tree->FindByName("sign").IsSet());

    // the handle the extension kept names nothing now
    Press("take_down");
    EXPECT_TRUE(LogOf("sign")->Contains(LogLevel::Info, "Took the sign down: no"));
    ExpectErrorOf("sign", "ui_remove was called with an element that is not there");
  }

  TEST_F(ExtensionsOnTheUiTest, AnExtensionClosesTheFileItShowedAndItsElementsAreGone)
  {
    Press("close_board");

    EXPECT_TRUE(LogOf("sign")->Contains(LogLevel::Info, "Closed the board: yes, and the button is gone"))
      << LogOf("sign")->Messages(LogLevel::Info);
    EXPECT_FALSE(_tree->IsShown(0));

    // it shows the file no more, so there is nothing of it to close
    Press("close_board");
    ExpectErrorOf("sign", "ui_close: the extension shows no file 'assets://ui/board.ui.yml'");
  }

  TEST_F(ExtensionsOnTheUiTest, AnExtensionAsksHowLargeTheViewIs)
  {
    EXPECT_TRUE(LogOf("sign")->Contains(LogLevel::Info, "The view is 640 by 360"))
      << LogOf("sign")->Messages(LogLevel::Info);
  }

  TEST_F(ExtensionsOnTheUiTest, RefusesWhatIsNoElementFieldPropertyOrFileAndSaysWhy)
  {
    EXPECT_TRUE(LogOf("sign")->Contains(
      LogLevel::Info, "What is no element, field, property, or file was refused: yes"))
      << LogOf("sign")->Messages(LogLevel::Info);

    ExpectErrorOf("sign", "ui_create was called without text to make an element from");
    ExpectErrorOf("sign", "ui_create: the user interface made no element from the text, and said why");
    ExpectErrorOf("sign", "ui_create was called with a parent that is not there");
    ExpectErrorOf("sign", "ui_set_field was called with an element that is not there");
    ExpectErrorOf("sign", "ui_set_field: 'weight' is not a field of a label");
    ExpectErrorOf("sign", "ui_set_field: 'value' of a bar");
    ExpectErrorOf("sign", "ui_set_style: 'colour' was not set to 'red'");
    ExpectErrorOf("sign", "ui_set_visible was called with an element that is not there");
    ExpectErrorOf("sign", "ui_remove was called with an element that is not there");
    ExpectErrorOf("sign", "ui_close: the extension shows no file 'assets://ui/never.ui.yml'");
    ExpectErrorOf("sign", "ui_show: the user interface did not take 'assets://ui/never.ui.yml', and said why");
    ExpectErrorOf("sign", "ui_listen was called without an event, or without a function to tell");
    ExpectErrorOf("sign", "ui_listen was called with an element that is not there");

    // what the user interface itself said of the text that is no element
    EXPECT_TRUE(_ui_logger->Contains(LogLevel::Warn, "type 'nothing' of the element is not known")
                || _ui_logger->Contains(LogLevel::Error, "type 'nothing' of the element is not known"))
      << _ui_logger->Messages(LogLevel::Warn) << _ui_logger->Messages(LogLevel::Error);
  }

  TEST_F(ExtensionsOnTheUiTest, SaysWhenTheApplicationHasNoUserInterfaceForItsExtensions)
  {
    _host.SetUi(nullptr);

    Press("take_down");

    ExpectErrorOf("sign", "ui_remove was called, and this application has no user interface for its extensions");
  }

  TEST_F(ExtensionsInTheWorldTest, SaysWhyNothingIsMadeWhileNoFileOfTheUserInterfaceIsShown)
  {
    // the user interface of this test takes every call and shows nothing
    ExpectErrorOf("sign", "ui_create: no file of the user interface is shown, so there is nothing to put an element "
      "into");
  }

  /// Recipes in memory, and a loader that opens nothing.
  class ExtensionRecipesTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    RecordingLoggingContext _logging;
    MemoryFileSystem _file_system{SettingsConfig{}, std::make_shared<RecordingLogger>()};
    NiceMock<MockLibraryLoader> _library_loader;
    RYML_DocumentFormat _yaml;
    ExtensionHost _host{&_file_system, &_yaml, &_library_loader, &_logging, _logger};

    void SetUp() override
    {
      _file_system.Initialize();
      ON_CALL(_library_loader, GetPlatform()).WillByDefault(Return("linux-x86_64"));
    }

    /// Puts a recipe into the folder of an extension.
    void Write(const std::string &folder, const std::string &text)
    {
      _file_system.AddNativeFile("/extensions/" + folder + "/extension.yml", text);
    }

    /// Starts the extensions, and expects an error that holds the text and
    /// that none was started.
    void ExpectLeftOut(const std::string &text)
    {
      _host.Initialize();

      EXPECT_TRUE(_logger->Contains(LogLevel::Error, text)) << _logger->Messages(LogLevel::Error);
      EXPECT_THAT(_host.GetLoaded(), IsEmpty());
    }
  };

  TEST_F(ExtensionRecipesTest, HasNoExtensionsWithoutTheFolderWhichIsNoError)
  {
    EXPECT_CALL(_library_loader, Open(_, _)).Times(0);

    _host.Initialize();

    EXPECT_THAT(_host.GetLoaded(), IsEmpty());
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "No extensions"));
  }

  TEST_F(ExtensionRecipesTest, OpensTheLibraryOfThePlatformInTheFolderOfTheExtension)
  {
    Write("quake",
          "version: 1\n"
          "name: quake\n"
          "libraries:\n"
          "  macos-arm64: quake-macos-arm64.dylib\n"
          "  linux-x86_64: quake-linux-x86_64.so\n");

    EXPECT_CALL(_library_loader, Open("extensions://quake/quake-linux-x86_64.so", _))
      .WillOnce([](const std::string &, std::string &error)
      {
        error = "it is of another platform";
        return nullptr;
      });

    ExpectLeftOut("The library extensions://quake/quake-linux-x86_64.so of the extension 'quake' cannot be opened: "
      "it is of another platform");
  }

  TEST_F(ExtensionRecipesTest, LeavesOutAnExtensionWithoutALibraryForThePlatform)
  {
    Write("quake",
          "version: 1\n"
          "name: quake\n"
          "libraries:\n"
          "  macos-arm64: quake-macos-arm64.dylib\n");

    EXPECT_CALL(_library_loader, Open(_, _)).Times(0);

    ExpectLeftOut("The extension 'quake' has no library for this platform, linux-x86_64");
  }

  TEST_F(ExtensionRecipesTest, AcceptsAnExtensionThatBringsNoLibrary)
  {
    Write("palette",
          "version: 1\n"
          "name: palette\n");

    EXPECT_CALL(_library_loader, Open(_, _)).Times(0);

    _host.Initialize();

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "The extension 'palette' brings no library"));
    // what brings no library still brings its assets
    EXPECT_THAT(_host.GetAssetFolders(), ElementsAre("extensions://palette/assets/"));
  }

  TEST_F(ExtensionRecipesTest, LeavesOutAFolderWithoutARecipe)
  {
    _file_system.AddNativeFile("/extensions/quake/quake-linux-x86_64.so", "");

    ExpectLeftOut("extensions://quake has no extension.yml");
  }

  TEST_F(ExtensionRecipesTest, LeavesAFileAtTheTopAlone)
  {
    _file_system.AddNativeFile("/extensions/README.md", "");

    _host.Initialize();

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << _logger->Messages(LogLevel::Error);
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "extensions://README.md is not in the folder of an extension"));
  }

  TEST_F(ExtensionRecipesTest, RefusesARecipeWhoseNameIsNotThatOfItsFolder)
  {
    Write("quake",
          "version: 1\n"
          "name: doom\n");

    ExpectLeftOut("extensions://quake/extension.yml:2: 'name' is 'doom', and the folder of the extension is 'quake'");
  }

  TEST_F(ExtensionRecipesTest, RefusesARecipeWithoutAVersionOrAName)
  {
    Write("quake", "libraries:\n  linux-x86_64: quake.so\n");

    ExpectLeftOut("'version' is missing");
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "'name' is missing"));
  }

  TEST_F(ExtensionRecipesTest, RefusesARecipeOfALaterLayout)
  {
    Write("quake",
          "version: 2\n"
          "name: quake\n");

    ExpectLeftOut("the extension has version 2, and this engine reads up to version 1");
  }

  TEST_F(ExtensionRecipesTest, RefusesAPlatformThatIsNotKnown)
  {
    Write("quake",
          "version: 1\n"
          "name: quake\n"
          "libraries:\n"
          "  amiga-m68k: quake.library\n");

    ExpectLeftOut("extensions://quake/extension.yml:4: 'amiga-m68k' is not a platform. Those are macos-arm64, ");
  }

  TEST_F(ExtensionRecipesTest, RefusesANameThatIsNotKnown)
  {
    Write("quake",
          "version: 1\n"
          "name: quake\n"
          "library: quake.so\n");

    ExpectLeftOut("library");
  }

  TEST_F(ExtensionRecipesTest, StartsTheOthersWhenOneHasAProblem)
  {
    Write("broken", "version: 1\n");
    Write("quake",
          "version: 1\n"
          "name: quake\n"
          "libraries:\n"
          "  linux-x86_64: quake-linux-x86_64.so\n");

    EXPECT_CALL(_library_loader, Open("extensions://quake/quake-linux-x86_64.so", _)).Times(1);

    _host.Initialize();

    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "The extension 'broken' is left out until its recipe is corrected"));
  }
}
