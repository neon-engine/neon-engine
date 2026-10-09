#include "player-movement.hpp"

#include <cmath>
#include <memory>

#include <gtest/gtest.h>

#include <neon/common/transform.hpp>
#include <neon/testing/fake-entity-store.hpp>
#include <neon/testing/mock-input-system.hpp>
#include <neon/testing/recording-logger.hpp>
#include <neon/world-system/ecs/components/camera.hpp>
#include <neon/world-system/ecs/components/character-body.hpp>
#include <neon/world-system/ecs/components/collider.hpp>
#include <neon/world-system/ecs/components/player.hpp>

namespace
{
  using neon::Axis;
  using neon::Camera;
  using neon::CharacterBody;
  using neon::Collider;
  using neon::ControllerButton;
  using neon::Entity;
  using neon::Key;
  using neon::Player;
  using neon::PlayerMovement;
  using neon::ShapeKind;
  using neon::Transform;
  using neon::testing::FakeEntityStore;
  using neon::testing::FakeInputContext;
  using neon::testing::RecordingLogger;
  using ::testing::NiceMock;

  constexpr float tolerance = 1e-4f;

  void ExpectVector(const glm::vec3 &actual, const float x, const float y, const float z)
  {
    EXPECT_NEAR(actual.x, x, tolerance);
    EXPECT_NEAR(actual.y, y, tolerance);
    EXPECT_NEAR(actual.z, z, tolerance);
  }

  class PlayerMovementTest : public ::testing::Test
  {
  protected:
    FakeEntityStore _store;
    NiceMock<FakeInputContext> _input{std::make_shared<RecordingLogger>()};
    PlayerMovement _system{&_input};

    void SetUp() override
    {
      _store.Initialize();
      _store.Register<Transform>("Transform");
      _store.Register<Camera>("Camera");
      _store.Register<CharacterBody>("CharacterBody");
      _store.Register<Collider>("Collider");
      _store.Register<Player>("Player");
      _system.Initialize(_store);
      _input.TakeTheDevicesAsTheyAre();
    }

    /// A player that walks 2 meters a second, runs 3, jumps with 5, steers
    /// half as much in the air, and turns a hundredth of a radian for every
    /// pixel, standing on the ground in a capsule of 1.8, with its camera
    /// below it.
    Entity CreatePlayer(const float yaw = 0.0f, const bool on_floor = true)
    {
      const Entity entity = _store.CreateEntity("player");
      Transform transform;
      transform.rotation.yaw = yaw;
      _store.Set(entity, transform);
      _store.Set(entity, Player{
        .walk_speed = 2.0f,
        .run_speed = 3.0f,
        .jump_speed = 5.0f,
        .air_control = 0.5f,
        .look_speed = 0.01f
      });
      _store.Set(entity, CharacterBody{.on_floor = on_floor});
      _store.Set(entity, Collider{.shape = ShapeKind::Capsule, .radius = 0.4f, .height = 1.8f});

      const Entity camera = _store.CreateEntity("camera", entity);
      _store.Set(camera, Transform{});
      _store.Set(camera, Camera{});
      return entity;
    }

    Entity CameraOf(const Entity player)
    {
      return _store.GetChildren(player).front();
    }

    const Transform &TransformOf(const Entity entity)
    {
      return *_store.Get<Transform>(entity);
    }

    const CharacterBody &BodyOf(const Entity entity)
    {
      return *_store.Get<CharacterBody>(entity);
    }

    /// What the physics did with the body in the steps between two frames:
    /// where it put it, and whether it stands on the ground.
    void PhysicsPlaced(const Entity player, const float height, const bool on_floor)
    {
      _store.Get<Transform>(player)->position.y = height;
      _store.Get<CharacterBody>(player)->on_floor = on_floor;
    }

    /// A frame: the actions are worked out from the keys, and the system
    /// runs. The map is the engine's default, with `move` on W, A, S, D
    /// and the left stick, `look` on the mouse, `jump` on space, and `run`
    /// on the left shift and the click of the left stick, but without a
    /// dead zone, so that a stick reads as far as it is pushed.
    void Update()
    {
      _input.Refresh();
      _system.Update(_store, 1.0 / 60.0);
    }
  };

