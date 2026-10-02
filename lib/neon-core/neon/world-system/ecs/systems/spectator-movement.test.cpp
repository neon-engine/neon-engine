#include "spectator-movement.hpp"

#include <memory>

#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/mock-input-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/spectator.hpp>

namespace
{
  using neon::Axis;
  using neon::Entity;
  using neon::Key;
  using neon::Spectator;
  using neon::SpectatorMovement;
  using neon::Transform;
  using neon::testing::FakeEntityStore;
  using neon::testing::FakeInputContext;
  using neon::testing::RecordingLogger;
  using ::testing::NiceMock;

  constexpr float tolerance = 1e-5f;

  void ExpectVector(const glm::vec3 &actual, const float x, const float y, const float z)
  {
    EXPECT_NEAR(actual.x, x, tolerance);
    EXPECT_NEAR(actual.y, y, tolerance);
    EXPECT_NEAR(actual.z, z, tolerance);
  }

  /// A transform that was placed in the world with a yaw.
  Transform Turned(const float yaw)
  {
    Transform transform;
    transform.rotation.yaw = yaw;
    transform.world_coordinates = mat4_cast(transform.rotation.GetQuaternion());
    return transform;
  }

  class SpectatorMovementTest : public ::testing::Test
  {
  protected:
    FakeEntityStore _store;
    NiceMock<FakeInputContext> _input{std::make_shared<RecordingLogger>()};
    SpectatorMovement _system{&_input};

    void SetUp() override
    {
      _store.Initialize();
      _store.Register<Transform>("Transform");
      _store.Register<Spectator>("Spectator");
      _system.Initialize(_store);
    }

    /// A spectator that moves 2 units per second and turns half a degree
    /// for every unit of the mouse.
    Entity CreateSpectator(const Transform &transform = {})
    {
      const Entity entity = _store.CreateEntity("");
      _store.Set(entity, transform);
      _store.Set(entity, Spectator{.move_speed = 2.0f, .look_speed = 0.5f});
      return entity;
    }

    const Transform &TransformOf(const Entity entity)
    {
      return *_store.Get<Transform>(entity);
    }

    /// A frame: the actions are worked out from the keys, and the system
    /// runs. The map is the engine's default, with `move` on W, A, S, D and
    /// the left stick, and `look` on the mouse.
    void Update(const double delta_time)
    {
      _input.Refresh();
      _system.Update(_store, delta_time);
    }
  };

  TEST_F(SpectatorMovementTest, LeavesASpectatorWhereItIsWithoutInput)
  {
    Transform transform;
    transform.position = {1.0f, 2.0f, 3.0f};
    transform.rotation = {.pitch = 10.0f, .yaw = 20.0f, .roll = 30.0f};
    const Entity spectator = CreateSpectator(transform);

    Update(1.0);

    ExpectVector(TransformOf(spectator).position, 1.0f, 2.0f, 3.0f);
    EXPECT_EQ(TransformOf(spectator).rotation.pitch, 10.0f);
    EXPECT_EQ(TransformOf(spectator).rotation.yaw, 20.0f);
    EXPECT_EQ(TransformOf(spectator).rotation.roll, 30.0f);
  }

  TEST_F(SpectatorMovementTest, MovesForward)
  {
    const Entity spectator = CreateSpectator();
    _input.state.SetKeyDown(Key::W);

    Update(1.0);

    ExpectVector(TransformOf(spectator).position, 0.0f, 0.0f, -2.0f);
  }

  TEST_F(SpectatorMovementTest, MovesBack)
  {
    const Entity spectator = CreateSpectator();
    _input.state.SetKeyDown(Key::S);

    Update(1.0);

    ExpectVector(TransformOf(spectator).position, 0.0f, 0.0f, 2.0f);
  }

  TEST_F(SpectatorMovementTest, MovesToTheRight)
  {
    const Entity spectator = CreateSpectator();
    _input.state.SetKeyDown(Key::D);

    Update(1.0);

    ExpectVector(TransformOf(spectator).position, 2.0f, 0.0f, 0.0f);
  }

  TEST_F(SpectatorMovementTest, MovesToTheLeft)
  {
    const Entity spectator = CreateSpectator();
    _input.state.SetKeyDown(Key::A);

    Update(1.0);

    ExpectVector(TransformOf(spectator).position, -2.0f, 0.0f, 0.0f);
  }

  TEST_F(SpectatorMovementTest, MovesFromWhereItIs)
  {
    Transform transform;
    transform.position = {10.0f, 20.0f, 30.0f};
    const Entity spectator = CreateSpectator(transform);
    _input.state.SetKeyDown(Key::W);

    Update(1.0);

    ExpectVector(TransformOf(spectator).position, 10.0f, 20.0f, 28.0f);
  }

  TEST_F(SpectatorMovementTest, MovesByTheTimeThatPassed)
  {
    const Entity spectator = CreateSpectator();
    _input.state.SetKeyDown(Key::W);

    Update(0.25);
    ExpectVector(TransformOf(spectator).position, 0.0f, 0.0f, -0.5f);

    Update(0.5);
    ExpectVector(TransformOf(spectator).position, 0.0f, 0.0f, -1.5f);
  }

