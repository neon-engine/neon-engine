#include "script-running.hpp"

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/fake-physics-context.hpp>
#include <neon/testing/mock-script-context.hpp>
#include <neon/testing/recording-logger.hpp>

namespace
{
  using neon::ComponentFormats;
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
}
