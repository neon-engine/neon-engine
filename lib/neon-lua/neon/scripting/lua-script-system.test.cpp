#include "lua-script-system.hpp"

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>

#include <neon/common/transform.hpp>
#include <neon/reflection/type-builder.hpp>
#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/memory-file-system.hpp>
#include <neon/testing/mock-ui-system.hpp>
#include <neon/settings/settings-store.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/scene-file/component-format.hpp>

namespace
{
  using neon::ComponentFormat;
  using neon::ComponentFormats;
  using neon::ComponentId;
  using neon::ComponentInfo;
  using neon::Entity;
  using neon::FieldValue;
  using neon::Lua_ScriptSystem;
  using neon::No_Component;
  using neon::PhysicsEvent;
  using neon::PhysicsEventKind;
  using neon::SettingKind;
  using neon::SettingsStore;
  using neon::Transform;
  using neon::testing::FakeEntityStore;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using ::testing::HasSubstr;

  /// A component of the engine with the kinds a script does not declare
  /// itself: whole numbers of every width, two and four numbers, lists.
  struct Gauge
  {
    std::uint8_t level = 7;
    std::int16_t offset = -3;
    std::uint64_t ticks = 10;
    char mark = 'a';
    glm::vec2 size{1.0f, 2.0f};
    glm::vec4 tint{0.0f, 0.0f, 0.0f, 1.0f};
    glm::ivec3 cell{1, 2, 3};
    std::vector<int> steps{1, 2};
  };

  void Describe(neon::TypeBuilder<Gauge> &type)
  {
    type.Named("Gauge");
    type.Field("level", &Gauge::level);
    type.Field("offset", &Gauge::offset);
    type.Field("ticks", &Gauge::ticks);
    type.Field("mark", &Gauge::mark);
    type.Field("size", &Gauge::size);
    type.Field("tint", &Gauge::tint);
    type.Field("cell", &Gauge::cell);
    type.Field("steps", &Gauge::steps);
  }

  /// A user interface that keeps what was set for one user interface.
  class TerminalUi final : public ::testing::NiceMock<neon::testing::MockUiContext>
  {
  public:
    std::vector<std::string> set;

    void SetTextOf(const std::string &interface, const std::string &name, const std::string &text) override
    {
      set.push_back(interface + "." + name + " = " + text);
    }

    void SetNumberOf(const std::string &interface, const std::string &name, const double number) override
    {
      set.push_back(interface + "." + name + " = " + std::to_string(static_cast<int>(number * 100)) + "%");
    }

    void SetFlagOf(const std::string &interface, const std::string &name, const bool flag) override
    {
      set.push_back(interface + "." + name + " = " + (flag ? "on" : "off"));
    }
  };

  /// What the user interface asks of the scripts when an element of the
  /// terminal that says `on_click: <call>` is chosen.
  neon::ScriptUiCall Asked(const Entity entity, const std::string &call, const std::string &element = "unlock")
  {
    neon::ScriptUiCall asked;
    asked.entity = entity;
    asked.event.element = element;
    asked.event.document = "terminal";
    asked.event.surface = "screen";
    std::string problem;
    EXPECT_TRUE(neon::UiCall::Parse(call, asked.event.call, problem)) << problem;
    return asked;
  }

  /// Scripts in a file system in memory, under `assets://scripts`, run
  /// over a fake store that knows the engine's Transform.
  class LuaScriptSystemTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    MemoryFileSystem _file_system{SettingsConfig{}, _logger};
    FakeEntityStore _store;
    ComponentFormats _formats;
    Lua_ScriptSystem _lua{&_file_system, _logger};
    ComponentId _transform = No_Component;

    void SetUp() override
    {
      _file_system.Initialize();
      _store.Initialize();
      _transform = _store.RegisterComponent(ComponentInfo::Of<Transform>("Transform"));
      _formats.Add(ComponentFormat::Of<Transform>());
      _lua.Initialize();
    }

    void AddScript(const std::string &name, const std::string &text)
    {
      _file_system.AddNativeFile("/assets/scripts/" + name, text);
    }

    /// Loads the scripts and starts the systems, as the world does.
    bool Load()
    {
      const bool loaded = _lua.LoadScripts("assets://scripts", _store, _formats);
      _lua.Start(_store);
      return loaded;
    }

    Entity Place(const std::string &name, const glm::vec3 &position = glm::vec3{0.0f})
    {
      const Entity entity = _store.CreateEntity(name);
      Transform transform;
      transform.position = position;
      _store.SetComponent(entity, _transform, &transform);
      return entity;
    }

    /// Gives an entity a component of a script, with its defaults.
    void Give(const Entity entity, const std::string &component)
    {
      const ComponentId id = _store.FindComponent(component);
      ASSERT_NE(id, No_Component) << component;
      // the store constructs the defaults when given nothing to copy from
      std::vector<std::byte> blank(256);
      const ComponentFormat *format = _formats.Find(component);
      ASSERT_NE(format, nullptr);
      // read an empty map onto the entity, which gives it the defaults
      neon::DataValue map = neon::DataValue::Map();
      std::vector<std::string> errors;
      const neon::DataReader reader(map, "test", component, errors);
      format->read(reader, _store, entity);
    }

    FieldValue Field(const Entity entity, const std::string &component, const std::string &field)
    {
      const ComponentFormat *format = _formats.Find(component);
      if (format == nullptr || format->type == nullptr) { return {}; }
      const neon::FieldInfo *info = format->type->Find(field);
      void *object = _store.GetComponent(entity, _store.FindComponent(component));
      if (info == nullptr || object == nullptr) { return {}; }
      return info->get(object);
    }

    const Transform &TransformOf(const Entity entity)
    {
      return *static_cast<const Transform *>(_store.GetComponent(entity, _transform));
    }