  TEST_F(SpectatorMovementTest, DoesNotMoveWhenNoTimePassed)
  {
    const Entity spectator = CreateSpectator();
    _input.state.SetKeyDown(Key::W);

    Update(0.0);

    ExpectVector(TransformOf(spectator).position, 0.0f, 0.0f, 0.0f);
  }

  TEST_F(SpectatorMovementTest, StaysWhenOppositeDirectionsArePressed)
  {
    const Entity spectator = CreateSpectator();
    _input.state.SetKeyDown(Key::W);
    _input.state.SetKeyDown(Key::S);
    _input.state.SetKeyDown(Key::A);
    _input.state.SetKeyDown(Key::D);

    Update(1.0);

    ExpectVector(TransformOf(spectator).position, 0.0f, 0.0f, 0.0f);
  }

  TEST_F(SpectatorMovementTest, MovesInBothDirectionsThatArePressed)
  {
    const Entity spectator = CreateSpectator();
    _input.state.SetKeyDown(Key::W);
    _input.state.SetKeyDown(Key::D);

    Update(1.0);

    ExpectVector(TransformOf(spectator).position, 2.0f, 0.0f, -2.0f);
  }

  TEST_F(SpectatorMovementTest, MovesWithTheLeftStick)
  {
    // pushed all the way aslant, which the dead zone of the default map
    // leaves as it is
    const Entity spectator = CreateSpectator();
    _input.state.SetLeftStick(0.6, -0.8);

    Update(1.0);

    ExpectVector(TransformOf(spectator).position, 1.2f, 0.0f, -1.6f);
  }