  // walking

  TEST_F(PlayerMovementTest, StandsStillWithoutInput)
  {
    const Entity player = CreatePlayer(30.0f);

    Update();

    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, 0.0f);
    ExpectVector(BodyOf(player).fall_velocity, 0.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(TransformOf(player).rotation.yaw, 30.0f);
    EXPECT_FLOAT_EQ(TransformOf(CameraOf(player)).rotation.pitch, 0.0f);
  }

  TEST_F(PlayerMovementTest, WalksForwardAtTheWalkingSpeed)
  {
    const Entity player = CreatePlayer();
    _input.state.SetKeyDown(Key::W);

    Update();

    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, -2.0f);
  }

  TEST_F(PlayerMovementTest, WalksBackAndToTheSides)
  {
    const Entity player = CreatePlayer();

    _input.state.SetKeyDown(Key::S);
    Update();
    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, 2.0f);

    _input.state.Reset();
    _input.state.SetKeyDown(Key::D);
    Update();
    ExpectVector(BodyOf(player).velocity, 2.0f, 0.0f, 0.0f);

    _input.state.Reset();
    _input.state.SetKeyDown(Key::A);
    Update();
    ExpectVector(BodyOf(player).velocity, -2.0f, 0.0f, 0.0f);
  }

  TEST_F(PlayerMovementTest, WalksTheWayItFaces)
  {
    const Entity player = CreatePlayer(90.0f);

    _input.state.SetKeyDown(Key::W);
    Update();
    ExpectVector(BodyOf(player).velocity, -2.0f, 0.0f, 0.0f);

    _input.state.Reset();
    _input.state.SetKeyDown(Key::D);
    Update();
    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, -2.0f);
  }

  TEST_F(PlayerMovementTest, WalksNoFasterAlongADiagonal)
  {
    const Entity player = CreatePlayer();
    _input.state.SetKeyDown(Key::W);
    _input.state.SetKeyDown(Key::D);

    Update();

    EXPECT_NEAR(length(BodyOf(player).velocity), 2.0f, tolerance);
    EXPECT_NEAR(BodyOf(player).velocity.x, -BodyOf(player).velocity.z, tolerance);
  }

  TEST_F(PlayerMovementTest, WalksAsFarAsTheStickIsPushed)
  {
    // half the way is half the walking speed
    const Entity player = CreatePlayer();
    _input.state.SetLeftStick(0.0, -0.5);

    Update();

    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, -1.0f);
  }

  TEST_F(PlayerMovementTest, StopsWhenTheKeysAreReleased)
  {
    const Entity player = CreatePlayer();
    _input.state.SetKeyDown(Key::W);
    Update();

    _input.state.Reset();
    Update();

    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, 0.0f);
  }

  TEST_F(PlayerMovementTest, RunsWhileRunIsDown)
  {
    const Entity player = CreatePlayer();
    _input.state.SetKeyDown(Key::W);
    _input.state.SetKeyDown(Key::LeftShift);

    Update();

    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, -3.0f);
  }

  TEST_F(PlayerMovementTest, RunsWhileTheLeftStickIsPressedIn)
  {
    const Entity player = CreatePlayer();
    _input.state.SetLeftStick(0.0, -1.0);
    _input.state.SetControllerButtonDown(ControllerButton::LeftStick);

    Update();

    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, -3.0f);
  }

  TEST_F(PlayerMovementTest, WalksAgainOnceRunIsReleased)
  {
    const Entity player = CreatePlayer();
    _input.state.SetKeyDown(Key::W);
    _input.state.SetKeyDown(Key::LeftShift);
    Update();

    _input.state.Reset();
    _input.state.SetKeyDown(Key::W);
    Update();

    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, -2.0f);
  }

  TEST_F(PlayerMovementTest, LeavesThePhysicsToMoveTheBody)
  {
    const Entity player = CreatePlayer();
    _input.state.SetKeyDown(Key::W);

    Update();

    ExpectVector(TransformOf(player).position, 0.0f, 0.0f, 0.0f);
  }

  // looking

  TEST_F(PlayerMovementTest, TurnsTheBodyAgainstTheMotionOfTheMouse)
  {
    // a hundredth of a radian a pixel: 10 pixels are 0.1 radians
    const Entity player = CreatePlayer();
    _input.state.SetAxisMotion(Axis::Mouse, 10.0, 0.0);

    Update();

    EXPECT_NEAR(TransformOf(player).rotation.yaw, -glm::degrees(0.1f), tolerance);
    EXPECT_FLOAT_EQ(TransformOf(player).rotation.pitch, 0.0f);
    EXPECT_FLOAT_EQ(TransformOf(player).rotation.roll, 0.0f);
  }

  TEST_F(PlayerMovementTest, TurnsFromWhereItFaces)
  {
    const Entity player = CreatePlayer(20.0f);
    _input.state.SetAxisMotion(Axis::Mouse, -10.0, 0.0);

    Update();

    EXPECT_NEAR(TransformOf(player).rotation.yaw, 20.0f + glm::degrees(0.1f), tolerance);
  }

  TEST_F(PlayerMovementTest, PitchesTheCameraAndNotTheBody)
  {
    // the mouse moved down the screen, so the view goes down
    const Entity player = CreatePlayer();
    _input.state.SetAxisMotion(Axis::Mouse, 0.0, 4.0);

    Update();

    EXPECT_NEAR(TransformOf(CameraOf(player)).rotation.pitch, -glm::degrees(0.04f), tolerance);
    EXPECT_FLOAT_EQ(TransformOf(CameraOf(player)).rotation.yaw, 0.0f);
    EXPECT_FLOAT_EQ(TransformOf(player).rotation.pitch, 0.0f);
  }

  TEST_F(PlayerMovementTest, LooksNoFurtherUpOrDownThanMaxPitch)
  {
    const Entity player = CreatePlayer();
    _store.Get<Player>(player)->max_pitch = 60.0f;

    _input.state.SetAxisMotion(Axis::Mouse, 0.0, -1000.0);
    Update();
    EXPECT_FLOAT_EQ(TransformOf(CameraOf(player)).rotation.pitch, 60.0f);

    _input.state.SetAxisMotion(Axis::Mouse, 0.0, 1000.0);
    Update();
    EXPECT_FLOAT_EQ(TransformOf(CameraOf(player)).rotation.pitch, -60.0f);
  }

  TEST_F(PlayerMovementTest, TurnsAndWalksInTheSameFrame)
  {
    // the quarter turn to the left comes first, so the step goes to the
    // left as well
    const Entity player = CreatePlayer();
    _input.state.SetKeyDown(Key::W);
    _input.state.SetAxisMotion(Axis::Mouse, -glm::half_pi<double>() * 100.0, 0.0);

    Update();

    EXPECT_NEAR(TransformOf(player).rotation.yaw, 90.0f, tolerance);
    ExpectVector(BodyOf(player).velocity, -2.0f, 0.0f, 0.0f);
  }

  TEST_F(PlayerMovementTest, KeepsTheLookOnceTheMouseStopped)
  {
    const Entity player = CreatePlayer();
    _input.state.SetAxisMotion(Axis::Mouse, 10.0, 4.0);
    Update();

    _input.state.Reset();
    Update();

    EXPECT_NEAR(TransformOf(player).rotation.yaw, -glm::degrees(0.1f), tolerance);
    EXPECT_NEAR(TransformOf(CameraOf(player)).rotation.pitch, -glm::degrees(0.04f), tolerance);
  }

  // the camera

  TEST_F(PlayerMovementTest, LiftsTheCameraToTheEyesAboveTheFeet)
  {
    // the capsule of 1.8 stands 0.9 below the origin, and the eyes are 1.6
    // above the feet
    const Entity player = CreatePlayer();

    Update();

    ExpectVector(TransformOf(CameraOf(player)).position, 0.0f, 0.7f, 0.0f);
  }

  TEST_F(PlayerMovementTest, MeasuresTheEyesFromTheOriginWithoutACollider)
  {
    const Entity player = CreatePlayer();
    _store.RemoveComponent(player, _store.IdOf<Collider>());

    Update();

    ExpectVector(TransformOf(CameraOf(player)).position, 0.0f, 1.6f, 0.0f);
  }

  TEST_F(PlayerMovementTest, MeasuresTheEyesInTheUnitsOfABodyThatIsScaled)
  {
    // the body is twice as large, so 1.6 meters are 0.8 of its units, and
    // the capsule, which is scaled with it, still stands 0.9 of them below
    const Entity player = CreatePlayer();
    _store.Get<Transform>(player)->scale = glm::vec3{2.0f};

    Update();

    ExpectVector(TransformOf(CameraOf(player)).position, 0.0f, -0.1f, 0.0f);
  }

  TEST_F(PlayerMovementTest, LeavesACameraThatIsNoChildOfAPlayerAlone)
  {
    const Entity camera = _store.CreateEntity("camera");
    Transform transform;
    transform.position = {1.0f, 2.0f, 3.0f};
    _store.Set(camera, transform);
    _store.Set(camera, Camera{});
    _input.state.SetAxisMotion(Axis::Mouse, 0.0, 4.0);

    Update();

    ExpectVector(TransformOf(camera).position, 1.0f, 2.0f, 3.0f);
    EXPECT_FLOAT_EQ(TransformOf(camera).rotation.pitch, 0.0f);
  }

  // jumping

  TEST_F(PlayerMovementTest, JumpsFromTheGround)
  {
    const Entity player = CreatePlayer();
    _input.state.SetKeyDown(Key::Space);

    Update();

    ExpectVector(BodyOf(player).fall_velocity, 0.0f, 5.0f, 0.0f);
  }

  TEST_F(PlayerMovementTest, DoesNotJumpInTheAir)
  {
    const Entity player = CreatePlayer(0.0f, false);
    _store.Get<CharacterBody>(player)->fall_velocity = {0.0f, -3.0f, 0.0f};
    _input.state.SetKeyDown(Key::Space);

    Update();

    ExpectVector(BodyOf(player).fall_velocity, 0.0f, -3.0f, 0.0f);
  }

  TEST_F(PlayerMovementTest, JumpsOnceForOnePress)
  {
    const Entity player = CreatePlayer();
    _input.state.SetKeyDown(Key::Space);
    Update();

    // the key is still held in the next frame, and the physics has not
    // taken the body off the ground yet
    _store.Get<CharacterBody>(player)->fall_velocity = {0.0f, 4.0f, 0.0f};
    Update();

    ExpectVector(BodyOf(player).fall_velocity, 0.0f, 4.0f, 0.0f);
  }

  TEST_F(PlayerMovementTest, JumpsWhileWalking)
  {
    const Entity player = CreatePlayer();
    _input.state.SetKeyDown(Key::W);
    _input.state.SetKeyDown(Key::Space);

    Update();

    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, -2.0f);
    ExpectVector(BodyOf(player).fall_velocity, 0.0f, 5.0f, 0.0f);
  }

  // in the air

  TEST_F(PlayerMovementTest, KeepsTheVelocityItLeftTheGroundWithInTheAir)
  {
    const Entity player = CreatePlayer();
    _store.Get<Player>(player)->air_control = 0.0f;
    _input.state.SetKeyDown(Key::W);
    Update();

    // the key is released over the gap, and the body goes on
    PhysicsPlaced(player, 0.5f, false);
    _input.state.Reset();
    Update();
    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, -2.0f);

    _input.state.SetKeyDown(Key::S);
    Update();
    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, -2.0f);
  }

  TEST_F(PlayerMovementTest, SteersInTheAirByAirControl)
  {
    // half the control: the keys pull the take-off velocity halfway to
    // what they ask for
    const Entity player = CreatePlayer();
    _input.state.SetKeyDown(Key::W);
    Update();
    PhysicsPlaced(player, 0.5f, false);

    _input.state.Reset();
    Update();
    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, -1.0f);

    _input.state.SetKeyDown(Key::S);
    Update();
    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, 0.0f);

    _input.state.Reset();
    _input.state.SetKeyDown(Key::D);
    Update();
    ExpectVector(BodyOf(player).velocity, 1.0f, 0.0f, -1.0f);
  }

  TEST_F(PlayerMovementTest, SteersNoFurtherFrameAfterFrameInTheAir)
  {
    // the pull is from the take-off velocity every frame, not from the
    // frame before, so a long jump with the keys released does not slow
    // to a stop
    const Entity player = CreatePlayer();
    _input.state.SetKeyDown(Key::W);
    Update();
    PhysicsPlaced(player, 0.5f, false);
    _input.state.Reset();

    for (int frame = 0; frame < 30; frame++) { Update(); }

    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, -1.0f);
  }

  TEST_F(PlayerMovementTest, SteersAsOnTheGroundWithFullAirControl)
  {
    const Entity player = CreatePlayer();
    _store.Get<Player>(player)->air_control = 1.0f;
    _input.state.SetKeyDown(Key::W);
    Update();
    PhysicsPlaced(player, 0.5f, false);

    _input.state.Reset();
    _input.state.SetKeyDown(Key::D);
    Update();

    ExpectVector(BodyOf(player).velocity, 2.0f, 0.0f, 0.0f);
  }

  TEST_F(PlayerMovementTest, WalksAsTheKeysSayOnceItLands)
  {
    const Entity player = CreatePlayer();
    _input.state.SetKeyDown(Key::W);
    Update();
    PhysicsPlaced(player, 0.5f, false);
    _input.state.Reset();
    Update();

    PhysicsPlaced(player, 0.0f, true);
    Update();
    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, 0.0f);

    _input.state.SetKeyDown(Key::A);
    Update();
    ExpectVector(BodyOf(player).velocity, -2.0f, 0.0f, 0.0f);
  }

  TEST_F(PlayerMovementTest, KeepsRunningInTheAirWhenRunWasDownAtTakeOff)
  {
    const Entity player = CreatePlayer();
    _store.Get<Player>(player)->air_control = 0.0f;
    _input.state.SetKeyDown(Key::W);
    _input.state.SetKeyDown(Key::LeftShift);
    Update();
    PhysicsPlaced(player, 0.5f, false);

    _input.state.Reset();
    Update();

    ExpectVector(BodyOf(player).velocity, 0.0f, 0.0f, -3.0f);
  }

  // the glide over steps

  TEST_F(PlayerMovementTest, GlidesTheEyesUpAStep)
  {
    // the physics lifted the body a quarter of a meter between two frames
    // on the floor: the eyes stay where they were in that frame, and catch
    // up at step_smoothing, a tenth of the way left every sixtieth
    const Entity player = CreatePlayer();
    _store.Get<Player>(player)->step_smoothing = 10.0f;
    Update();

    PhysicsPlaced(player, 0.25f, true);
    Update();
    ExpectVector(TransformOf(CameraOf(player)).position, 0.0f, 0.45f, 0.0f);

    Update();
    const float left = 0.25f * std::exp(-10.0f / 60.0f);
    ExpectVector(TransformOf(CameraOf(player)).position, 0.0f, 0.7f - left, 0.0f);

    for (int frame = 0; frame < 120; frame++) { Update(); }
    ExpectVector(TransformOf(CameraOf(player)).position, 0.0f, 0.7f, 0.0f);
  }

  TEST_F(PlayerMovementTest, GlidesTheEyesDownAStepToo)
  {
    const Entity player = CreatePlayer();
    _store.Get<Player>(player)->step_smoothing = 10.0f;
    PhysicsPlaced(player, 0.25f, true);
    Update();

    PhysicsPlaced(player, 0.0f, true);
    Update();

    ExpectVector(TransformOf(CameraOf(player)).position, 0.0f, 0.95f, 0.0f);
  }

  TEST_F(PlayerMovementTest, LiftsTheEyesWithTheBodyWithoutSmoothing)
  {
    const Entity player = CreatePlayer();
    _store.Get<Player>(player)->step_smoothing = 0.0f;
    Update();

    PhysicsPlaced(player, 0.25f, true);
    Update();

    ExpectVector(TransformOf(CameraOf(player)).position, 0.0f, 0.7f, 0.0f);
  }

  TEST_F(PlayerMovementTest, DoesNotGlideThroughAJumpOrOnLanding)
  {
    const Entity player = CreatePlayer();
    _store.Get<Player>(player)->step_smoothing = 10.0f;
    Update();

    // up in the air, and down again onto a step that was not there before
    PhysicsPlaced(player, 1.0f, false);
    Update();
    ExpectVector(TransformOf(CameraOf(player)).position, 0.0f, 0.7f, 0.0f);

    PhysicsPlaced(player, 0.25f, true);
    Update();
    ExpectVector(TransformOf(CameraOf(player)).position, 0.0f, 0.7f, 0.0f);
  }

  TEST_F(PlayerMovementTest, GlidesInTheUnitsOfABodyThatIsScaled)
  {
    // the body is twice as large: a step of 0.5 in the world is 0.25 of its
    // units, below the eyes at -0.1
    const Entity player = CreatePlayer();
    _store.Get<Player>(player)->step_smoothing = 10.0f;
    _store.Get<Transform>(player)->scale = glm::vec3{2.0f};
    Update();

    PhysicsPlaced(player, 0.5f, true);
    Update();

    ExpectVector(TransformOf(CameraOf(player)).position, 0.0f, -0.35f, 0.0f);
  }

  // the offset of the camera

  TEST_F(PlayerMovementTest, MovesTheCameraByTheOffset)
  {
    // to the right, up, and back from the eyes, in the frame of the body
    const Entity player = CreatePlayer();
    _store.Get<Player>(player)->camera_offset = {0.2f, 0.1f, 1.5f};

    Update();

    ExpectVector(TransformOf(CameraOf(player)).position, 0.2f, 0.8f, 1.5f);
  }

  TEST_F(PlayerMovementTest, MeasuresTheOffsetInTheUnitsOfABodyThatIsScaled)
  {
    const Entity player = CreatePlayer();
    _store.Get<Player>(player)->camera_offset = {1.0f, 0.0f, 2.0f};
    _store.Get<Transform>(player)->scale = glm::vec3{2.0f};

    Update();

    ExpectVector(TransformOf(CameraOf(player)).position, 0.5f, -0.1f, 1.0f);
  }

  // what is left alone

  TEST_F(PlayerMovementTest, LeavesACharacterThatIsNoPlayerAlone)
  {
    const Entity walker = _store.CreateEntity("walker");
    _store.Set(walker, Transform{});
    _store.Set(walker, CharacterBody{.velocity = {0.5f, 0.0f, -1.5f}, .on_floor = true});
    _input.state.SetKeyDown(Key::W);
    _input.state.SetKeyDown(Key::Space);
    _input.state.SetAxisMotion(Axis::Mouse, 10.0, 0.0);

    Update();

    ExpectVector(BodyOf(walker).velocity, 0.5f, 0.0f, -1.5f);
    ExpectVector(BodyOf(walker).fall_velocity, 0.0f, 0.0f, 0.0f);
    EXPECT_FLOAT_EQ(TransformOf(walker).rotation.yaw, 0.0f);
  }

  TEST_F(PlayerMovementTest, DrivesEveryPlayerByTheSameInput)
  {
    const Entity first = CreatePlayer();
    const Entity second = _store.CreateEntity("second");
    _store.Set(second, Transform{});
    _store.Set(second, Player{.walk_speed = 10.0f});
    _store.Set(second, CharacterBody{.on_floor = true});
    _input.state.SetKeyDown(Key::W);

    Update();

    ExpectVector(BodyOf(first).velocity, 0.0f, 0.0f, -2.0f);
    ExpectVector(BodyOf(second).velocity, 0.0f, 0.0f, -10.0f);
  }

  TEST_F(PlayerMovementTest, LeavesThePlayerAsItWasSet)
  {
    const Entity player = CreatePlayer();
    _input.state.SetKeyDown(Key::W);
    _input.state.SetAxisMotion(Axis::Mouse, 10.0, 4.0);

    Update();

    EXPECT_EQ(_store.Get<Player>(player)->walk_speed, 2.0f);
    EXPECT_EQ(_store.Get<Player>(player)->look_speed, 0.01f);
  }

  TEST_F(PlayerMovementTest, DoesNothingInAWorldWithoutPlayers)
  {
    _input.state.SetKeyDown(Key::W);

    Update();

    EXPECT_EQ(_store.EntityCount(), 0u);
  }
} // namespace