    std::string Errors() const
    {
      return _logger->Messages(LogLevel::Error);
    }
  };

  TEST_F(LuaScriptSystemTest, HasNoScriptsWithoutTheFolder)
  {
    EXPECT_FALSE(Load());
    EXPECT_EQ(_lua.GetComponentCount(), 0u);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
  }

  TEST_F(LuaScriptSystemTest, DeclaresAComponentNamedAfterItsFileWithItsFieldsInOrder)
  {
    AddScript("door.lua", R"(
      return Component:extend {
        speed = 2.0,
        open = false,
        sound = "assets://sounds/door.wav",
        count = integer(3),
        at = vec3(1, 2, 3),
        tint = color(1, 0, 0),
      }
    )");

    EXPECT_TRUE(Load());
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    EXPECT_EQ(_lua.GetComponentCount(), 1u);
    EXPECT_NE(_store.FindComponent("Door"), No_Component);

    const ComponentFormat *format = _formats.Find("Door");
    ASSERT_NE(format, nullptr);
    ASSERT_NE(format->type, nullptr);
    EXPECT_THAT(format->type->GetPaths(), ::testing::ElementsAre("at", "count", "open", "sound", "speed", "tint"));
    EXPECT_EQ(format->type->Find("count")->kind, neon::FieldKind::Integer);
    EXPECT_EQ(format->type->Find("speed")->kind, neon::FieldKind::Float);
    EXPECT_EQ(format->type->Find("at")->kind, neon::FieldKind::Vector3);
    EXPECT_EQ(format->type->Find("tint")->kind, neon::FieldKind::Color);
  }

  TEST_F(LuaScriptSystemTest, NamesAComponentInPascalCaseFromASnakeCaseFile)
  {
    AddScript("turret_gun.lua", "return Component:extend { range = 12.0 }");
    EXPECT_TRUE(Load());
    EXPECT_NE(_store.FindComponent("TurretGun"), No_Component) << Errors();
  }

  TEST_F(LuaScriptSystemTest, RefusesAFieldThatIsAFunction)
  {
    AddScript("door.lua", "return Component:extend { update = function() end }");
    Load();
    EXPECT_EQ(_lua.GetComponentCount(), 0u);
    EXPECT_THAT(Errors(), HasSubstr("'update' of Door is a function; behavior belongs in a System"));
  }

  TEST_F(LuaScriptSystemTest, RefusesAFieldWhoseDefaultIsNotSomethingAComponentHolds)
  {
    AddScript("door.lua", "return Component:extend { parts = {} }");
    Load();
    EXPECT_THAT(Errors(), HasSubstr("The default of 'parts' of Door is table; a field holds a number, a bool, text, a vec2, a vec3, a vec4, a color, a quat, a mat3, or a mat4"));
  }

  TEST_F(LuaScriptSystemTest, RefusesAComponentOfTheEngine)
  {
    AddScript("transform.lua", "return Component:extend { x = 1.0 }");
    Load();
    EXPECT_THAT(Errors(), HasSubstr("a component called Transform exists already, of the engine"));
  }

  TEST_F(LuaScriptSystemTest, RefusesASystemThatDefinesWhatIsNotAHook)
  {
    AddScript("door.lua", R"(
      local Door = Component:extend { speed = 1.0 }
      local DoorSystem = System:extend(Door)
      function DoorSystem:updte(entity, door, dt) end
      return Door, DoorSystem
    )");
    Load();
    EXPECT_THAT(Errors(), HasSubstr("DoorSystem defines 'updte', which is not a hook of System. The hooks are ready, update, fixed_update, removed, on_trigger_enter, on_trigger_exit, on_collision"));
  }

  TEST_F(LuaScriptSystemTest, RefusesAHookWithTheWrongNumberOfParameters)
  {
    AddScript("door.lua", R"(
      local Door = Component:extend { speed = 1.0 }
      local DoorSystem = System:extend "Door"
      function DoorSystem:update(entity, dt) end
      return Door, DoorSystem
    )");
    Load();
    EXPECT_THAT(Errors(), HasSubstr("'update' of DoorSystem takes 3 parameters; it is called with 4"));
  }

  TEST_F(LuaScriptSystemTest, RefusesASystemOverAComponentNobodyDeclares)
  {
    AddScript("door.lua", R"(
      local Door = Component:extend { speed = 1.0 }
      local S = System:extend "Dor"
      function S:update(entity, door, dt) end
      return Door, S
    )");
    Load();
    EXPECT_THAT(Errors(), HasSubstr("runs over Dor, which no script and nothing of the engine declares; the scripts declare Door"));
  }

  TEST_F(LuaScriptSystemTest, AFileNamedAsAComponentHasToReturnOne)
  {
    AddScript("door.component.lua", R"(
      local S = System:extend "Transform"
      function S:update(entity, transform, dt) end
      return S
    )");
    Load();
    EXPECT_THAT(Errors(), HasSubstr("is named as a component, so it has to return one component and nothing else"));
  }

  TEST_F(LuaScriptSystemTest, RunsUpdateOverEveryEntityWithTheComponentAndWritesInPlace)
  {
    AddScript("spinner.lua", R"(
      local Spinner = Component:extend { speed = 1.0, turned = 0.0 }
      local SpinnerSystem = System:extend(Spinner)
      function SpinnerSystem:update(entity, spinner, dt)
        spinner.turned = spinner.turned + spinner.speed * dt
      end
      return Spinner, SpinnerSystem
    )");
    ASSERT_TRUE(Load());
    ASSERT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();

    const Entity a = Place("a");
    const Entity b = Place("b");
    const Entity other = Place("other");
    Give(a, "Spinner");
    Give(b, "Spinner");

    _lua.Update(_store, 0.5);
    _lua.Update(_store, 0.5);

    EXPECT_EQ(std::get<float>(Field(a, "Spinner", "turned")), 1.0f);
    EXPECT_EQ(std::get<float>(Field(b, "Spinner", "turned")), 1.0f);
    EXPECT_EQ(_store.GetComponent(other, _store.FindComponent("Spinner")), nullptr);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
  }

  TEST_F(LuaScriptSystemTest, ReadsAndChangesAComponentOfTheEngineThroughItsDescription)
  {
    AddScript("riser.lua", R"(
      local Riser = Component:extend { rate = 2.0 }
      local RiserSystem = System:extend("Riser", "Transform")
      function RiserSystem:fixed_update(entity, riser, transform, dt)
        transform.position.y = transform.position.y + riser.rate * dt
        transform.scale = vec3(2)
      end
      return Riser, RiserSystem
    )");
    ASSERT_TRUE(Load());

    const Entity a = Place("a", glm::vec3{0.0f, 1.0f, 0.0f});
    Give(a, "Riser");

    _lua.FixedUpdate(_store, 0.25);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    EXPECT_FLOAT_EQ(TransformOf(a).position.y, 1.5f);
    EXPECT_EQ(TransformOf(a).scale, glm::vec3{2.0f});
  }

  TEST_F(LuaScriptSystemTest, ReachesAnotherComponentOfTheEntityByItsName)
  {
    AddScript("lifter.lua", R"(
      local Lifter = Component:extend { by = 1.0 }
      local LifterSystem = System:extend "Lifter"
      function LifterSystem:update(entity, lifter, dt)
        if entity.Transform then entity.Transform.position.y = lifter.by end
        lifter._seen = (lifter._seen or 0) + 1
        lifter.by = lifter._seen
      end
      return Lifter, LifterSystem
    )");
    ASSERT_TRUE(Load());

    const Entity a = Place("a");
    Give(a, "Lifter");

    _lua.Update(_store, 0.1);
    _lua.Update(_store, 0.1);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    EXPECT_FLOAT_EQ(TransformOf(a).position.y, 1.0f);
    EXPECT_EQ(std::get<float>(Field(a, "Lifter", "by")), 2.0f);
  }

  TEST_F(LuaScriptSystemTest, SaysWhichFieldsAComponentHasWhenAScriptNamesOneItLacks)
  {
    AddScript("door.lua", R"(
      local Door = Component:extend { speed = 1.0, open = false }
      local S = System:extend "Door"
      function S:update(entity, door, dt) door.opne = true end
      return Door, S
    )");
    ASSERT_TRUE(Load());
    Give(Place("a"), "Door");

    _lua.Update(_store, 0.1);
    _lua.Update(_store, 0.1);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u) << Errors();
    EXPECT_THAT(Errors(), HasSubstr("The hook update of DoorSystem failed for entity 'a' and is switched off"));
    EXPECT_THAT(Errors(), HasSubstr("Door has no field 'opne'; the fields are open, speed"));
  }

  TEST_F(LuaScriptSystemTest, RefusesAValueOfAnotherKindThanTheFieldHolds)
  {
    AddScript("door.lua", R"(
      local Door = Component:extend { speed = 1.0 }
      local S = System:extend "Door"
      function S:update(entity, door, dt) door.speed = "fast" end
      return Door, S
    )");
    ASSERT_TRUE(Load());
    Give(Place("a"), "Door");

    _lua.Update(_store, 0.1);

    EXPECT_THAT(Errors(), HasSubstr("'speed' of Door holds a number, not string"));
  }

  TEST_F(LuaScriptSystemTest, CallsReadyOnceWhenAnEntityAppearsAndRemovedWhenItGoes)
  {
    AddScript("counter.lua", R"(
      local Counter = Component:extend { readies = integer(0), removes = integer(0) }
      local S = System:extend "Counter"
      function S:ready(entity, counter) counter.readies = counter.readies + 1 end
      function S:removed(entity) removed_entity = entity.id end
      return Counter, S
    )");
    ASSERT_TRUE(Load());

    const Entity a = Place("a");
    Give(a, "Counter");

    _lua.Update(_store, 0.1);
    _lua.Update(_store, 0.1);
    EXPECT_EQ(std::get<int>(Field(a, "Counter", "readies")), 1);

    _store.RemoveComponent(a, _store.FindComponent("Counter"));
    _lua.Update(_store, 0.1);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
  }

  TEST_F(LuaScriptSystemTest, HandsATriggerEventToTheSystemOfTheTriggersEntity)
  {
    AddScript("door.lua", R"(
      local Door = Component:extend { open = false, visitor = "" }
      local S = System:extend "Door"
      function S:on_trigger_enter(entity, door, other) door.open = true; door.visitor = other.name end
      function S:on_trigger_exit(entity, door, other) door.open = false end
      return Door, S
    )");
    ASSERT_TRUE(Load());

    const Entity door = Place("door");
    const Entity player = Place("player");
    Give(door, "Door");

    PhysicsEvent entered;
    entered.trigger = true;
    entered.first = door;
    entered.second = player;
    _lua.DispatchPhysicsEvents(_store, {entered});

    EXPECT_EQ(std::get<bool>(Field(door, "Door", "open")), true);
    EXPECT_EQ(std::get<std::string>(Field(door, "Door", "visitor")), "player");

    PhysicsEvent left = entered;
    left.kind = PhysicsEventKind::Ended;
    _lua.DispatchPhysicsEvents(_store, {left});

    EXPECT_EQ(std::get<bool>(Field(door, "Door", "open")), false);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
  }

  TEST_F(LuaScriptSystemTest, RequiresAModuleOfTheFolderOnceAndNothingOutsideIt)
  {
    AddScript("lib/twice.lua", "loads = (loads or 0) + 1\nreturn { of = function(x) return x * 2 end }");
    AddScript("door.lua", R"(
      local twice = require "lib.twice"
      local again = require "lib.twice"
      local Door = Component:extend { speed = twice.of(1.5) }
      local ok, message = pcall(require, "../secret")
      refused = message
      return Door
    )");
    ASSERT_TRUE(Load());

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    EXPECT_EQ(_lua.GetComponentCount(), 1u);
    const Entity a = Place("a");
    Give(a, "Door");
    EXPECT_EQ(std::get<float>(Field(a, "Door", "speed")), 3.0f);
  }

  TEST_F(LuaScriptSystemTest, RunsTheScriptsWithTheCompilerOffWhenAsked)
  {
    _lua.SetJit(false);
    AddScript("door.lua", R"(
      local Door = Component:extend { speed = 2.0 }
      local S = System:extend(Door)
      function S:update(entity, door, dt) door.speed = door.speed + dt end
      return Door, S
    )");
    ASSERT_TRUE(Load());
    const Entity a = Place("a");
    Give(a, "Door");
    _lua.Update(_store, 0.5);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    EXPECT_EQ(std::get<float>(Field(a, "Door", "speed")), 2.5f);
  }

  TEST_F(LuaScriptSystemTest, KeepsIoAndOsAndLoadOutOfReach)
  {
    AddScript("check.lua", "assert(io == nil and os == nil and load == nil and dofile == nil and debug == nil)");
    EXPECT_TRUE(Load());
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
  }

  TEST_F(LuaScriptSystemTest, KeepsTheForeignFunctionsAndTheCompilerOfLuaJitOutOfReach)
  {
    AddScript("check.lua", R"(
      assert(ffi == nil and jit == nil and package == nil and loadstring == nil)
      assert(not pcall(require, "ffi") and not pcall(require, "jit"))
    )");
    EXPECT_TRUE(Load());
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
  }

  TEST_F(LuaScriptSystemTest, GivesTheOperationsOnBitsAsTheBitLibrary)
  {
    AddScript("check.lua", R"(
      assert(bit.band(0xFF, 0x0F) == 15 and bit.bor(1, 4) == 5 and bit.bxor(5, 1) == 4)
      assert(bit.lshift(1, 4) == 16 and bit.rshift(256, 4) == 16 and bit.bnot(0) == -1)
    )");
    EXPECT_TRUE(Load());
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
  }

  TEST_F(LuaScriptSystemTest, RefusesAFileWithTheSyntaxOfALaterLua)
  {
    AddScript("later.lua", "local half <const> = 7 // 2");
    Load();
    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u);
    EXPECT_THAT(Errors(), HasSubstr("later.lua"));
  }

  TEST_F(LuaScriptSystemTest, IteratesTheWorldWithEach)
  {
    AddScript("seeker.lua", R"(
      local Seeker = Component:extend { found = integer(0) }
      local S = System:extend "Seeker"
      function S:update(entity, seeker, dt)
        local count = 0
        for other, transform in world.each("Transform") do
          count = count + 1
          transform.position.x = 7
        end
        seeker.found = count
      end
      return Seeker, S
    )");
    ASSERT_TRUE(Load());

    const Entity a = Place("a");
    Place("b");
    Place("c");
    Give(a, "Seeker");

    _lua.Update(_store, 0.1);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    EXPECT_EQ(std::get<int>(Field(a, "Seeker", "found")), 3);
    EXPECT_FLOAT_EQ(TransformOf(a).position.x, 7.0f);
  }

  TEST_F(LuaScriptSystemTest, AVectorMadeByAScriptIsAValueAndOneReadFromAComponentWritesThrough)
  {
    AddScript("mover.lua", R"(
      local Mover = Component:extend { step = vec3(1, 0, 0) }
      local S = System:extend("Mover", "Transform")
      function S:update(entity, mover, transform, dt)
        local p = transform.position
        p.x = p.x + mover.step.x
        local own = vec3(5, 5, 5)
        own.y = 9
        transform.scale = own + vec3(1, 1, 1)
      end
      return Mover, S
    )");
    ASSERT_TRUE(Load());

    const Entity a = Place("a");
    Give(a, "Mover");

    _lua.Update(_store, 0.1);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    EXPECT_FLOAT_EQ(TransformOf(a).position.x, 1.0f);
    EXPECT_EQ(TransformOf(a).scale, glm::vec3(6.0f, 10.0f, 6.0f));
  }

  TEST_F(LuaScriptSystemTest, FindsAScriptAnywhereUnderAssetsAndNamesModulesFromThere)
  {
    _file_system.AddNativeFile("/assets/levels/lab/door.lua", R"(
      local twice = require "scripts.lib.twice"
      return Component:extend { speed = twice.of(2) }
    )");
    AddScript("lib/twice.lua", "return { of = function(x) return x * 2 end }");
    _file_system.AddNativeFile("/assets/models/cube.obj", "not a script");

    EXPECT_TRUE(_lua.LoadScripts("assets://", _store, _formats));
    _lua.Start(_store);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    EXPECT_EQ(_lua.GetComponentCount(), 1u);
    EXPECT_NE(_store.FindComponent("Door"), No_Component);
  }

  TEST_F(LuaScriptSystemTest, DeclaresFieldsOfTwoAndFourNumbers)
  {
    AddScript("plane.lua", R"(
      local Plane = Component:extend { size = vec2(3, 4), tint = vec4(1, 0, 0, 0.5) }
      local S = System:extend "Plane"
      function S:update(entity, plane, dt)
        plane.size.x = plane.size.x + 1
        plane.tint = plane.tint * 2
      end
      return Plane, S
    )");
    ASSERT_TRUE(Load());
    ASSERT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();

    const ComponentFormat *format = _formats.Find("Plane");
    ASSERT_NE(format, nullptr);
    EXPECT_EQ(format->type->Find("size")->kind, neon::FieldKind::Vector2);
    EXPECT_EQ(format->type->Find("tint")->kind, neon::FieldKind::Vector4);

    const Entity a = Place("a");
    Give(a, "Plane");
    _lua.Update(_store, 0.1);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    EXPECT_EQ(std::get<glm::vec2>(Field(a, "Plane", "size")), glm::vec2(4.0f, 4.0f));
    EXPECT_EQ(std::get<glm::vec4>(Field(a, "Plane", "tint")), glm::vec4(2.0f, 0.0f, 0.0f, 1.0f));
  }

  TEST_F(LuaScriptSystemTest, ReadsAndWritesEveryWholeKindAndTheListsOfTheEngine)
  {
    const ComponentId gauge = _store.RegisterComponent(ComponentInfo::Of<Gauge>("Gauge"));
    _formats.Add(ComponentFormat::Of<Gauge>());

    AddScript("reader.lua", R"(
      local S = System:extend "Gauge"
      function S:update(entity, gauge, dt)
        seen = { gauge.level, gauge.offset, gauge.ticks, gauge.mark, gauge.size.y, gauge.tint.w, gauge.cell.z, gauge.steps[2] }
        gauge.level = 255
        gauge.offset = -32768
        gauge.ticks = 1099511627776
        gauge.mark = "z"
        gauge.size = vec2(5, 6)
        gauge.cell = vec3(7, 8, 9)
        gauge.steps = { 4, 5, 6 }
      end
      return S
    )");
    ASSERT_TRUE(Load());

    const Entity a = Place("a");
    const Gauge standard;
    _store.SetComponent(a, gauge, &standard);
    _lua.Update(_store, 0.1);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    const auto *after = static_cast<const Gauge *>(_store.GetComponent(a, gauge));
    ASSERT_NE(after, nullptr);
    EXPECT_EQ(after->level, 255);
    EXPECT_EQ(after->offset, -32768);
    EXPECT_EQ(after->ticks, std::uint64_t{1} << 40);
    EXPECT_EQ(after->mark, 'z');
    EXPECT_EQ(after->size, glm::vec2(5.0f, 6.0f));
    EXPECT_EQ(after->cell, glm::ivec3(7, 8, 9));
    EXPECT_EQ(after->steps, (std::vector<int>{4, 5, 6}));
  }

  TEST_F(LuaScriptSystemTest, RefusesAWholeNumberOutsideTheRangeOfItsKind)
  {
    _store.RegisterComponent(ComponentInfo::Of<Gauge>("Gauge"));
    _formats.Add(ComponentFormat::Of<Gauge>());

    AddScript("over.lua", R"(
      local S = System:extend "Gauge"
      function S:update(entity, gauge, dt) gauge.level = 256 end
      return S
    )");
    ASSERT_TRUE(Load());
    const Entity a = Place("a");
    const Gauge standard;
    _store.SetComponent(a, _store.FindComponent("Gauge"), &standard);

    _lua.Update(_store, 0.1);

    EXPECT_THAT(Errors(), HasSubstr("'level' of Gauge holds a whole number from 0 to 255, not number"));
  }

  TEST_F(LuaScriptSystemTest, HoldsAQuaternionAndMatricesAndTurnsAVectorWithThem)
  {
    AddScript("frame.lua", R"(
      local Frame = Component:extend { turn = quat(), basis = mat3(), place = mat4() }
      local S = System:extend "Frame"
      function S:update(entity, frame, dt)
        frame.turn = quat.from_euler(0, 90, 0)
        local forward = frame.turn * vec3(0, 0, -1)
        frame.basis:set(1, 1, forward.x)
        frame.place:set(1, 4, 5)
        local p, y, r = frame.turn:to_euler()
        yaw_seen = y
        moved = frame.place * vec3(1, 2, 3)
      end
      return Frame, S
    )");
    ASSERT_TRUE(Load());
    ASSERT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();

    const ComponentFormat *format = _formats.Find("Frame");
    ASSERT_NE(format, nullptr);
    EXPECT_EQ(format->type->Find("turn")->kind, neon::FieldKind::Quaternion);
    EXPECT_EQ(format->type->Find("basis")->kind, neon::FieldKind::Matrix3);
    EXPECT_EQ(format->type->Find("place")->kind, neon::FieldKind::Matrix4);

    const Entity a = Place("a");
    Give(a, "Frame");
    _lua.Update(_store, 0.1);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    const auto turn = std::get<glm::quat>(Field(a, "Frame", "turn"));
    EXPECT_NEAR(turn.y, 0.7071f, 0.001f);
    const auto basis = std::get<glm::mat3>(Field(a, "Frame", "basis"));
    EXPECT_NEAR(basis[0][0], -1.0f, 0.001f);
    const auto place = std::get<glm::mat4>(Field(a, "Frame", "place"));
    EXPECT_EQ(place[3][0], 5.0f);
  }

  TEST_F(LuaScriptSystemTest, ReachesAListOfTheEngineInPlace)
  {
    _store.RegisterComponent(ComponentInfo::Of<Gauge>("Gauge"));
    _formats.Add(ComponentFormat::Of<Gauge>());

    AddScript("steps.lua", R"(
      local S = System:extend "Gauge"
      function S:update(entity, gauge, dt)
        local steps = gauge.steps
        count_before = #steps
        steps[1] = 10
        steps:insert(30)
        steps:insert(2, 20)
        steps:remove(3)
        count_after = #gauge.steps
        second = gauge.steps[2]
      end
      return S
    )");
    ASSERT_TRUE(Load());

    const Entity a = Place("a");
    const Gauge standard;
    _store.SetComponent(a, _store.FindComponent("Gauge"), &standard);
    _lua.Update(_store, 0.1);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    const auto *after = static_cast<const Gauge *>(_store.GetComponent(a, _store.FindComponent("Gauge")));
    ASSERT_NE(after, nullptr);
    EXPECT_EQ(after->steps, (std::vector<int>{10, 20, 30}));
  }

  TEST_F(LuaScriptSystemTest, SaysWhenAListIndexIsPastTheEnd)
  {
    _store.RegisterComponent(ComponentInfo::Of<Gauge>("Gauge"));
    _formats.Add(ComponentFormat::Of<Gauge>());
    AddScript("over.lua", R"(
      local S = System:extend "Gauge"
      function S:update(entity, gauge, dt) gauge.steps[5] = 1 end
      return S
    )");
    ASSERT_TRUE(Load());
    const Entity a = Place("a");
    const Gauge standard;
    _store.SetComponent(a, _store.FindComponent("Gauge"), &standard);

    _lua.Update(_store, 0.1);

    EXPECT_THAT(Errors(), HasSubstr("The list has 2 elements, so there is no element 5"));
  }

  TEST_F(LuaScriptSystemTest, ReadingVectorsAndComponentsEveryFrameMakesNothingNew)
  {
    // the handles read from a component are kept and bound again, so a
    // read every frame makes nothing Lua has to collect: with the collector
    // held still, the memory in use stays the same from one frame to the next
    AddScript("reader.lua", R"(
      local Reader = Component:extend { at = vec3(0, 0, 0), grew = 0.0 }
      local S = System:extend("Reader", "Transform")
      collectgarbage('stop')
      local last = nil
      function S:update(entity, reader, transform, dt)
        local p = transform.position
        local a = reader.at
        local t = entity.Transform
        p.x = p.x + a.y + t.scale.y * 0
        local now = collectgarbage('count')
        if last ~= nil and now > last then reader.grew = reader.grew + (now - last) end
        last = now
      end
      return Reader, S
    )");
    ASSERT_TRUE(Load());
    const Entity a = Place("a");
    Give(a, "Reader");

    // the first frames make the handles
    for (int i = 0; i < 3; i++) { _lua.Update(_store, 0.1); }
    const float grew_at_start = std::get<float>(Field(a, "Reader", "grew"));
    for (int i = 0; i < 200; i++) { _lua.Update(_store, 0.1); }

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    EXPECT_EQ(std::get<float>(Field(a, "Reader", "grew")), grew_at_start);
  }

  // the user interface

  TEST_F(LuaScriptSystemTest, AnElementCallsAFunctionOfTheSystemOfTheEntityThatShowsIt)
  {
    AddScript("panel.lua", R"(
      local Panel = Component:extend { presses = 0 }
      local PanelSystem = System:extend("Panel", "Transform")
      function PanelSystem.handlers:unlock()
        self.Panel.presses = self.Panel.presses + 1
        self.Transform.position.y = 2
        self.Panel._seen = (self.Panel._seen or 0) + 1
        if self.Panel._seen == 3 then self.Transform.position.x = 3 end
      end
      return Panel, PanelSystem
    )");
    ASSERT_TRUE(Load());

    const Entity a = Place("a");
    const Entity b = Place("b");
    Give(a, "Panel");
    Give(b, "Panel");

    _lua.DispatchUiCalls(_store, {Asked(a, "unlock")});
    _lua.DispatchUiCalls(_store, {Asked(a, "unlock()"), Asked(a, "unlock")});

    // the entity that shows the user interface, and no other
    EXPECT_EQ(std::get<float>(Field(a, "Panel", "presses")), 3.0f);
    EXPECT_EQ(std::get<float>(Field(b, "Panel", "presses")), 0.0f);
    EXPECT_FLOAT_EQ(TransformOf(a).position.y, 2.0f);
    EXPECT_FLOAT_EQ(TransformOf(b).position.y, 0.0f);

    // what a handler keeps for itself on a component is kept between calls
    EXPECT_FLOAT_EQ(TransformOf(a).position.x, 3.0f);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 0u) << _logger->Messages(LogLevel::Warn);
  }

  TEST_F(LuaScriptSystemTest, AFunctionAnElementCallsIsHandedWhatTheFileWrote)
  {
    AddScript("panel.lua", R"(
      local Panel = Component:extend { said = "", sum = 0, flag = false, missing = false }
      local PanelSystem = System:extend "Panel"
      function PanelSystem.handlers:open(what, by, flag, nothing, event, beyond)
        local panel = self.Panel
        panel.said = what .. " " .. event.kind .. " " .. event.element .. " " .. event.interface .. " " .. event.surface
        panel.sum = by
        panel.flag = flag
        panel.missing = nothing == nil and beyond == nil
      end
      return Panel, PanelSystem
    )");
    ASSERT_TRUE(Load());

    const Entity a = Place("a");
    Give(a, "Panel");

    // a value that was named and is not there is nothing
    neon::ScriptUiCall asked = Asked(a, "open('safe', 2.5, true, gone, $event)", "open");
    asked.event.call.arguments[3].kind = neon::UiCallArgument::Kind::Nothing;
    _lua.DispatchUiCalls(_store, {asked});

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    EXPECT_EQ(std::get<std::string>(Field(a, "Panel", "said")), "safe click open terminal screen");
    EXPECT_EQ(std::get<float>(Field(a, "Panel", "sum")), 2.5f);
    EXPECT_TRUE(std::get<bool>(Field(a, "Panel", "flag")));
    EXPECT_TRUE(std::get<bool>(Field(a, "Panel", "missing")));
  }

  TEST_F(LuaScriptSystemTest, TheEventSaysWhoTheClickComesFrom)
  {
    AddScript("panel.lua", R"(
      local Panel = Component:extend { by = "", anyone = true }
      local PanelSystem = System:extend "Panel"
      function PanelSystem.handlers:unlock(event)
        self.Panel.anyone = event.instigator ~= nil
        if event.instigator then
          self.Panel.by = event.instigator.name
          event.instigator.Transform.position.y = 5
        end
      end
      return Panel, PanelSystem
    )");
    ASSERT_TRUE(Load());

    const Entity a = Place("a");
    const Entity player = Place("player");
    Give(a, "Panel");

    neon::ScriptUiCall asked = Asked(a, "unlock($event)");
    asked.instigator = player;
    _lua.DispatchUiCalls(_store, {asked});

    EXPECT_EQ(std::get<std::string>(Field(a, "Panel", "by")), "player");
    EXPECT_FLOAT_EQ(TransformOf(player).position.y, 5.0f);

    // a click on the window comes from no entity
    _lua.DispatchUiCalls(_store, {Asked(a, "unlock($event)")});
    EXPECT_FALSE(std::get<bool>(Field(a, "Panel", "anyone")));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
  }

  TEST_F(LuaScriptSystemTest, AChangeHandsTheEventWithTheValueAsWhatItIs)
  {
    AddScript("panel.lua", R"(
      local Panel = Component:extend { said = "", volume = 0, vsync = false, quality = "" }
      local PanelSystem = System:extend "Panel"
      function PanelSystem.handlers:tune(what, event)
        local panel = self.Panel
        panel.said = what .. " " .. event.kind .. " " .. event.element .. " " .. event.interface .. " " .. type(event.value)
        if type(event.value) == "number" then panel.volume = event.value end
        if type(event.value) == "boolean" then panel.vsync = event.value end
        if type(event.value) == "string" then panel.quality = event.value end
      end
      return Panel, PanelSystem
    )");
    ASSERT_TRUE(Load());

    const Entity a = Place("a");
    Give(a, "Panel");

    const auto changed = [&a](const std::string &element, const neon::UiValue &value)
    {
      neon::ScriptUiCall asked = Asked(a, "tune('" + element + "', $event)", element);
      asked.event.kind = neon::UiEvent::Kind::Change;
      asked.event.value = value;
      return asked;
    };

    _lua.DispatchUiCalls(_store, {changed("volume", neon::UiValue::Number(0.75))});
    EXPECT_EQ(std::get<std::string>(Field(a, "Panel", "said")), "volume change volume terminal number");
    EXPECT_FLOAT_EQ(std::get<float>(Field(a, "Panel", "volume")), 0.75f);

    _lua.DispatchUiCalls(_store, {changed("vsync", neon::UiValue::Flag(true)), changed("quality", neon::UiValue::Text("high"))});
    EXPECT_TRUE(std::get<bool>(Field(a, "Panel", "vsync")));
    EXPECT_EQ(std::get<std::string>(Field(a, "Panel", "quality")), "high");
    EXPECT_EQ(std::get<std::string>(Field(a, "Panel", "said")), "quality change quality terminal string");
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
  }

  TEST_F(LuaScriptSystemTest, EverySystemOverTheEntityThatHasTheFunctionIsCalled)
  {
    AddScript("panel.lua", R"(
      local Panel = Component:extend { presses = 0 }
      local PanelSystem = System:extend "Panel"
      function PanelSystem.handlers:unlock() self.Panel.presses = self.Panel.presses + 1 end
      local Lifting = System:extend "Transform"
      function Lifting.handlers:unlock() self.Transform.position.y = self.Transform.position.y + 1 end
      local Deaf = System:extend "Transform"
      function Deaf.handlers:lock() self.Transform.position.x = 9 end
      return Panel, PanelSystem, Lifting, Deaf
    )");
    ASSERT_TRUE(Load());

    const Entity a = Place("a");
    Give(a, "Panel");

    _lua.DispatchUiCalls(_store, {Asked(a, "unlock")});

    EXPECT_EQ(std::get<float>(Field(a, "Panel", "presses")), 1.0f);
    EXPECT_FLOAT_EQ(TransformOf(a).position.y, 1.0f);
    EXPECT_FLOAT_EQ(TransformOf(a).position.x, 0.0f);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
  }

  TEST_F(LuaScriptSystemTest, SaysSoWhenNothingCanBeCalled)
  {
    AddScript("panel.lua", R"(
      local Panel = Component:extend { presses = 0 }
      local PanelSystem = System:extend "Panel"
      function PanelSystem:update(entity, panel, dt) panel.presses = panel.presses + 1 end
      return Panel, PanelSystem
    )");
    ASSERT_TRUE(Load());

    const Entity a = Place("a");
    Give(a, "Panel");

    // no handler of that name, and neither a hook nor what a system
    // inherits is one
    _lua.DispatchUiCalls(_store, {Asked(a, "unlock"), Asked(a, "extend"), Asked(a, "update(1)")});
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Warn,
      "'unlock' of the user interface terminal calls unlock, and no system over the entity a has a handler of that name"))
      << _logger->Messages(LogLevel::Warn);
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 3u);
    EXPECT_EQ(std::get<float>(Field(a, "Panel", "presses")), 0.0f);

    // a user interface that no entity shows
    _lua.DispatchUiCalls(_store, {Asked(neon::No_Entity, "unlock")});
    EXPECT_TRUE(_logger->Contains(LogLevel::Warn, "no entity shows that user interface"));

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
  }

  TEST_F(LuaScriptSystemTest, AFunctionAnElementCallsThatFailsIsReportedAndCalledAgainTheNextTime)
  {
    AddScript("panel.lua", R"(
      local Panel = Component:extend { presses = 0 }
      local PanelSystem = System:extend "Panel"
      function PanelSystem.handlers:unlock(fail)
        if fail then error("the bolt is stuck") end
        self.Panel.presses = self.Panel.presses + 1
      end
      return Panel, PanelSystem
    )");
    ASSERT_TRUE(Load());

    const Entity a = Place("a");
    Give(a, "Panel");

    _lua.DispatchUiCalls(_store, {Asked(a, "unlock(true)")});
    EXPECT_THAT(Errors(), HasSubstr("unlock of PanelSystem failed for a, called by 'unlock' of the user interface terminal"));
    EXPECT_THAT(Errors(), HasSubstr("the bolt is stuck"));

    _lua.DispatchUiCalls(_store, {Asked(a, "unlock(false)")});
    EXPECT_EQ(std::get<float>(Field(a, "Panel", "presses")), 1.0f);
    EXPECT_EQ(_logger->Count(LogLevel::Error), 1u) << Errors();
  }

  TEST_F(LuaScriptSystemTest, AFunctionOfASystemThatIsNoHookIsRefusedAndPointedAtTheHandlers)
  {
    AddScript("panel.lua", R"(
      local Panel = Component:extend { presses = 0 }
      local PanelSystem = System:extend "Panel"
      function PanelSystem:unlock(entity, panel) end
      return Panel, PanelSystem
    )");
    AddScript("wrong.lua", R"(
      local Wrong = Component:extend { presses = 0 }
      local WrongSystem = System:extend "Wrong"
      WrongSystem.handlers.unlock = 3
      return Wrong, WrongSystem
    )");
    Load();

    EXPECT_THAT(Errors(), HasSubstr("function PanelSystem.handlers:unlock(...)"));
    EXPECT_THAT(Errors(), HasSubstr("'handlers' of WrongSystem holds something that is not a function with a name"));
  }

  TEST_F(LuaScriptSystemTest, SetsTheValuesOfTheUserInterface)
  {
    AddScript("panel.lua", R"(
      local Panel = Component:extend { presses = 0 }
      local PanelSystem = System:extend "Panel"
      function PanelSystem.handlers:unlock()
        ui.set_text_of("terminal", "door", "unlocked")
        ui.set_number_of("terminal", "power", 0.75)
        ui.set_flag_of("terminal", "open", true)
        ui.set_text("title", "Started")
        ui.set_number("score", 3)
        ui.set_flag("playing", true)
      end
      return Panel, PanelSystem
    )");
    ASSERT_TRUE(Load());

    const Entity a = Place("a");
    Give(a, "Panel");

    // without a user interface what is set is dropped
    _lua.DispatchUiCalls(_store, {Asked(a, "unlock")});
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();

    TerminalUi ui;
    _lua.SetUi(&ui);
    EXPECT_CALL(ui, SetText("title", "Started"));
    EXPECT_CALL(ui, SetNumber("score", 3.0));
    EXPECT_CALL(ui, SetFlag("playing", true));
    _lua.DispatchUiCalls(_store, {Asked(a, "unlock")});

    EXPECT_THAT(
      ui.set,
      ::testing::ElementsAre("terminal.door = unlocked", "terminal.power = 75%", "terminal.open = on"));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
  }

  TEST_F(LuaScriptSystemTest, RefusesAValueOfTheUserInterfaceOfTheWrongKind)
  {
    AddScript("panel.lua", R"(
      local Panel = Component:extend { presses = 0 }
      local PanelSystem = System:extend "Panel"
      function PanelSystem:update(entity, panel, dt)
        ui.set_flag("playing", 1)
      end
      return Panel, PanelSystem
    )");
    ASSERT_TRUE(Load());

    Give(Place("a"), "Panel");
    _lua.Update(_store, 0.1);

    EXPECT_THAT(Errors(), HasSubstr("boolean expected"));
  }

  // the settings of the game

  class LuaSettingsTest : public LuaScriptSystemTest
  {
  protected:
    SettingsStore _settings{_logger};

    void SetUp() override
    {
      LuaScriptSystemTest::SetUp();
      ASSERT_TRUE(_settings.Declare({.name = "subtitles", .kind = SettingKind::Flag, .default_value = neon::DataValue::Bool(true)}));
      ASSERT_TRUE(_settings.Declare({
        .name = "field_of_view", .kind = SettingKind::Number, .default_value = neon::DataValue::Number(90.0), .least = 60.0,
        .most = 120.0
      }));
      ASSERT_TRUE(_settings.Declare({
        .name = "difficulty", .kind = SettingKind::Choice, .default_value = neon::DataValue::Text("normal"),
        .choices = {"easy", "normal", "hard"}
      }));
      ASSERT_TRUE(_settings.Declare({.name = "reset_progress", .kind = SettingKind::Action}));
      _lua.SetSettings(&_settings);
    }
  };

  TEST_F(LuaSettingsTest, ReadsAndSetsTheSettingsOfTheGame)
  {
    AddScript("probe.lua", R"(
      local Probe = Component:extend { done = false }
      local ProbeSystem = System:extend "Probe"
      function ProbeSystem:update(entity, probe, dt)
        if probe.done then return end
        probe.done = true
        log.info("subtitles", settings.get("subtitles"), "fov", settings.get("field_of_view"),
                 "difficulty", settings.get("difficulty"), "reset", settings.get("reset_progress"))
        log.info("set hard", settings.set("difficulty", "hard"))
        log.info("set wide", settings.set("field_of_view", 200))
        log.info("set off", settings.set("subtitles", false))
        log.info("trigger", settings.trigger("reset_progress"))
      end
      return Probe, ProbeSystem
    )");
    ASSERT_TRUE(Load());
    Give(Place("a"), "Probe");

    int pressed = 0;
    const neon::SettingsSubscription subscription = _settings.OnChange("reset_progress", [&](const std::string &, const neon::DataValue &) { pressed++; });
    _lua.Update(_store, 0.1);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    const std::string info = _logger->Messages(LogLevel::Info);
    EXPECT_THAT(info, HasSubstr("subtitles true fov 90 difficulty normal reset nil"));
    EXPECT_THAT(info, HasSubstr("set hard true"));
    EXPECT_THAT(info, HasSubstr("set wide false")) << "above the most, which the store says";
    EXPECT_THAT(info, HasSubstr("set off true"));
    EXPECT_THAT(info, HasSubstr("trigger true"));
    EXPECT_EQ(pressed, 1);

    std::string difficulty;
    EXPECT_TRUE(_settings.GetText("difficulty", difficulty));
    EXPECT_EQ(difficulty, "hard");
    bool subtitles = true;
    EXPECT_TRUE(_settings.GetFlag("subtitles", subtitles));
    EXPECT_FALSE(subtitles);
    double field_of_view = 0.0;
    EXPECT_TRUE(_settings.GetNumber("field_of_view", field_of_view));
    EXPECT_DOUBLE_EQ(field_of_view, 90.0);
  }

  TEST_F(LuaSettingsTest, HearsOfAChangeForAsLongAsTheScriptsRun)
  {
    AddScript("probe.lua", R"(
      local Probe = Component:extend { heard = 0 }
      local ProbeSystem = System:extend "Probe"
      function ProbeSystem:ready(entity, probe)
        settings.on_change("difficulty", function(value) log.info("difficulty is now", value) end)
        settings.on_change("reset_progress", function(value) log.info("reset with", value) end)
      end
      return Probe, ProbeSystem
    )");
    ASSERT_TRUE(Load());
    Give(Place("a"), "Probe");
    _lua.Update(_store, 0.1);
    const std::size_t before = _logger->Count(LogLevel::Info);

    EXPECT_TRUE(_settings.Set("difficulty", neon::DataValue::Text("easy")));
    EXPECT_TRUE(_settings.Set("difficulty", neon::DataValue::Text("easy"))) << "the same, which is no change";
    EXPECT_TRUE(_settings.Trigger("reset_progress"));
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    const std::string info = _logger->Messages(LogLevel::Info);
    EXPECT_THAT(info, HasSubstr("difficulty is now easy"));
    EXPECT_THAT(info, HasSubstr("reset with nil"));
    EXPECT_EQ(_logger->Count(LogLevel::Info), before + 2) << info;

    // once the scripts are gone nothing is called, and nothing goes wrong
    _lua.CleanUp();
    EXPECT_TRUE(_settings.Set("difficulty", neon::DataValue::Text("hard")));
    EXPECT_EQ(_logger->Count(LogLevel::Info), before + 2);
  }

  TEST_F(LuaSettingsTest, AFunctionThatFailsIsReportedWithTheSetting)
  {
    AddScript("probe.lua", R"(
      local Probe = Component:extend { heard = 0 }
      local ProbeSystem = System:extend "Probe"
      function ProbeSystem:ready(entity, probe)
        settings.on_change("subtitles", function(value) error("no subtitles today") end)
      end
      return Probe, ProbeSystem
    )");
    ASSERT_TRUE(Load());
    Give(Place("a"), "Probe");
    _lua.Update(_store, 0.1);

    EXPECT_TRUE(_settings.Set("subtitles", neon::DataValue::Bool(false)));
    EXPECT_THAT(Errors(), HasSubstr("settings.on_change was given for subtitles failed"));
    EXPECT_THAT(Errors(), HasSubstr("no subtitles today"));
  }

  TEST_F(LuaSettingsTest, ANameThatIsNotDeclaredIsAnErrorThatListsTheNames)
  {
    AddScript("probe.lua", R"(
      local Probe = Component:extend { heard = 0 }
      local ProbeSystem = System:extend "Probe"
      function ProbeSystem:update(entity, probe, dt)
        settings.get("volume")
      end
      return Probe, ProbeSystem
    )");
    ASSERT_TRUE(Load());
    Give(Place("a"), "Probe");
    _lua.Update(_store, 0.1);

    EXPECT_THAT(Errors(), HasSubstr("There is no setting called 'volume'. The settings are: subtitles, field_of_view, difficulty, reset_progress"));
  }

  TEST_F(LuaSettingsTest, WithoutAStoreEverySettingIsNilAndNothingIsSet)
  {
    AddScript("probe.lua", R"(
      local Probe = Component:extend { heard = 0 }
      local ProbeSystem = System:extend "Probe"
      function ProbeSystem:update(entity, probe, dt)
        log.info("get", settings.get("subtitles"), "set", settings.set("subtitles", false))
        settings.on_change("subtitles", function(value) end)
      end
      return Probe, ProbeSystem
    )");
    _lua.SetSettings(nullptr);
    ASSERT_TRUE(Load());
    Give(Place("a"), "Probe");
    _lua.Update(_store, 0.1);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
    EXPECT_THAT(_logger->Messages(LogLevel::Info), HasSubstr("get nil set false"));
  }
}