  TEST_F(SpectatorMovementTest, TurnsWithTheRightStickByTheTimeOfTheFrame)
  {
    // the default map turns 600 pixels a second with the stick pushed all
    // the way, and the view turns half a degree a pixel
    const Entity spectator = CreateSpectator();
    _input.state.SetRightStick(1.0, 0.0);
    _input.state.SetFrameTime(0.5);

    Update(0.5);

    ExpectVector(TransformOf(spectator).position, 0.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(TransformOf(spectator).rotation.yaw, -150.0f);
    EXPECT_FLOAT_EQ(TransformOf(spectator).rotation.pitch, 0.0f);
  }

  TEST_F(SpectatorMovementTest, MovesTheWayItLooksInTheWorld)
  {
    const Entity spectator = CreateSpectator(Turned(90.0f));
    _input.state.SetKeyDown(Key::W);

    Update(1.0);
    ExpectVector(TransformOf(spectator).position, -2.0f, 0.0f, 0.0f);

    _input.state.Reset();
    _input.state.SetKeyDown(Key::D);
    Update(1.0);
    ExpectVector(TransformOf(spectator).position, -2.0f, 0.0f, -2.0f);
  }

  TEST_F(SpectatorMovementTest, TurnsAgainstTheMotionOfTheMouse)
  {
    const Entity spectator = CreateSpectator();
    _input.state.SetAxisMotion(Axis::Mouse, 10.0, 4.0);

    Update(1.0);

    EXPECT_FLOAT_EQ(TransformOf(spectator).rotation.yaw, -5.0f);
    EXPECT_FLOAT_EQ(TransformOf(spectator).rotation.pitch, -2.0f);
    EXPECT_EQ(TransformOf(spectator).rotation.roll, 0.0f);
    ExpectVector(TransformOf(spectator).position, 0.0f, 0.0f, 0.0f);
  }

  TEST_F(SpectatorMovementTest, TurnsFromWhereItLooks)
  {
    Transform transform;
    transform.rotation = {.pitch = 10.0f, .yaw = 20.0f};
    const Entity spectator = CreateSpectator(transform);
    _input.state.SetAxisMotion(Axis::Mouse, -10.0, -4.0);

    Update(1.0);

    EXPECT_FLOAT_EQ(TransformOf(spectator).rotation.yaw, 25.0f);
    EXPECT_FLOAT_EQ(TransformOf(spectator).rotation.pitch, 12.0f);
  }

  TEST_F(SpectatorMovementTest, TurnsTheSameWhateverTimePassed)
  {
    const Entity spectator = CreateSpectator();
    _input.state.SetAxisMotion(Axis::Mouse, 10.0, 4.0);

    Update(0.001);

    EXPECT_FLOAT_EQ(TransformOf(spectator).rotation.yaw, -5.0f);
    EXPECT_FLOAT_EQ(TransformOf(spectator).rotation.pitch, -2.0f);
  }

  TEST_F(SpectatorMovementTest, DoesNotLookFurtherUpOrDownThan89Degrees)
  {
    const Entity spectator = CreateSpectator();

    _input.state.SetAxisMotion(Axis::Mouse, 0.0, -1000.0);
    Update(1.0);
    EXPECT_FLOAT_EQ(TransformOf(spectator).rotation.pitch, 89.0f);

    _input.state.SetAxisMotion(Axis::Mouse, 0.0, 1000.0);
    Update(1.0);
    EXPECT_FLOAT_EQ(TransformOf(spectator).rotation.pitch, -89.0f);
  }

  TEST_F(SpectatorMovementTest, TurnsAroundAsOftenAsTheMouseSays)
  {
    const Entity spectator = CreateSpectator();
    _input.state.SetAxisMotion(Axis::Mouse, 2000.0, 0.0);

    Update(1.0);

    EXPECT_FLOAT_EQ(TransformOf(spectator).rotation.yaw, -1000.0f);
  }

  TEST_F(SpectatorMovementTest, IgnoresTheLastMotionOfTheMouseOnceItStopped)
  {
    const Entity spectator = CreateSpectator();
    _input.state.SetAxisMotion(Axis::Mouse, 10.0, 4.0);
    Update(1.0);

    // what an input system does at the start of a frame
    _input.state.Reset();
    Update(1.0);

    EXPECT_FLOAT_EQ(TransformOf(spectator).rotation.yaw, -5.0f);
    EXPECT_FLOAT_EQ(TransformOf(spectator).rotation.pitch, -2.0f);
  }

  TEST_F(SpectatorMovementTest, MovesAndTurnsInTheSameFrame)
  {
    const Entity spectator = CreateSpectator();
    _input.state.SetKeyDown(Key::W);
    _input.state.SetAxisMotion(Axis::Mouse, 180.0, 0.0);

    Update(1.0);

    // it moves the way it was placed in the world, which the turn of this
    // frame has not changed yet
    ExpectVector(TransformOf(spectator).position, 0.0f, 0.0f, -2.0f);
    EXPECT_FLOAT_EQ(TransformOf(spectator).rotation.yaw, -90.0f);
  }

  TEST_F(SpectatorMovementTest, MovesEverySpectatorAtItsOwnSpeed)
  {
    const Entity slow = _store.CreateEntity("slow");
    _store.Set(slow, Transform{});
    _store.Set(slow, Spectator{.move_speed = 1.0f, .look_speed = 0.1f});
    const Entity fast = _store.CreateEntity("fast");
    _store.Set(fast, Transform{});
    _store.Set(fast, Spectator{.move_speed = 10.0f, .look_speed = 1.0f});
    _input.state.SetKeyDown(Key::W);
    _input.state.SetAxisMotion(Axis::Mouse, 10.0, 0.0);

    Update(1.0);

    ExpectVector(TransformOf(slow).position, 0.0f, 0.0f, -1.0f);
    ExpectVector(TransformOf(fast).position, 0.0f, 0.0f, -10.0f);
    EXPECT_FLOAT_EQ(TransformOf(slow).rotation.yaw, -1.0f);
    EXPECT_FLOAT_EQ(TransformOf(fast).rotation.yaw, -10.0f);
  }

  TEST_F(SpectatorMovementTest, UsesTheSpeedsASpectatorHasByDefault)
  {
    const Entity spectator = _store.CreateEntity("");
    _store.Set(spectator, Transform{});
    _store.Set(spectator, Spectator{});
    _input.state.SetKeyDown(Key::W);
    _input.state.SetAxisMotion(Axis::Mouse, 10.0, 0.0);

    Update(1.0);

    ExpectVector(TransformOf(spectator).position, 0.0f, 0.0f, -2.5f);
    EXPECT_FLOAT_EQ(TransformOf(spectator).rotation.yaw, -1.0f);
  }

  TEST_F(SpectatorMovementTest, LeavesWhatIsNoSpectatorAlone)
  {
    const Entity rock = _store.CreateEntity("rock");
    _store.Set(rock, Transform{});
    _input.state.SetKeyDown(Key::W);
    _input.state.SetAxisMotion(Axis::Mouse, 10.0, 4.0);

    Update(1.0);

    ExpectVector(TransformOf(rock).position, 0.0f, 0.0f, 0.0f);
    EXPECT_EQ(TransformOf(rock).rotation.yaw, 0.0f);
  }

  TEST_F(SpectatorMovementTest, LeavesTheSpectatorAsItWasSet)
  {
    const Entity spectator = CreateSpectator();
    _input.state.SetKeyDown(Key::W);

    Update(1.0);

    EXPECT_EQ(_store.Get<Spectator>(spectator)->move_speed, 2.0f);
    EXPECT_EQ(_store.Get<Spectator>(spectator)->look_speed, 0.5f);
  }

  TEST_F(SpectatorMovementTest, LeavesPlacingInTheWorldToAnotherSystem)
  {
    const Entity spectator = CreateSpectator();
    _input.state.SetKeyDown(Key::W);

    Update(1.0);

    EXPECT_EQ(TransformOf(spectator).world_coordinates, glm::mat4(1.0f));
  }

  TEST_F(SpectatorMovementTest, DoesNothingInAWorldWithoutSpectators)
  {
    _input.state.SetKeyDown(Key::W);

    Update(1.0);

    EXPECT_EQ(_store.EntityCount(), 0u);
  }
}
