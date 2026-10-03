#include "lua-script-system.hpp"

#include <memory>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/memory-file-system.hpp>
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
  using neon::Transform;
  using neon::testing::FakeEntityStore;
  using neon::testing::LogLevel;
  using neon::testing::MemoryFileSystem;
  using neon::testing::RecordingLogger;
  using ::testing::HasSubstr;

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
        count = 3,
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
    EXPECT_THAT(Errors(), HasSubstr("'update' of Door is a function; behaviour belongs in a System"));
  }

  TEST_F(LuaScriptSystemTest, RefusesAFieldWhoseDefaultIsNotSomethingAComponentHolds)
  {
    AddScript("door.lua", "return Component:extend { parts = {} }");
    Load();
    EXPECT_THAT(Errors(), HasSubstr("The default of 'parts' of Door is table; a field holds a number, a bool, text, a vec3, or a color"));
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
    EXPECT_THAT(Errors(), HasSubstr("DoorSystem defines 'updte', which is not a hook of System. The hooks are ready, update, step, removed, on_trigger_enter, on_trigger_exit, on_collision"));
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
      function RiserSystem:step(entity, riser, transform, dt)
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
      local Counter = Component:extend { readies = 0, removes = 0 }
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

  TEST_F(LuaScriptSystemTest, KeepsIoAndOsAndLoadOutOfReach)
  {
    AddScript("check.lua", "assert(io == nil and os == nil and load == nil and dofile == nil and debug == nil)");
    EXPECT_TRUE(Load());
    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u) << Errors();
  }

  TEST_F(LuaScriptSystemTest, IteratesTheWorldWithEach)
  {
    AddScript("seeker.lua", R"(
      local Seeker = Component:extend { found = 0 }
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
}
