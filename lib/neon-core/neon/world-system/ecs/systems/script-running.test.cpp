#include "script-running.hpp"

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/fake-physics-context.hpp>
#include <neon/testing/mock-script-context.hpp>
#include <neon/testing/mock-ui-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/ui-surface-view.hpp>
#include <neon/world-system/ecs/components/ui-view.hpp>

namespace
{
  using neon::ComponentFormats;
  using neon::Entity;
  using neon::ScriptUiCall;
  using neon::UiEvent;
  using neon::UiSurfaceView;
  using neon::UiView;
  using neon::testing::MockUiContext;
  using ::testing::NiceMock;
  using ::testing::ReturnRef;
  using ::testing::SaveArg;
  using neon::PhysicsEvent;
  using neon::ScriptRunning;
  using neon::testing::FakeEntityStore;
  using neon::testing::FakePhysicsContext;
  using neon::testing::LogLevel;
  using neon::testing::MockScriptContext;
  using neon::testing::RecordingLogger;
  using ::testing::_;
  using ::testing::InSequence;
  using ::testing::Return;
  using ::testing::SizeIs;

  class ScriptRunningTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    FakeEntityStore _store;
    FakePhysicsContext _physics;
    ComponentFormats _formats;
    MockScriptContext _scripts;

    void SetUp() override
    {
      _store.Initialize();
    }
  };

  TEST_F(ScriptRunningTest, LoadsTheScriptsWhenComponentsAreRegisteredAndSaysWhatTheyDeclare)
  {
    ScriptRunning system(&_scripts, &_physics, &_formats, "assets://scripts", _logger);

    EXPECT_CALL(_scripts, LoadScripts("assets://scripts", _, _)).WillOnce(Return(true));
    EXPECT_CALL(_scripts, GetComponentCount()).WillRepeatedly(Return(2));
    EXPECT_CALL(_scripts, GetSystemCount()).WillRepeatedly(Return(3));
    system.Register(_store);

    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "Scripts under assets://scripts declare 2 components and 3 systems"))
      << _logger->Messages(LogLevel::Info);

    EXPECT_CALL(_scripts, Start(_));
    system.Initialize(_store);
  }

  TEST_F(ScriptRunningTest, AGameWithoutScriptsHasNoFolderAndThatIsNotAnError)
  {
    ScriptRunning system(&_scripts, &_physics, &_formats, "assets://scripts", _logger);

    EXPECT_CALL(_scripts, LoadScripts(_, _, _)).WillOnce(Return(false));
    system.Register(_store);

    EXPECT_EQ(_logger->Count(LogLevel::Error), 0u);
    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "No scripts: there is no folder assets://scripts"));
  }

  TEST_F(ScriptRunningTest, HandsTheFrameEventsOfThePhysicsOverBeforeTheUpdate)
  {
    ScriptRunning system(&_scripts, &_physics, &_formats, "assets://scripts", _logger);

    PhysicsEvent event;
    event.trigger = true;
    _physics.next_events = {event};
    _physics.Step(0.1);

    InSequence in_order;
    EXPECT_CALL(_scripts, DispatchPhysicsEvents(_, SizeIs(1)));
    EXPECT_CALL(_scripts, Update(_, 0.25));
    system.Update(_store, 0.25);

    EXPECT_CALL(_scripts, FixedUpdate(_, 0.05));
    system.FixedUpdate(_store, 0.05);
  }

  TEST_F(ScriptRunningTest, WithoutPhysicsNoEventIsHandedOver)
  {
    ScriptRunning system(&_scripts, nullptr, &_formats, "assets://scripts", _logger);

    EXPECT_CALL(_scripts, DispatchPhysicsEvents(_, _)).Times(0);
    EXPECT_CALL(_scripts, Update(_, 0.25));
    system.Update(_store, 0.25);
  }

  TEST_F(ScriptRunningTest, HandsOverWhatTheUserInterfaceCallsWithTheEntityThatShowsIt)
  {
    _store.Register<UiView>("Ui");
    _store.Register<UiSurfaceView>("UiSurface");

    const Entity hud = _store.CreateEntity("hud");
    _store.Set(hud, UiView{.file = "assets://ui/hud.ui.yml", .document = 3});
    const Entity terminal = _store.CreateEntity("terminal");
    const Entity player = _store.CreateEntity("player");
    const Entity gone = _store.CreateEntity("gone");
    _store.Set(terminal, UiSurfaceView{.name = "terminal", .surface = 1, .document = 5, .pointed_by = player});
    const Entity sign = _store.CreateEntity("sign");
    _store.Set(sign, UiSurfaceView{.name = "sign", .surface = 2, .document = 6, .pointed_by = gone});
    _store.DestroyEntity(gone);

    NiceMock<MockUiContext> ui;
    std::vector<UiEvent> events(5);
    events[4].element = "read";
    events[4].document_id = 6;
    events[4].call.function = "read";
    events[0].element = "resume";
    events[1].element = "unlock";
    events[1].document_id = 5;
    events[1].call.function = "unlock";
    events[2].element = "map";
    events[2].document_id = 3;
    events[2].call.function = "open_map";
    events[3].element = "quit";
    events[3].document_id = 9;
    events[3].call.function = "quit";
    EXPECT_CALL(ui, GetEvents()).WillRepeatedly(ReturnRef(events));

    ScriptRunning system(&_scripts, nullptr, &_formats, "assets://scripts", _logger);
    system.SetUi(&ui);
    EXPECT_CALL(_scripts, Start(_));
    system.Initialize(_store);

    // a click that calls nothing is left out; a file no entity shows is
    // handed over without one
    std::vector<ScriptUiCall> calls;
    InSequence in_order;
    EXPECT_CALL(_scripts, DispatchUiCalls(_, SizeIs(4))).WillOnce(SaveArg<1>(&calls));
    EXPECT_CALL(_scripts, Update(_, 0.25));
    system.Update(_store, 0.25);

    ASSERT_EQ(calls.size(), 4u);
    EXPECT_EQ(calls[0].entity, terminal);
    EXPECT_EQ(calls[0].event.call.function, "unlock");
    EXPECT_EQ(calls[1].entity, hud);
    EXPECT_EQ(calls[1].event.call.function, "open_map");
    EXPECT_EQ(calls[2].entity, neon::No_Entity);

    // a click on a surface in the world comes from who pointed at it; one
    // on the window from no entity, and so does one from who is gone
    EXPECT_EQ(calls[0].instigator, player);
    EXPECT_EQ(calls[1].instigator, neon::No_Entity);
    EXPECT_EQ(calls[2].instigator, neon::No_Entity);
    EXPECT_EQ(calls[3].entity, sign);
    EXPECT_EQ(calls[3].instigator, neon::No_Entity);

    // and nothing is handed over in a frame without a call
    events.resize(1);
    EXPECT_CALL(_scripts, DispatchUiCalls(_, _)).Times(0);
    EXPECT_CALL(_scripts, Update(_, 0.25));
    system.Update(_store, 0.25);
  }

  TEST_F(ScriptRunningTest, WithoutAUserInterfaceNothingIsCalled)
  {
    ScriptRunning system(&_scripts, nullptr, &_formats, "assets://scripts", _logger);

    EXPECT_CALL(_scripts, DispatchUiCalls(_, _)).Times(0);
    EXPECT_CALL(_scripts, Update(_, 0.25));
    system.Update(_store, 0.25);
  }

  TEST_F(ScriptRunningTest, ReadsTheScriptsOfMoreFoldersAfterTheFirstAndLeavesOutThoseThatAreNotThere)
  {
    ScriptRunning system(&_scripts, &_physics, &_formats, "assets://", _logger);
    system.AddFolder("extensions://bench/assets/");
    system.AddFolder("extensions://quake/assets/");

    ::testing::InSequence in_order;
    EXPECT_CALL(_scripts, LoadScripts("assets://", _, _)).WillOnce(Return(false));
    EXPECT_CALL(_scripts, LoadScripts("extensions://bench/assets/", _, _)).WillOnce(Return(true));
    EXPECT_CALL(_scripts, LoadScripts("extensions://quake/assets/", _, _)).WillOnce(Return(false));

    system.Register(_store);

    EXPECT_TRUE(_logger->Contains(LogLevel::Info, "With the scripts under extensions://bench/assets/, "));
    EXPECT_FALSE(_logger->Contains(LogLevel::Info, "extensions://quake"));
  }

}
