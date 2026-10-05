#include "jolt-physics-system.hpp"

#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <neon/common/rotation.hpp>
#include <neon/testing/recording-logger.hpp>

// The physics is tested through its interface, with worlds that are small
// enough to know what has to come out.

namespace
{
  using neon::BodyId;
  using neon::BodyInfo;
  using neon::BodyKind;
  using neon::BodyState;
  using neon::CharacterId;
  using neon::CharacterInfo;
  using neon::CharacterState;
  using neon::Entity;
  using neon::JointId;
  using neon::JointInfo;
  using neon::JointKind;
  using neon::JointState;
  using neon::Jolt_PhysicsSystem;
  using neon::No_Body;
  using neon::No_Character;
  using neon::No_Joint;
  using neon::OverlapHit;
  using neon::PhysicsEvent;
  using neon::PhysicsEventKind;
  using neon::QueryFilter;
  using neon::Ray;
  using neon::RayHit;
  using neon::Rotation;
  using neon::ShapeCastHit;
  using neon::ShapeInfo;
  using neon::ShapeKind;
  using neon::testing::LogLevel;
  using neon::testing::RecordingLogger;
  using ::testing::HasSubstr;

  constexpr double step = 1.0 / 60.0;

  // what entities are called in these tests
  constexpr Entity floor_entity = 1;
  constexpr Entity crate_entity = 2;
  constexpr Entity ball_entity = 3;
  constexpr Entity wall_entity = 4;
  constexpr Entity trigger_entity = 5;
  constexpr Entity player_entity = 6;
  constexpr Entity other_entity = 7;

  ShapeInfo Box(const glm::vec3 &size)
  {
    ShapeInfo shape;
    shape.kind = ShapeKind::Box;
    shape.size = size;
    return shape;
  }

  ShapeInfo Sphere(const float radius)
  {
    ShapeInfo shape;
    shape.kind = ShapeKind::Sphere;
    shape.radius = radius;
    return shape;
  }

  ShapeInfo Capsule(const float radius, const float height)
  {
    ShapeInfo shape;
    shape.kind = ShapeKind::Capsule;
    shape.radius = radius;
    shape.height = height;
    return shape;
  }

  /// The corners of a cube with sides of 1, and the triangles of its sides.
  ShapeInfo CubeOf(const ShapeKind kind)
  {
    ShapeInfo shape;
    shape.kind = kind;
    shape.points = {
      {-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f}, {-0.5f, 0.5f, -0.5f},
      {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, {0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}
    };
    if (kind == ShapeKind::Mesh)
    {
      shape.triangles = {
        0, 2, 1, 0, 3, 2, // back
        4, 5, 6, 4, 6, 7, // front
        0, 4, 7, 0, 7, 3, // left
        1, 2, 6, 1, 6, 5, // right
        3, 7, 6, 3, 6, 2, // top
        0, 1, 5, 0, 5, 4 // bottom
      };
    }
    return shape;
  }

  class JoltPhysicsSystemTest : public ::testing::Test
  {
  protected:
    std::shared_ptr<RecordingLogger> _logger = std::make_shared<RecordingLogger>();
    Jolt_PhysicsSystem _physics{SettingsConfig{}, _logger};
    std::string _error;

    /// Every event since the test began.
    std::vector<PhysicsEvent> _events;

    void SetUp() override
    {
      _physics.Initialize();
    }

    void TearDown() override
    {
      _physics.CleanUp();
    }

    BodyId Create(const BodyInfo &info)
    {
      BodyId body = No_Body;
      EXPECT_TRUE(_physics.CreateBody(info, body, _error)) << _error;
      return body;
    }

    BodyId CreateBody(
      const Entity entity,
      const BodyKind kind,
      const ShapeInfo &shape,
      const glm::vec3 &position,
      const std::function<void(BodyInfo &)> &change = {})
    {
      BodyInfo info;
      info.entity = entity;
      info.kind = kind;
      info.shapes = {shape};
      info.position = position;
      if (change) { change(info); }
      return Create(info);
    }

    /// A floor whose top lies at height 0.
    BodyId CreateFloor()
    {
      return CreateBody(floor_entity, BodyKind::Static, Box({100.0f, 1.0f, 100.0f}), {0.0f, -0.5f, 0.0f});
    }

    BodyId CreateTrigger(const ShapeInfo &shape, const glm::vec3 &position, const std::uint32_t mask = 1)
    {
      return CreateBody(trigger_entity, BodyKind::Kinematic, shape, position, [mask](BodyInfo &info)
      {
        info.trigger = true;
        info.mask = mask;
      });
    }

    CharacterId CreateCharacter(const glm::vec3 &position, const std::function<void(CharacterInfo &)> &change = {})
    {
      CharacterInfo info;
      info.entity = player_entity;
      info.shapes = {Capsule(0.5f, 2.0f)};
      info.position = position;
      if (change) { change(info); }

      CharacterId character = No_Character;
      EXPECT_TRUE(_physics.CreateCharacter(info, character, _error)) << _error;
      return character;
    }

    void Run(const int steps)
    {
      for (int i = 0; i < steps; i++)
      {
        _physics.Step(step);
        const auto &events = _physics.GetStepEvents();
        _events.insert(_events.end(), events.begin(), events.end());
      }
    }

    /// Moves a character as the engine does, with falling added.
    CharacterState Walk(const CharacterId character, const glm::vec3 &velocity, const int steps)
    {
      CharacterState state;
      for (int i = 0; i < steps; i++)
      {
        const bool falls = !state.on_floor;
        _fall = falls ? _fall - 9.81f * static_cast<float>(step) : -9.81f * static_cast<float>(step);

        EXPECT_TRUE(_physics.MoveCharacter(character, velocity + glm::vec3(0.0f, _fall, 0.0f), step, state));
        Run(1);
      }
      return state;
    }

    JointId Join(const JointInfo &info)
    {
      JointId joint = No_Joint;
      EXPECT_TRUE(_physics.CreateJoint(info, joint, _error)) << _error;
      return joint;
    }

    /// A door of 1.6 by 2 that hangs on a post at the origin, with its
    /// hinge a little off its left edge, so that the door swings clear of
    /// the post. Gravity does not pull it.
    BodyId CreateDoor(const std::vector<float> &limits = {}, const std::function<void(JointInfo &)> &change = {})
    {
      const auto frame = CreateBody(wall_entity, BodyKind::Static, Box({0.2f, 2.0f, 0.2f}), {0.0f, 1.0f, 0.0f});
      const auto door = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.6f, 2.0f, 0.1f}), {1.05f, 1.0f, 0.0f},
                                   [](BodyInfo &info) { info.gravity_scale = 0.0f; });

      JointInfo hinge;
      hinge.kind = JointKind::Hinge;
      hinge.body = door;
      hinge.other = frame;
      hinge.anchor = {0.2f, 1.0f, 0.0f};
      hinge.axis = {0.0f, 1.0f, 0.0f};
      if (limits.size() == 2)
      {
        hinge.has_limits = true;
        hinge.limit_min = limits[0];
        hinge.limit_max = limits[1];
      }
      if (change) { change(hinge); }
      _door_joint = Join(hinge);
      return door;
    }

    /// A door that hangs at its frame, with its left edge inside the
    /// frame, so that the two overlap. The hinge sits in the overlap.
    BodyId CreateDoorAtItsFrame(const bool collide_with_other)
    {
      const auto frame = CreateBody(wall_entity, BodyKind::Static, Box({0.2f, 2.0f, 0.2f}), {0.0f, 1.0f, 0.0f});
      const auto door = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.6f, 2.0f, 0.1f}), {0.85f, 1.0f, 0.0f},
                                   [](BodyInfo &info) { info.gravity_scale = 0.0f; });

      _door_joint = Join(JointInfo{
        .kind = JointKind::Hinge,
        .body = door,
        .other = frame,
        .anchor = {0.05f, 1.0f, 0.0f},
        .axis = {0.0f, 1.0f, 0.0f},
        .collide_with_other = collide_with_other
      });
      return door;
    }

    JointState StateOfDoor()
    {
      JointState state;
      EXPECT_TRUE(_physics.GetJointState(_door_joint, state));
      return state;
    }

    JointId _door_joint = No_Joint;

    /// How far a door turned from where it hung, in degrees.
    float AngleOf(const BodyId door)
    {
      const auto across = StateOf(door).rotation * glm::vec3(1.0f, 0.0f, 0.0f);
      return glm::degrees(std::acos(std::clamp(across.x, -1.0f, 1.0f)));
    }

    BodyState StateOf(const BodyId body)
    {
      BodyState state;
      EXPECT_TRUE(_physics.GetBodyState(body, state));
      return state;
    }

    std::size_t Count(const PhysicsEventKind kind, const Entity first, const Entity second) const
    {
      return static_cast<std::size_t>(std::ranges::count_if(_events, [&](const PhysicsEvent &event)
      {
        return event.kind == kind
               && ((event.first == first && event.second == second)
                   || (event.first == second && event.second == first));
      }));
    }

    float _fall = 0.0f;
  };

  // locked axes

  TEST_F(JoltPhysicsSystemTest, DoesNotTurnABodyWhoseRotationIsLockedWhenItIsHitBesideItsMiddle)
  {
    CreateFloor();
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 2.0f, 1.0f}), {0.0f, 1.0f, 0.0f},
                                  [](BodyInfo &info)
                                  {
                                    info.locked_rotation = neon::Axis_X | neon::Axis_Y | neon::Axis_Z;
                                  });
    const auto other = CreateBody(other_entity, BodyKind::Dynamic, Box({1.0f, 2.0f, 1.0f}), {3.0f, 1.0f, 0.0f});
    Run(30);

    // a kick at the top, which tips a crate over
    _physics.AddImpulseAt(crate, {30.0f, 0.0f, 0.0f}, {0.0f, 1.9f, 0.0f});
    _physics.AddImpulseAt(other, {30.0f, 0.0f, 0.0f}, {3.0f, 1.9f, 0.0f});
    Run(60);

    const auto locked = StateOf(crate);
    const auto free = StateOf(other);

    // the locked one slid, upright, and the other fell over
    EXPECT_GT(locked.position.x, 0.5f);
    EXPECT_NEAR(std::abs(locked.rotation.w), 1.0f, 1e-4f);
    EXPECT_EQ(locked.angular_velocity, glm::vec3(0.0f));
    EXPECT_LT(std::abs(free.rotation.w), 0.99f);
  }

  TEST_F(JoltPhysicsSystemTest, DoesNotMoveABodyAlongAnAxisItIsLockedOn)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f},
                                  [](BodyInfo &info)
                                  {
                                    info.gravity_scale = 0.0f;
                                    info.locked_position = neon::Axis_X;
                                  });

    _physics.AddImpulse(crate, {10.0f, 0.0f, 10.0f});
    Run(30);

    const auto state = StateOf(crate);
    EXPECT_EQ(state.position.x, 0.0f);
    EXPECT_GT(state.position.z, 1.0f);
  }

  TEST_F(JoltPhysicsSystemTest, LetsAFallingBodyWithALockedRotationComeToRestOnTheFloor)
  {
    CreateFloor();
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 3.0f, 0.0f},
                                  [](BodyInfo &info)
                                  {
                                    info.rotation = Rotation{.yaw = 30.0f}.GetQuaternion();
                                    info.locked_rotation = neon::Axis_X | neon::Axis_Y | neon::Axis_Z;
                                  });
    Run(180);

    // Jolt lets what rests sink in by up to 0.02
    const auto state = StateOf(crate);
    EXPECT_NEAR(state.position.y, 0.5f, 0.03f);
    EXPECT_NEAR(std::abs(dot(state.rotation, Rotation{.yaw = 30.0f}.GetQuaternion())), 1.0f, 1e-4f);
  }

  TEST_F(JoltPhysicsSystemTest, RefusesADynamicBodyThatIsLockedOnEveryAxis)
  {
    BodyInfo info;
    info.entity = crate_entity;
    info.shapes = {Box({1.0f, 1.0f, 1.0f})};
    info.locked_position = neon::Axis_X | neon::Axis_Y | neon::Axis_Z;
    info.locked_rotation = neon::Axis_X | neon::Axis_Y | neon::Axis_Z;

    BodyId body = No_Body;
    EXPECT_FALSE(_physics.CreateBody(info, body, _error));
    EXPECT_THAT(_error, HasSubstr("locked along every axis and around every axis"));
    EXPECT_EQ(_physics.GetBodyCount(), 0u);

    // what does not move anyway is not locked
    info.kind = BodyKind::Static;
    EXPECT_TRUE(_physics.CreateBody(info, body, _error)) << _error;
  }

  // bodies that react

  TEST_F(JoltPhysicsSystemTest, LetsABoxFallAndComeToRestOnTheFloor)
  {
    CreateFloor();
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});

    Run(30);
    const auto falling = StateOf(crate);
    EXPECT_LT(falling.position.y, 5.0f);
    EXPECT_GT(falling.position.y, 0.5f);
    EXPECT_LT(falling.linear_velocity.y, -1.0f);
    EXPECT_TRUE(falling.active);

    Run(270);
    const auto resting = StateOf(crate);
    // Jolt lets what rests sink in by up to 0.02
    EXPECT_NEAR(resting.position.y, 0.5f, 0.03f);
    EXPECT_NEAR(resting.position.x, 0.0f, 0.01f);
    EXPECT_NEAR(resting.position.z, 0.0f, 0.01f);
    EXPECT_NEAR(length(resting.linear_velocity), 0.0f, 0.01f);

    // what rests is put to sleep
    EXPECT_FALSE(resting.active);
  }

  TEST_F(JoltPhysicsSystemTest, FallsAsFastAsGravitySays)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 100.0f, 0.0f},
                                  [](BodyInfo &info) { info.linear_damping = 0.0f; });

    Run(60);

    EXPECT_NEAR(StateOf(crate).linear_velocity.y, -9.81f, 0.01f);
    EXPECT_NEAR(StateOf(crate).position.y, 100.0f - 0.5f * 9.81f, 0.1f);
  }

  TEST_F(JoltPhysicsSystemTest, StacksBoxes)
  {
    CreateFloor();
    const auto lower = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 1.0f, 0.0f});
    const auto upper = CreateBody(other_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 3.0f, 0.0f});

    Run(300);

    EXPECT_NEAR(StateOf(lower).position.y, 0.5f, 0.03f);
    EXPECT_NEAR(StateOf(upper).position.y, 1.5f, 0.05f);
  }

  TEST_F(JoltPhysicsSystemTest, LetsASphereRollDownASlope)
  {
    // a slope that falls to the right
    CreateBody(floor_entity, BodyKind::Static, Box({20.0f, 1.0f, 20.0f}), {0.0f, 0.0f, 0.0f}, [](BodyInfo &info)
    {
      info.rotation = Rotation{.roll = -20.0f}.GetQuaternion();
    });
    const auto ball = CreateBody(ball_entity, BodyKind::Dynamic, Sphere(0.5f), {-3.0f, 3.0f, 0.0f});

    Run(120);

    const auto state = StateOf(ball);
    EXPECT_GT(state.position.x, -2.0f);
    EXPECT_LT(state.position.y, 2.0f);
    EXPECT_NEAR(state.position.z, 0.0f, 0.01f);
    EXPECT_GT(state.linear_velocity.x, 1.0f);

    // it rolls, and does not slide: it turns around the axis that points
    // away from the viewer
    EXPECT_LT(state.angular_velocity.z, -1.0f);
  }

  TEST_F(JoltPhysicsSystemTest, LeavesAStaticBodyWhereItIs)
  {
    const auto floor = CreateFloor();
    CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f},
               [](BodyInfo &info) { info.mass = 1000.0f; });

    Run(200);

    EXPECT_EQ(StateOf(floor).position, glm::vec3(0.0f, -0.5f, 0.0f));
    EXPECT_FALSE(StateOf(floor).active);
  }

  TEST_F(JoltPhysicsSystemTest, LetsABodyFloatThatGravityDoesNotPull)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f},
                                  [](BodyInfo &info) { info.gravity_scale = 0.0f; });

    Run(60);

    EXPECT_EQ(StateOf(crate).position, glm::vec3(0.0f, 5.0f, 0.0f));
  }

  TEST_F(JoltPhysicsSystemTest, BouncesWhatBounces)
  {
    CreateFloor();
    const auto dull = CreateBody(crate_entity, BodyKind::Dynamic, Sphere(0.5f), {-3.0f, 3.0f, 0.0f});
    const auto lively = CreateBody(ball_entity, BodyKind::Dynamic, Sphere(0.5f), {3.0f, 3.0f, 0.0f},
                                   [](BodyInfo &info) { info.bounce = 0.9f; });

    float highest_dull = 0.0f;
    float highest_lively = 0.0f;
    for (int i = 0; i < 120; i++)
    {
      Run(1);
      // after both have hit the floor
      if (i < 50) { continue; }
      highest_dull = std::max(highest_dull, StateOf(dull).position.y);
      highest_lively = std::max(highest_lively, StateOf(lively).position.y);
    }

    EXPECT_LT(highest_dull, 0.6f);
    EXPECT_GT(highest_lively, 1.5f);
  }

  TEST_F(JoltPhysicsSystemTest, MovesABodyWithTheVelocityItIsCreatedWith)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f},
                                  [](BodyInfo &info)
                                  {
                                    info.gravity_scale = 0.0f;
                                    info.linear_damping = 0.0f;
                                    info.angular_damping = 0.0f;
                                    info.linear_velocity = {2.0f, 0.0f, 0.0f};
                                    info.angular_velocity = {0.0f, glm::half_pi<float>(), 0.0f};
                                  });

    Run(60);

    const auto state = StateOf(crate);
    EXPECT_NEAR(state.position.x, 2.0f, 0.001f);
    // a quarter of a turn around the axis that points up
    const auto turned = state.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    EXPECT_NEAR(turned.x, -1.0f, 0.01f);
    EXPECT_NEAR(turned.z, 0.0f, 0.01f);
  }

  TEST_F(JoltPhysicsSystemTest, ChangesTheVelocityOfABody)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f},
                                  [](BodyInfo &info)
                                  {
                                    info.gravity_scale = 0.0f;
                                    info.linear_damping = 0.0f;
                                    info.angular_damping = 0.0f;
                                  });

    _physics.SetLinearVelocity(crate, {0.0f, 0.0f, -3.0f});
    _physics.SetAngularVelocity(crate, {1.0f, 0.0f, 0.0f});
    Run(60);

    EXPECT_NEAR(StateOf(crate).position.z, -3.0f, 0.001f);
    EXPECT_NEAR(StateOf(crate).angular_velocity.x, 1.0f, 0.001f);
  }

  TEST_F(JoltPhysicsSystemTest, KicksABodyWithAnImpulse)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f},
                                  [](BodyInfo &info)
                                  {
                                    info.mass = 4.0f;
                                    info.gravity_scale = 0.0f;
                                    info.linear_damping = 0.0f;
                                  });

    _physics.AddImpulse(crate, {8.0f, 0.0f, 0.0f});

    // an impulse is a change of momentum: 8 on a mass of 4 is a velocity of 2
    EXPECT_NEAR(StateOf(crate).linear_velocity.x, 2.0f, 0.001f);
  }

  TEST_F(JoltPhysicsSystemTest, TurnsABodyWithAnImpulseBesideItsMiddle)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f},
                                  [](BodyInfo &info) { info.gravity_scale = 0.0f; });

    _physics.AddImpulseAt(crate, {0.0f, 0.0f, -1.0f}, {0.5f, 5.0f, 0.0f});

    EXPECT_LT(StateOf(crate).linear_velocity.z, -0.5f);
    EXPECT_GT(std::abs(StateOf(crate).angular_velocity.y), 0.5f);
  }

  TEST_F(JoltPhysicsSystemTest, PushesABodyWithAForceForOneStep)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f},
                                  [](BodyInfo &info)
                                  {
                                    info.mass = 2.0f;
                                    info.gravity_scale = 0.0f;
                                    info.linear_damping = 0.0f;
                                  });

    // a force of 12 on a mass of 2 for a sixtieth of a second
    _physics.AddForce(crate, {12.0f, 0.0f, 0.0f});
    Run(1);
    EXPECT_NEAR(StateOf(crate).linear_velocity.x, 0.1f, 0.0001f);

    // and gone after it
    Run(10);
    EXPECT_NEAR(StateOf(crate).linear_velocity.x, 0.1f, 0.0001f);
  }

  TEST_F(JoltPhysicsSystemTest, TurnsABodyWithATorque)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f},
                                  [](BodyInfo &info) { info.gravity_scale = 0.0f; });

    _physics.AddTorque(crate, {0.0f, 10.0f, 0.0f});
    Run(1);

    EXPECT_GT(StateOf(crate).angular_velocity.y, 0.1f);
    EXPECT_EQ(StateOf(crate).position, glm::vec3(0.0f, 5.0f, 0.0f));
  }

  TEST_F(JoltPhysicsSystemTest, PutsABodySomewhereAtOnce)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});
    const auto floor = CreateFloor();

    _physics.SetBodyPlace(crate, {7.0f, 8.0f, 9.0f}, Rotation{.yaw = 90.0f}.GetQuaternion());
    _physics.SetBodyPlace(floor, {0.0f, -2.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

    EXPECT_EQ(StateOf(crate).position, glm::vec3(7.0f, 8.0f, 9.0f));
    EXPECT_NEAR(std::abs(dot(StateOf(crate).rotation, Rotation{.yaw = 90.0f}.GetQuaternion())), 1.0f, 1e-5f);
    EXPECT_EQ(StateOf(floor).position, glm::vec3(0.0f, -2.0f, 0.0f));
  }

  TEST_F(JoltPhysicsSystemTest, ChangesGravity)
  {
    EXPECT_EQ(_physics.GetGravity(), glm::vec3(0.0f, -9.81f, 0.0f));

    _physics.SetGravity({0.0f, 1.0f, 0.0f});
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});
    Run(60);

    EXPECT_EQ(_physics.GetGravity(), glm::vec3(0.0f, 1.0f, 0.0f));
    EXPECT_GT(StateOf(crate).position.y, 5.0f);
  }

  TEST_F(JoltPhysicsSystemTest, StopsWhatIsFastAtAThinWallWhenItLooksAlongItsWay)
  {
    const auto thin_wall = Box({10.0f, 10.0f, 0.05f});
    CreateBody(wall_entity, BodyKind::Static, thin_wall, {0.0f, 0.0f, -10.0f});

    const auto shoot = [&](const Entity entity, const float x, const bool continuous)
    {
      return CreateBody(entity, BodyKind::Dynamic, Sphere(0.05f), {x, 0.0f, 0.0f}, [continuous](BodyInfo &info)
      {
        info.gravity_scale = 0.0f;
        info.linear_damping = 0.0f;
        info.linear_velocity = {0.0f, 0.0f, -300.0f};
        info.continuous = continuous;
      });
    };
    const auto careless = shoot(crate_entity, -2.0f, false);
    const auto careful = shoot(ball_entity, 2.0f, true);

    Run(30);

    EXPECT_LT(StateOf(careless).position.z, -20.0f);
    EXPECT_GT(StateOf(careful).position.z, -10.0f);
  }

  // bodies that are moved by code

  TEST_F(JoltPhysicsSystemTest, MovesAKinematicBodyToWhereItIsMoved)
  {
    const auto lift = CreateBody(crate_entity, BodyKind::Kinematic, Box({2.0f, 0.2f, 2.0f}), {0.0f, 0.0f, 0.0f});

    _physics.MoveBody(lift, {0.0f, 0.5f, 0.0f}, Rotation{.yaw = 30.0f}.GetQuaternion(), step);
    Run(1);

    const auto state = StateOf(lift);
    EXPECT_NEAR(state.position.y, 0.5f, 1e-4f);
    EXPECT_NEAR(std::abs(dot(state.rotation, Rotation{.yaw = 30.0f}.GetQuaternion())), 1.0f, 1e-4f);
  }

  TEST_F(JoltPhysicsSystemTest, LetsAKinematicBodyStayWhereItArrived)
  {
    const auto lift = CreateBody(crate_entity, BodyKind::Kinematic, Box({2.0f, 0.2f, 2.0f}), {0.0f, 0.0f, 0.0f});

    _physics.MoveBody(lift, {0.0f, 0.5f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), step);
    Run(60);

    EXPECT_NEAR(StateOf(lift).position.y, 0.5f, 1e-4f);
    EXPECT_EQ(StateOf(lift).linear_velocity, glm::vec3(0.0f));
  }

  TEST_F(JoltPhysicsSystemTest, MovesAKinematicBodyWithTheVelocityItIsGiven)
  {
    const auto lift = CreateBody(crate_entity, BodyKind::Kinematic, Box({2.0f, 0.2f, 2.0f}), {0.0f, 0.0f, 0.0f});

    _physics.SetLinearVelocity(lift, {0.0f, 2.0f, 0.0f});
    Run(60);

    EXPECT_NEAR(StateOf(lift).position.y, 2.0f, 1e-3f);
  }

  TEST_F(JoltPhysicsSystemTest, LeavesAKinematicBodyAloneThatIsNotMoved)
  {
    CreateFloor();
    const auto lift = CreateBody(crate_entity, BodyKind::Kinematic, Box({2.0f, 0.2f, 2.0f}), {0.0f, 3.0f, 0.0f});

    Run(120);

    // gravity does not pull it
    EXPECT_EQ(StateOf(lift).position, glm::vec3(0.0f, 3.0f, 0.0f));
  }

  TEST_F(JoltPhysicsSystemTest, PushesADynamicBodyWithAKinematicOneThatIsNotPushedBack)
  {
    CreateFloor();
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {2.0f, 0.5f, 0.0f},
                                  [](BodyInfo &info) { info.mass = 1000.0f; });
    const auto pusher = CreateBody(other_entity, BodyKind::Kinematic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 0.5f, 0.0f});

    // 3 to the right in a second, through where the crate is
    for (int i = 1; i <= 60; i++)
    {
      _physics.MoveBody(pusher, {3.0f * static_cast<float>(i) / 60.0f, 0.5f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), step);
      Run(1);
    }

    // it arrived where it was moved to, however heavy what was in its way
    EXPECT_NEAR(StateOf(pusher).position.x, 3.0f, 1e-3f);
    EXPECT_NEAR(StateOf(pusher).position.y, 0.5f, 1e-3f);
    EXPECT_GT(StateOf(crate).position.x, 3.9f);
  }

  TEST_F(JoltPhysicsSystemTest, DoesNotMoveAKinematicBodyThatIsHit)
  {
    const auto door = CreateBody(wall_entity, BodyKind::Kinematic, Box({2.0f, 2.0f, 0.2f}), {0.0f, 0.0f, -3.0f});
    const auto ball = CreateBody(ball_entity, BodyKind::Dynamic, Sphere(0.5f), {0.0f, 0.0f, 0.0f}, [](BodyInfo &info)
    {
      info.mass = 500.0f;
      info.gravity_scale = 0.0f;
      info.linear_velocity = {0.0f, 0.0f, -10.0f};
    });

    Run(120);

    EXPECT_EQ(StateOf(door).position, glm::vec3(0.0f, 0.0f, -3.0f));
    // the ball was stopped or thrown back
    EXPECT_GT(StateOf(ball).position.z, -2.5f);
    EXPECT_EQ(Count(PhysicsEventKind::Began, wall_entity, ball_entity), 1u);
  }

  // what moves and slides

  TEST_F(JoltPhysicsSystemTest, LetsACharacterStandOnTheFloor)
  {
    CreateFloor();
    const auto player = CreateCharacter({0.0f, 3.0f, 0.0f});

    const auto state = Walk(player, {0.0f, 0.0f, 0.0f}, 120);

    EXPECT_TRUE(state.on_floor);
    EXPECT_FALSE(state.on_wall);
    EXPECT_FALSE(state.on_ceiling);
    EXPECT_EQ(state.floor, floor_entity);
    EXPECT_NEAR(state.floor_normal.y, 1.0f, 1e-3f);
    // the capsule is 2 high, and its middle is where the character is
    EXPECT_NEAR(state.position.y, 1.0f, 0.05f);
  }

  TEST_F(JoltPhysicsSystemTest, LetsACharacterFallThatIsInTheAir)
  {
    const auto player = CreateCharacter({0.0f, 50.0f, 0.0f});

    const auto state = Walk(player, {0.0f, 0.0f, 0.0f}, 60);

    EXPECT_FALSE(state.on_floor);
    EXPECT_EQ(state.floor, neon::No_Entity);
    EXPECT_NEAR(state.position.y, 50.0f - 0.5f * 9.81f, 0.2f);
    EXPECT_NEAR(state.velocity.y, -9.81f, 0.01f);
  }

  TEST_F(JoltPhysicsSystemTest, MovesACharacterByItsVelocity)
  {
    CreateFloor();
    const auto player = CreateCharacter({0.0f, 1.1f, 0.0f});
    Walk(player, {0.0f, 0.0f, 0.0f}, 30);

    const auto state = Walk(player, {2.0f, 0.0f, -1.0f}, 120);

    EXPECT_NEAR(state.position.x, 4.0f, 0.05f);
    EXPECT_NEAR(state.position.z, -2.0f, 0.05f);
    EXPECT_NEAR(state.velocity.x, 2.0f, 0.01f);
    EXPECT_NEAR(state.velocity.z, -1.0f, 0.01f);
    EXPECT_TRUE(state.on_floor);
  }

  TEST_F(JoltPhysicsSystemTest, StopsACharacterAtAWall)
  {
    CreateFloor();
    // its face lies at 3 to the right
    CreateBody(wall_entity, BodyKind::Static, Box({1.0f, 4.0f, 20.0f}), {3.5f, 2.0f, 0.0f});
    const auto player = CreateCharacter({0.0f, 1.1f, 0.0f});

    const auto state = Walk(player, {2.0f, 0.0f, 0.0f}, 240);

    // the capsule is half a unit wide to each side
    EXPECT_NEAR(state.position.x, 2.5f, 0.05f);
    EXPECT_NEAR(state.position.z, 0.0f, 0.01f);
    EXPECT_NEAR(state.velocity.x, 0.0f, 0.01f);
    EXPECT_TRUE(state.on_wall);
    EXPECT_TRUE(state.on_floor);
  }

  TEST_F(JoltPhysicsSystemTest, LetsACharacterSlideAlongAWall)
  {
    CreateFloor();
    CreateBody(wall_entity, BodyKind::Static, Box({1.0f, 4.0f, 40.0f}), {3.5f, 2.0f, 0.0f});
    const auto player = CreateCharacter({2.0f, 1.1f, 0.0f});

    // into the wall and along it
    const auto state = Walk(player, {2.0f, 0.0f, -2.0f}, 240);

    EXPECT_NEAR(state.position.x, 2.5f, 0.05f);
    // what it was stopped by took nothing from the way along the wall
    EXPECT_NEAR(state.position.z, -8.0f, 0.2f);
    EXPECT_NEAR(state.velocity.z, -2.0f, 0.05f);
    EXPECT_NEAR(state.velocity.x, 0.0f, 0.05f);
  }

  TEST_F(JoltPhysicsSystemTest, DoesNotPushACharacterWithADynamicBody)
  {
    CreateFloor();
    const auto player = CreateCharacter({0.0f, 1.1f, 0.0f});
    Walk(player, {0.0f, 0.0f, 0.0f}, 30);
    const auto ball = CreateBody(ball_entity, BodyKind::Dynamic, Sphere(0.5f), {-5.0f, 1.0f, 0.0f}, [](BodyInfo &info)
    {
      info.mass = 500.0f;
      info.linear_velocity = {10.0f, 0.0f, 0.0f};
    });

    const auto state = Walk(player, {0.0f, 0.0f, 0.0f}, 120);

    EXPECT_NEAR(state.position.x, 0.0f, 0.01f);
    EXPECT_NEAR(state.position.z, 0.0f, 0.01f);
    // the ball did not pass through
    EXPECT_LT(StateOf(ball).position.x, 0.0f);
    EXPECT_EQ(Count(PhysicsEventKind::Began, player_entity, ball_entity), 1u);
  }

  TEST_F(JoltPhysicsSystemTest, PushesADynamicBodyWithACharacter)
  {
    CreateFloor();
    const auto player = CreateCharacter({0.0f, 1.1f, 0.0f});
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {2.0f, 0.5f, 0.0f},
                                  [](BodyInfo &info) { info.can_sleep = false; });

    const auto state = Walk(player, {2.0f, 0.0f, 0.0f}, 240);

    EXPECT_GT(StateOf(crate).position.x, 3.0f);
    EXPECT_GT(state.position.x, 2.0f);
  }

  TEST_F(JoltPhysicsSystemTest, LetsACharacterWalkUpAStepThatIsLowEnough)
  {
    CreateFloor();
    // a low step and a high one
    CreateBody(wall_entity, BodyKind::Static, Box({4.0f, 0.2f, 4.0f}), {4.0f, 0.1f, 0.0f});
    CreateBody(other_entity, BodyKind::Static, Box({4.0f, 1.0f, 4.0f}), {4.0f, 0.5f, 10.0f});
    const auto low = CreateCharacter({0.0f, 1.1f, 0.0f});
    const auto high = CreateCharacter({0.0f, 1.1f, 10.0f});

    const auto on_low = Walk(low, {2.0f, 0.0f, 0.0f}, 120);
    _fall = 0.0f;
    const auto at_high = Walk(high, {2.0f, 0.0f, 0.0f}, 120);

    EXPECT_GT(on_low.position.x, 3.0f);
    EXPECT_NEAR(on_low.position.y, 1.2f, 0.05f);
    EXPECT_LT(at_high.position.x, 1.6f);
    EXPECT_NEAR(at_high.position.y, 1.0f, 0.05f);
  }

  TEST_F(JoltPhysicsSystemTest, LetsACharacterWalkUpAStepWhilePressedAgainstAWall)
  {
    CreateFloor();
    // a wall whose face lies at 4 to the right, and a step of 0.2 across
    // the way along it, which reaches the wall
    CreateBody(wall_entity, BodyKind::Static, Box({1.0f, 4.0f, 40.0f}), {4.5f, 2.0f, 0.0f});
    CreateBody(other_entity, BodyKind::Static, Box({4.0f, 0.2f, 1.0f}), {2.0f, 0.1f, -3.5f});
    const auto player = CreateCharacter({3.0f, 1.1f, 0.0f}, [](CharacterInfo &info)
    {
      info.shapes = {Capsule(0.4f, 1.8f)};
    });
    Walk(player, {0.0f, 0.0f, 0.0f}, 30);

    // mostly into the wall, and a little along it, onto the step: what a
    // player does who leans on a wall and strafes along it
    const auto state = Walk(player, {3.86f, 0.0f, -1.03f}, 240);

    EXPECT_NEAR(state.position.x, 3.58f, 0.05f);
    EXPECT_LT(state.position.z, -3.5f);
    EXPECT_NEAR(state.position.y, 1.1f, 0.05f);
    EXPECT_TRUE(state.on_floor);
    EXPECT_EQ(state.floor, other_entity);
  }

  TEST_F(JoltPhysicsSystemTest, LetsACharacterStandStillOnASlope)
  {
    // a slope of 14 degrees that falls to the right, well under the 45 a
    // character can stand on, with its top through the origin
    CreateBody(floor_entity, BodyKind::Static, Box({20.0f, 1.0f, 20.0f}), {0.0f, -0.5f, 0.0f}, [](BodyInfo &info)
    {
      info.rotation = Rotation{.roll = -14.0f}.GetQuaternion();
    });
    const auto player = CreateCharacter({0.0f, 1.1f, 0.0f});
    const auto settled = Walk(player, {0.0f, 0.0f, 0.0f}, 60);
    ASSERT_TRUE(settled.on_floor);

    const auto state = Walk(player, {0.0f, 0.0f, 0.0f}, 300);

    // five seconds later it stands where it stood, within a millimetre
    EXPECT_TRUE(state.on_floor);
    EXPECT_FALSE(state.on_wall);
    EXPECT_NEAR(state.position.x, settled.position.x, 1e-3f);
    EXPECT_NEAR(state.position.y, settled.position.y, 1e-3f);
    EXPECT_NEAR(state.position.z, settled.position.z, 1e-3f);
    EXPECT_NEAR(state.velocity.x, 0.0f, 1e-3f);
  }

  TEST_F(JoltPhysicsSystemTest, LetsACharacterSlideDownASlopeThatIsTooSteep)
  {
    // a slope of 60 degrees that falls to the right, which is a wall to a
    // character that stands on 45 at most
    CreateBody(floor_entity, BodyKind::Static, Box({20.0f, 1.0f, 20.0f}), {0.0f, -0.5f, 0.0f}, [](BodyInfo &info)
    {
      info.rotation = Rotation{.roll = -60.0f}.GetQuaternion();
    });
    const auto player = CreateCharacter({0.0f, 1.5f, 0.0f});

    const auto state = Walk(player, {0.0f, 0.0f, 0.0f}, 30);

    EXPECT_FALSE(state.on_floor);
    EXPECT_TRUE(state.on_wall);
    EXPECT_GT(state.position.x, 0.3f);
    EXPECT_LT(state.position.y, 1.0f);
  }

  TEST_F(JoltPhysicsSystemTest, PutsACharacterSomewhereAtOnce)
  {
    const auto player = CreateCharacter({0.0f, 1.0f, 0.0f});

    _physics.SetCharacterPosition(player, {10.0f, 20.0f, 30.0f});
    CharacterState state;
    ASSERT_TRUE(_physics.MoveCharacter(player, {0.0f, 0.0f, 0.0f}, step, state));

    EXPECT_NEAR(state.position.x, 10.0f, 1e-3f);
    EXPECT_NEAR(state.position.y, 20.0f, 1e-3f);
    EXPECT_NEAR(state.position.z, 30.0f, 1e-3f);
  }

  TEST_F(JoltPhysicsSystemTest, RefusesACharacterWithAShapeThatHasNoInside)
  {
    CharacterInfo info;
    info.shapes = {CubeOf(ShapeKind::Mesh)};
    CharacterId character = 99;

    EXPECT_FALSE(_physics.CreateCharacter(info, character, _error));

    EXPECT_EQ(character, No_Character);
    EXPECT_EQ(_error, "a character cannot have a mesh. Give it a capsule, or another shape without dents");
    EXPECT_EQ(_physics.GetCharacterCount(), 0u);
  }

  TEST_F(JoltPhysicsSystemTest, DestroysACharacter)
  {
    const auto player = CreateCharacter({0.0f, 1.0f, 0.0f});
    ASSERT_EQ(_physics.GetCharacterCount(), 1u);
    // what a character is to the bodies around it does not count as a body
    EXPECT_EQ(_physics.GetBodyCount(), 0u);

    _physics.DestroyCharacter(player);

    EXPECT_EQ(_physics.GetCharacterCount(), 0u);
    CharacterState state;
    EXPECT_FALSE(_physics.MoveCharacter(player, {0.0f, 0.0f, 0.0f}, step, state));

    // and no ray finds it
    RayHit hit;
    EXPECT_FALSE(_physics.CastRay(Ray{.origin = {0.0f, 1.0f, 5.0f}}, QueryFilter{}, hit));
  }

  // triggers

  TEST_F(JoltPhysicsSystemTest, ReportsWhatEntersAndLeavesATriggerOnceEach)
  {
    const auto trigger = CreateTrigger(Box({4.0f, 2.0f, 4.0f}), {0.0f, 5.0f, 0.0f});
    const auto ball = CreateBody(ball_entity, BodyKind::Dynamic, Sphere(0.5f), {0.0f, 10.0f, 0.0f});

    Run(180);

    ASSERT_EQ(_events.size(), 2u);

    EXPECT_EQ(_events[0].kind, PhysicsEventKind::Began);
    EXPECT_TRUE(_events[0].trigger);
    EXPECT_EQ(_events[0].first, trigger_entity);
    EXPECT_EQ(_events[0].second, ball_entity);
    EXPECT_EQ(_events[0].first_body, trigger);
    EXPECT_EQ(_events[0].second_body, ball);

    EXPECT_EQ(_events[1].kind, PhysicsEventKind::Ended);
    EXPECT_TRUE(_events[1].trigger);
    EXPECT_EQ(_events[1].first, trigger_entity);
    EXPECT_EQ(_events[1].second, ball_entity);

    EXPECT_LT(StateOf(ball).position.y, 0.0f);
  }

  TEST_F(JoltPhysicsSystemTest, ChangesNothingOfWhatPassesThroughATrigger)
  {
    const auto fall = [](const bool with_trigger)
    {
      const auto logger = std::make_shared<RecordingLogger>();
      Jolt_PhysicsSystem physics{SettingsConfig{}, logger};
      physics.Initialize();
      std::string error;

      BodyInfo ball;
      ball.entity = ball_entity;
      ball.shapes = {Sphere(0.5f)};
      ball.position = {0.0f, 10.0f, 0.0f};
      ball.linear_velocity = {1.0f, 0.0f, 0.0f};
      ball.angular_velocity = {1.0f, 2.0f, 3.0f};
      BodyId body = No_Body;
      EXPECT_TRUE(physics.CreateBody(ball, body, error));

      if (with_trigger)
      {
        BodyInfo trigger;
        trigger.entity = trigger_entity;
        trigger.kind = BodyKind::Kinematic;
        trigger.trigger = true;
        trigger.shapes = {Box({4.0f, 2.0f, 4.0f})};
        trigger.position = {0.0f, 5.0f, 0.0f};
        BodyId trigger_body = No_Body;
        EXPECT_TRUE(physics.CreateBody(trigger, trigger_body, error));
      }

      std::vector<BodyState> states;
      for (int i = 0; i < 120; i++)
      {
        physics.Step(step);
        BodyState state;
        EXPECT_TRUE(physics.GetBodyState(body, state));
        states.push_back(state);
      }
      physics.CleanUp();
      return states;
    };

    const auto without = fall(false);
    const auto with = fall(true);

    ASSERT_EQ(without.size(), with.size());
    for (std::size_t i = 0; i < without.size(); i++)
    {
      // the same, and not merely close
      EXPECT_EQ(with[i].position, without[i].position) << "step " << i;
      EXPECT_EQ(with[i].rotation, without[i].rotation) << "step " << i;
      EXPECT_EQ(with[i].linear_velocity, without[i].linear_velocity) << "step " << i;
      EXPECT_EQ(with[i].angular_velocity, without[i].angular_velocity) << "step " << i;
    }
  }

  TEST_F(JoltPhysicsSystemTest, LeavesATriggerWhereItIs)
  {
    CreateFloor();
    const auto trigger = CreateTrigger(Box({4.0f, 2.0f, 4.0f}), {0.0f, 5.0f, 0.0f});
    CreateBody(ball_entity, BodyKind::Dynamic, Sphere(0.5f), {0.0f, 10.0f, 0.0f},
               [](BodyInfo &info) { info.mass = 100.0f; });

    Run(120);

    EXPECT_EQ(StateOf(trigger).position, glm::vec3(0.0f, 5.0f, 0.0f));
  }

  TEST_F(JoltPhysicsSystemTest, ReportsWhatRestsInsideATriggerOnce)
  {
    CreateFloor();
    CreateTrigger(Box({4.0f, 4.0f, 4.0f}), {0.0f, 1.0f, 0.0f});
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 1.0f, 0.0f});

    // long enough for the crate to be put to sleep
    Run(600);

    EXPECT_FALSE(StateOf(crate).active);
    EXPECT_EQ(Count(PhysicsEventKind::Began, trigger_entity, crate_entity), 1u);
    EXPECT_EQ(Count(PhysicsEventKind::Ended, trigger_entity, crate_entity), 0u);
  }

  TEST_F(JoltPhysicsSystemTest, ReportsWhatDoesNotMoveInsideATrigger)
  {
    CreateBody(wall_entity, BodyKind::Static, Box({1.0f, 1.0f, 1.0f}), {0.0f, 1.0f, 0.0f});
    CreateTrigger(Box({4.0f, 4.0f, 4.0f}), {0.0f, 1.0f, 0.0f});

    Run(3);

    EXPECT_EQ(Count(PhysicsEventKind::Began, trigger_entity, wall_entity), 1u);
  }

  TEST_F(JoltPhysicsSystemTest, ReportsACharacterThatEntersATrigger)
  {
    CreateFloor();
    CreateTrigger(Box({2.0f, 4.0f, 4.0f}), {5.0f, 1.0f, 0.0f});
    const auto player = CreateCharacter({0.0f, 1.1f, 0.0f});

    Walk(player, {3.0f, 0.0f, 0.0f}, 240);

    EXPECT_EQ(Count(PhysicsEventKind::Began, trigger_entity, player_entity), 1u);
    EXPECT_EQ(Count(PhysicsEventKind::Ended, trigger_entity, player_entity), 1u);
  }

  TEST_F(JoltPhysicsSystemTest, ReportsThatATriggerWasMovedOntoABody)
  {
    const auto trigger = CreateTrigger(Box({2.0f, 2.0f, 2.0f}), {50.0f, 0.0f, 0.0f});
    CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 0.0f, 0.0f},
               [](BodyInfo &info) { info.gravity_scale = 0.0f; });
    Run(3);
    EXPECT_TRUE(_events.empty());

    _physics.SetBodyPlace(trigger, {0.0f, 0.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    Run(3);
    EXPECT_EQ(Count(PhysicsEventKind::Began, trigger_entity, crate_entity), 1u);

    _physics.SetBodyPlace(trigger, {50.0f, 0.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    Run(3);
    EXPECT_EQ(Count(PhysicsEventKind::Ended, trigger_entity, crate_entity), 1u);
  }

  TEST_F(JoltPhysicsSystemTest, ReportsABodyOfSeveralShapesOnce)
  {
    CreateTrigger(Box({4.0f, 2.0f, 4.0f}), {0.0f, 5.0f, 0.0f});
    BodyInfo dumbbell;
    dumbbell.entity = crate_entity;
    dumbbell.shapes = {Sphere(0.5f), Sphere(0.5f), Box({2.0f, 0.2f, 0.2f})};
    dumbbell.shapes[0].position = {-1.0f, 0.0f, 0.0f};
    dumbbell.shapes[1].position = {1.0f, 0.0f, 0.0f};
    dumbbell.position = {0.0f, 10.0f, 0.0f};
    Create(dumbbell);

    Run(180);

    EXPECT_EQ(Count(PhysicsEventKind::Began, trigger_entity, crate_entity), 1u);
    EXPECT_EQ(Count(PhysicsEventKind::Ended, trigger_entity, crate_entity), 1u);
    EXPECT_EQ(_events.size(), 2u);
  }

  // events of bodies that collide

  TEST_F(JoltPhysicsSystemTest, ReportsThatTwoBodiesBeganToTouch)
  {
    const auto floor = CreateFloor();
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {2.0f, 3.0f, 1.0f});

    Run(120);

    ASSERT_EQ(_events.size(), 1u);
    const auto &event = _events[0];
    EXPECT_EQ(event.kind, PhysicsEventKind::Began);
    EXPECT_FALSE(event.trigger);

    // the one that was created first comes first
    EXPECT_EQ(event.first, floor_entity);
    EXPECT_EQ(event.second, crate_entity);
    EXPECT_EQ(event.first_body, floor);
    EXPECT_EQ(event.second_body, crate);

    // on top of the floor, below the crate, pointing from the floor to the
    // crate
    EXPECT_NEAR(event.point.y, 0.0f, 0.1f);
    EXPECT_NEAR(event.point.x, 2.0f, 0.6f);
    EXPECT_NEAR(event.point.z, 1.0f, 0.6f);
    EXPECT_NEAR(event.normal.y, 1.0f, 0.01f);
  }

  TEST_F(JoltPhysicsSystemTest, DoesNotReportThatATouchEndedWhenWhatTouchesComesToRest)
  {
    CreateFloor();
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 1.0f, 0.0f});

    Run(600);
    ASSERT_FALSE(StateOf(crate).active);

    EXPECT_EQ(Count(PhysicsEventKind::Began, floor_entity, crate_entity), 1u);
    EXPECT_EQ(Count(PhysicsEventKind::Ended, floor_entity, crate_entity), 0u);

    // woken up where it lies, it began nothing new
    _physics.AddImpulse(crate, {0.0f, 0.0f, 0.001f});
    Run(60);
    EXPECT_EQ(Count(PhysicsEventKind::Began, floor_entity, crate_entity), 1u);
    EXPECT_EQ(Count(PhysicsEventKind::Ended, floor_entity, crate_entity), 0u);
  }

  TEST_F(JoltPhysicsSystemTest, ReportsThatATouchEndedWhenWhatRestedIsTakenAway)
  {
    CreateFloor();
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 1.0f, 0.0f});
    Run(600);
    ASSERT_FALSE(StateOf(crate).active);

    _physics.SetBodyPlace(crate, {0.0f, 50.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    Run(2);

    EXPECT_EQ(Count(PhysicsEventKind::Ended, floor_entity, crate_entity), 1u);
  }

  TEST_F(JoltPhysicsSystemTest, ReportsThatATouchEndedWhenWhatBouncedLeaves)
  {
    CreateFloor();
    CreateBody(ball_entity, BodyKind::Dynamic, Sphere(0.5f), {0.0f, 3.0f, 0.0f},
               [](BodyInfo &info) { info.bounce = 0.9f; });

    Run(90);

    EXPECT_GE(Count(PhysicsEventKind::Began, floor_entity, ball_entity), 1u);
    EXPECT_GE(Count(PhysicsEventKind::Ended, floor_entity, ball_entity), 1u);
  }

  TEST_F(JoltPhysicsSystemTest, ReportsThatATouchEndedWhenABodyIsDestroyed)
  {
    CreateFloor();
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 0.6f, 0.0f});
    Run(30);
    ASSERT_EQ(Count(PhysicsEventKind::Began, floor_entity, crate_entity), 1u);

    _physics.DestroyBody(crate);
    Run(2);

    ASSERT_EQ(Count(PhysicsEventKind::Ended, floor_entity, crate_entity), 1u);
    // it still says who it was
    EXPECT_EQ(_events.back().second, crate_entity);
    EXPECT_EQ(_events.back().second_body, crate);
  }

  TEST_F(JoltPhysicsSystemTest, ReportsThatATouchEndedWhenABodyThatRestedIsDestroyed)
  {
    CreateFloor();
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 1.0f, 0.0f});
    Run(600);
    ASSERT_FALSE(StateOf(crate).active);

    _physics.DestroyBody(crate);
    Run(2);

    EXPECT_EQ(Count(PhysicsEventKind::Ended, floor_entity, crate_entity), 1u);
  }

  TEST_F(JoltPhysicsSystemTest, KeepsTheEventsOfAStepAndOfAFrameApart)
  {
    CreateFloor();
    CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {-5.0f, 0.55f, 0.0f});

    // a frame of many steps, in one of which the crate lands
    for (int i = 0; i < 30 && _physics.GetFrameEvents().empty(); i++) { _physics.Step(step); }
    ASSERT_EQ(_physics.GetFrameEvents().size(), 1u);
    ASSERT_EQ(_physics.GetStepEvents().size(), 1u);

    // the step after it has nothing to report, and the frame still knows
    _physics.Step(step);
    EXPECT_TRUE(_physics.GetStepEvents().empty());
    EXPECT_EQ(_physics.GetFrameEvents().size(), 1u);

    _physics.EndFrame();
    EXPECT_TRUE(_physics.GetFrameEvents().empty());

    // a frame without a step has none
    _physics.EndFrame();
    EXPECT_TRUE(_physics.GetFrameEvents().empty());
  }

  TEST_F(JoltPhysicsSystemTest, HandsOverTheEventsOfAStepInOrder)
  {
    CreateFloor();
    // many that land in the same step
    for (int i = 0; i < 40; i++)
    {
      CreateBody(100 + static_cast<Entity>(i), BodyKind::Dynamic, Sphere(0.4f),
                 {-40.0f + 2.0f * static_cast<float>(i), 1.0f, 0.0f});
    }

    for (int i = 0; i < 60; i++)
    {
      _physics.Step(step);

      const auto &events = _physics.GetStepEvents();
      for (std::size_t j = 1; j < events.size(); j++)
      {
        EXPECT_LT(events[j - 1].second_body, events[j].second_body);
      }
      _events.insert(_events.end(), events.begin(), events.end());
    }

    EXPECT_EQ(_events.size(), 40u);
  }

  // layers and masks

  TEST_F(JoltPhysicsSystemTest, LetsBodiesPassThatDoNotLookForEachOther)
  {
    CreateBody(floor_entity, BodyKind::Static, Box({100.0f, 1.0f, 100.0f}), {0.0f, -0.5f, 0.0f}, [](BodyInfo &info)
    {
      info.layers = 0b01;
      info.mask = 0b01;
    });
    const auto lands = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {-3.0f, 2.0f, 0.0f},
                                  [](BodyInfo &info)
                                  {
                                    info.layers = 0b01;
                                    info.mask = 0b01;
                                  });
    const auto passes = CreateBody(other_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {3.0f, 2.0f, 0.0f},
                                   [](BodyInfo &info)
                                   {
                                     info.layers = 0b10;
                                     info.mask = 0b10;
                                   });

    Run(120);

    EXPECT_NEAR(StateOf(lands).position.y, 0.5f, 0.02f);
    EXPECT_LT(StateOf(passes).position.y, -5.0f);
    EXPECT_EQ(Count(PhysicsEventKind::Began, floor_entity, other_entity), 0u);
  }

  TEST_F(JoltPhysicsSystemTest, LetsBodiesCollideWhenOneOfThemLooksForTheOther)
  {
    // the floor is in layer 2 and looks for nothing
    CreateBody(floor_entity, BodyKind::Static, Box({100.0f, 1.0f, 100.0f}), {0.0f, -0.5f, 0.0f}, [](BodyInfo &info)
    {
      info.layers = 0b10;
      info.mask = 0;
    });
    // in layer 1, looking for layer 2
    const auto looks = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {-3.0f, 2.0f, 0.0f},
                                  [](BodyInfo &info)
                                  {
                                    info.layers = 0b01;
                                    info.mask = 0b10;
                                  });
    // in layer 1, looking for layer 1
    const auto looks_elsewhere = CreateBody(other_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}),
                                            {3.0f, 2.0f, 0.0f}, [](BodyInfo &info)
                                            {
                                              info.layers = 0b01;
                                              info.mask = 0b01;
                                            });

    Run(120);

    EXPECT_NEAR(StateOf(looks).position.y, 0.5f, 0.02f);
    EXPECT_LT(StateOf(looks_elsewhere).position.y, -5.0f);
  }

  TEST_F(JoltPhysicsSystemTest, TellsAll32LayersApart)
  {
    CreateBody(floor_entity, BodyKind::Static, Box({100.0f, 1.0f, 100.0f}), {0.0f, -0.5f, 0.0f}, [](BodyInfo &info)
    {
      info.layers = 0x80000000u;
      info.mask = 0;
    });
    const auto highest = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {-3.0f, 2.0f, 0.0f},
                                    [](BodyInfo &info)
                                    {
                                      info.layers = 0;
                                      info.mask = 0x80000000u;
                                    });
    const auto next_to_it = CreateBody(other_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {3.0f, 2.0f, 0.0f},
                                       [](BodyInfo &info)
                                       {
                                         info.layers = 0;
                                         info.mask = 0x40000000u;
                                       });

    Run(120);

    EXPECT_NEAR(StateOf(highest).position.y, 0.5f, 0.02f);
    EXPECT_LT(StateOf(next_to_it).position.y, -5.0f);
  }

  TEST_F(JoltPhysicsSystemTest, ReportsToATriggerOnlyWhatItLooksFor)
  {
    // looks for layer 2
    CreateTrigger(Box({20.0f, 2.0f, 4.0f}), {0.0f, 5.0f, 0.0f}, 0b10);
    CreateBody(crate_entity, BodyKind::Dynamic, Sphere(0.5f), {-3.0f, 10.0f, 0.0f}, [](BodyInfo &info)
    {
      info.layers = 0b01;
      // that the body looks for the trigger does not count
      info.mask = 0b11;
    });
    CreateBody(ball_entity, BodyKind::Dynamic, Sphere(0.5f), {3.0f, 10.0f, 0.0f}, [](BodyInfo &info)
    {
      info.layers = 0b10;
      info.mask = 0;
    });

    Run(180);

    EXPECT_EQ(Count(PhysicsEventKind::Began, trigger_entity, crate_entity), 0u);
    EXPECT_EQ(Count(PhysicsEventKind::Began, trigger_entity, ball_entity), 1u);
    EXPECT_EQ(Count(PhysicsEventKind::Ended, trigger_entity, ball_entity), 1u);
  }

  TEST_F(JoltPhysicsSystemTest, LetsACharacterPassWhatItDoesNotLookFor)
  {
    CreateFloor();
    CreateBody(wall_entity, BodyKind::Static, Box({1.0f, 4.0f, 20.0f}), {3.5f, 2.0f, 0.0f}, [](BodyInfo &info)
    {
      info.layers = 0b10;
      info.mask = 0;
    });
    const auto player = CreateCharacter({0.0f, 1.1f, 0.0f});

    const auto state = Walk(player, {2.0f, 0.0f, 0.0f}, 240);

    EXPECT_GT(state.position.x, 7.0f);
  }

  // queries

  TEST_F(JoltPhysicsSystemTest, HitsWhatARayPointsAt)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Static, Box({2.0f, 2.0f, 2.0f}), {0.0f, 0.0f, -10.0f});

    RayHit hit;
    const bool found = _physics.CastRay(
      Ray{.origin = {0.0f, 0.0f, 0.0f}, .direction = {0.0f, 0.0f, -5.0f}, .distance = 100.0f}, QueryFilter{}, hit);

    ASSERT_TRUE(found);
    EXPECT_EQ(hit.entity, crate_entity);
    EXPECT_EQ(hit.body, crate);
    EXPECT_FALSE(hit.trigger);
    EXPECT_NEAR(hit.distance, 9.0f, 1e-3f);
    EXPECT_NEAR(hit.point.z, -9.0f, 1e-3f);
    EXPECT_NEAR(hit.point.x, 0.0f, 1e-3f);
    EXPECT_NEAR(hit.normal.z, 1.0f, 1e-3f);
  }

  TEST_F(JoltPhysicsSystemTest, ABodyThatIsTakenOutOfTheWorldIsKeptAndNothingFindsItUntilItIsPutBack)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Static, Box({2.0f, 2.0f, 2.0f}), {0.0f, 0.0f, -10.0f});
    const Ray ray{.origin = {0.0f, 0.0f, 0.0f}, .direction = {0.0f, 0.0f, -5.0f}, .distance = 100.0f};
    RayHit hit;
    ASSERT_TRUE(_physics.CastRay(ray, QueryFilter{}, hit));

    _physics.SetBodyInWorld(crate, false);
    _physics.SetBodyInWorld(crate, false);

    EXPECT_FALSE(_physics.CastRay(ray, QueryFilter{}, hit));
    EXPECT_TRUE(_physics.HasBody(crate)) << "it is kept, with its shapes";
    EXPECT_EQ(_physics.GetBodyCount(), 1u);

    // moved while it is out, and found where it was put when it is back
    _physics.SetBodyPlace(crate, {0.0f, 0.0f, -20.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
    _physics.SetBodyInWorld(crate, true);
    _physics.SetBodyInWorld(crate, true);

    ASSERT_TRUE(_physics.CastRay(ray, QueryFilter{}, hit));
    EXPECT_EQ(hit.body, crate);
    EXPECT_NEAR(hit.distance, 19.0f, 1e-3f);

    // and one that is not there is left alone
    _physics.SetBodyInWorld(12345, false);
  }

  TEST_F(JoltPhysicsSystemTest, MissesWhatARayDoesNotPointAt)
  {
    CreateBody(crate_entity, BodyKind::Static, Box({2.0f, 2.0f, 2.0f}), {0.0f, 0.0f, -10.0f});
    RayHit hit;

    // to the other side
    EXPECT_FALSE(_physics.CastRay(Ray{.direction = {0.0f, 0.0f, 1.0f}}, QueryFilter{}, hit));
    // past it
    EXPECT_FALSE(_physics.CastRay(Ray{.origin = {5.0f, 0.0f, 0.0f}, .direction = {0.0f, 0.0f, -1.0f}}, QueryFilter{}, hit));
    // not far enough
    EXPECT_FALSE(_physics.CastRay(Ray{.direction = {0.0f, 0.0f, -1.0f}, .distance = 8.9f}, QueryFilter{}, hit));
    // nowhere
    EXPECT_FALSE(_physics.CastRay(Ray{.direction = {0.0f, 0.0f, 0.0f}}, QueryFilter{}, hit));

    EXPECT_EQ(hit.entity, neon::No_Entity);
  }

  TEST_F(JoltPhysicsSystemTest, HitsTheNearestOfWhatARayPointsAt)
  {
    CreateBody(wall_entity, BodyKind::Static, Box({2.0f, 2.0f, 2.0f}), {0.0f, 0.0f, -20.0f});
    CreateBody(crate_entity, BodyKind::Dynamic, Sphere(1.0f), {0.0f, 0.0f, -10.0f},
               [](BodyInfo &info) { info.gravity_scale = 0.0f; });

    RayHit hit;
    ASSERT_TRUE(_physics.CastRay(Ray{.direction = {0.0f, 0.0f, -1.0f}}, QueryFilter{}, hit));

    EXPECT_EQ(hit.entity, crate_entity);
    EXPECT_NEAR(hit.distance, 9.0f, 1e-3f);
  }

  TEST_F(JoltPhysicsSystemTest, HitsWithARayOnlyWhatIsInTheLayersItLooksFor)
  {
    CreateBody(crate_entity, BodyKind::Static, Box({2.0f, 2.0f, 2.0f}), {0.0f, 0.0f, -10.0f},
               [](BodyInfo &info) { info.layers = 0b01; });
    CreateBody(wall_entity, BodyKind::Static, Box({2.0f, 2.0f, 2.0f}), {0.0f, 0.0f, -20.0f},
               [](BodyInfo &info) { info.layers = 0b10; });
    const Ray ray{.direction = {0.0f, 0.0f, -1.0f}};
    RayHit hit;

    ASSERT_TRUE(_physics.CastRay(ray, QueryFilter{.mask = 0b10}, hit));
    EXPECT_EQ(hit.entity, wall_entity);

    ASSERT_TRUE(_physics.CastRay(ray, QueryFilter{.mask = 0b11}, hit));
    EXPECT_EQ(hit.entity, crate_entity);

    EXPECT_FALSE(_physics.CastRay(ray, QueryFilter{.mask = 0b100}, hit));
  }

  TEST_F(JoltPhysicsSystemTest, SkipsTheEntityThatAsks)
  {
    CreateBody(crate_entity, BodyKind::Static, Box({2.0f, 2.0f, 2.0f}), {0.0f, 0.0f, -10.0f});
    CreateBody(wall_entity, BodyKind::Static, Box({2.0f, 2.0f, 2.0f}), {0.0f, 0.0f, -20.0f});
    RayHit hit;

    ASSERT_TRUE(_physics.CastRay(Ray{.direction = {0.0f, 0.0f, -1.0f}}, QueryFilter{.ignore = crate_entity}, hit));

    EXPECT_EQ(hit.entity, wall_entity);
  }

  TEST_F(JoltPhysicsSystemTest, HitsATriggerWithARayOnlyWhenAsked)
  {
    const auto trigger = CreateTrigger(Box({2.0f, 2.0f, 2.0f}), {0.0f, 0.0f, -10.0f});
    const Ray ray{.direction = {0.0f, 0.0f, -1.0f}};
    RayHit hit;

    EXPECT_FALSE(_physics.CastRay(ray, QueryFilter{}, hit));

    ASSERT_TRUE(_physics.CastRay(ray, QueryFilter{.triggers = true}, hit));
    EXPECT_EQ(hit.body, trigger);
    EXPECT_TRUE(hit.trigger);
  }

  TEST_F(JoltPhysicsSystemTest, HitsACharacterWithARay)
  {
    CreateCharacter({0.0f, 0.0f, -10.0f});
    RayHit hit;

    ASSERT_TRUE(_physics.CastRay(Ray{.direction = {0.0f, 0.0f, -1.0f}}, QueryFilter{}, hit));

    EXPECT_EQ(hit.entity, player_entity);
    EXPECT_NEAR(hit.distance, 9.5f, 0.05f);
  }

  TEST_F(JoltPhysicsSystemTest, FindsWhatOverlapsAShape)
  {
    const auto near = CreateBody(crate_entity, BodyKind::Static, Box({1.0f, 1.0f, 1.0f}), {1.0f, 0.0f, 0.0f});
    const auto also_near = CreateBody(ball_entity, BodyKind::Dynamic, Sphere(0.5f), {0.0f, 1.5f, 0.0f},
                                      [](BodyInfo &info) { info.gravity_scale = 0.0f; });
    CreateBody(wall_entity, BodyKind::Static, Box({1.0f, 1.0f, 1.0f}), {10.0f, 0.0f, 0.0f});
    CreateTrigger(Box({1.0f, 1.0f, 1.0f}), {-1.0f, 0.0f, 0.0f});
    std::vector<OverlapHit> hits;

    _physics.Overlap(Sphere(2.0f), {0.0f, 0.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), QueryFilter{}, hits);

    ASSERT_EQ(hits.size(), 2u);
    EXPECT_EQ(hits[0].body, near);
    EXPECT_EQ(hits[0].entity, crate_entity);
    EXPECT_EQ(hits[1].body, also_near);
    EXPECT_EQ(hits[1].entity, ball_entity);

    _physics.Overlap(
      Sphere(2.0f), {0.0f, 0.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), QueryFilter{.triggers = true}, hits);

    ASSERT_EQ(hits.size(), 3u);
    EXPECT_EQ(hits[2].entity, trigger_entity);
    EXPECT_TRUE(hits[2].trigger);
  }

  TEST_F(JoltPhysicsSystemTest, FindsNothingWhereNothingIs)
  {
    CreateBody(crate_entity, BodyKind::Static, Box({1.0f, 1.0f, 1.0f}), {1.0f, 0.0f, 0.0f});
    std::vector<OverlapHit> hits = {OverlapHit{.entity = 99}};

    _physics.Overlap(Sphere(2.0f), {0.0f, 50.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), QueryFilter{}, hits);

    EXPECT_TRUE(hits.empty());
  }

  TEST_F(JoltPhysicsSystemTest, FindsWithAShapeThatIsTurned)
  {
    CreateBody(crate_entity, BodyKind::Static, Box({1.0f, 1.0f, 1.0f}), {0.0f, 0.0f, -4.0f});
    std::vector<OverlapHit> hits;
    const auto plank = Box({10.0f, 0.5f, 0.5f});

    // along the axis to the right, it reaches nothing
    _physics.Overlap(plank, {0.0f, 0.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), QueryFilter{}, hits);
    EXPECT_TRUE(hits.empty());

    // turned to the front, it does
    _physics.Overlap(plank, {0.0f, 0.0f, 0.0f}, Rotation{.yaw = 90.0f}.GetQuaternion(), QueryFilter{}, hits);
    EXPECT_EQ(hits.size(), 1u);
  }

  TEST_F(JoltPhysicsSystemTest, FindsABodyOfSeveralShapesOnce)
  {
    BodyInfo dumbbell;
    dumbbell.entity = crate_entity;
    dumbbell.kind = BodyKind::Static;
    dumbbell.shapes = {Sphere(0.5f), Sphere(0.5f)};
    dumbbell.shapes[0].position = {-1.0f, 0.0f, 0.0f};
    dumbbell.shapes[1].position = {1.0f, 0.0f, 0.0f};
    Create(dumbbell);
    std::vector<OverlapHit> hits;

    _physics.Overlap(Sphere(3.0f), {0.0f, 0.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), QueryFilter{}, hits);

    EXPECT_EQ(hits.size(), 1u);
  }

  TEST_F(JoltPhysicsSystemTest, RefusesToLookForOverlapsWithAShapeThatHasNoInside)
  {
    CreateBody(crate_entity, BodyKind::Static, Box({1.0f, 1.0f, 1.0f}), {0.0f, 0.0f, 0.0f});
    std::vector<OverlapHit> hits;

    _physics.Overlap(CubeOf(ShapeKind::Mesh), {0.0f, 0.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), QueryFilter{}, hits);

    EXPECT_TRUE(hits.empty());
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Error, "An overlap cannot be looked for with a mesh, which has no inside"));
  }

  TEST_F(JoltPhysicsSystemTest, FindsWhatACastShapeMeetsOnItsWay)
  {
    const auto floor = CreateFloor();
    const glm::quat upright{1.0f, 0.0f, 0.0f, 0.0f};

    ShapeCastHit hit;
    const bool found = _physics.CastShape(Sphere(0.5f), {0.0f, 5.0f, 0.0f}, upright, {0.0f, -5.0f, 0.0f},
                                          QueryFilter{}, hit);

    ASSERT_TRUE(found);
    EXPECT_EQ(hit.entity, floor_entity);
    EXPECT_EQ(hit.body, floor);
    EXPECT_FALSE(hit.trigger);

    // the sphere touches the floor once its middle is half a unit above it
    EXPECT_NEAR(hit.fraction, 0.45f, 1e-3f);
    EXPECT_NEAR(hit.distance, 4.5f, 1e-2f);
    EXPECT_NEAR(hit.point.y, 0.0f, 1e-2f);
    EXPECT_NEAR(hit.point.x, 0.0f, 1e-2f);
    EXPECT_NEAR(hit.normal.y, 1.0f, 1e-3f);
  }

  TEST_F(JoltPhysicsSystemTest, FindsWhatACastShapeMeetsWithItsSideAndARayWouldMiss)
  {
    // a post beside the way, which a line down the middle passes
    CreateBody(wall_entity, BodyKind::Static, Box({0.2f, 10.0f, 0.2f}), {0.8f, 0.0f, 0.0f});
    const glm::quat upright{1.0f, 0.0f, 0.0f, 0.0f};

    RayHit ray_hit;
    const Ray ray{.origin = {0.0f, 0.0f, 5.0f}, .direction = {0.0f, 0.0f, -1.0f}, .distance = 10.0f};
    EXPECT_FALSE(_physics.CastRay(ray, QueryFilter{}, ray_hit));

    ShapeCastHit hit;
    ASSERT_TRUE(_physics.CastShape(Box({2.0f, 1.0f, 1.0f}), {0.0f, 0.0f, 5.0f}, upright, {0.0f, 0.0f, -5.0f},
                                   QueryFilter{}, hit));
    EXPECT_EQ(hit.entity, wall_entity);
    EXPECT_NEAR(hit.fraction, 0.44f, 1e-2f);
    EXPECT_NEAR(hit.normal.z, 1.0f, 1e-3f);
  }

  TEST_F(JoltPhysicsSystemTest, MissesWithACastShapeWhatIsNotOnItsWay)
  {
    CreateBody(crate_entity, BodyKind::Static, Box({2.0f, 2.0f, 2.0f}), {0.0f, 0.0f, -10.0f});
    const glm::quat upright{1.0f, 0.0f, 0.0f, 0.0f};
    ShapeCastHit hit;

    const auto cast = [&](const glm::vec3 &from, const glm::vec3 &to)
    {
      return _physics.CastShape(Sphere(0.5f), from, upright, to, QueryFilter{}, hit);
    };

    // to the other side
    EXPECT_FALSE(cast({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10.0f}));
    // past it
    EXPECT_FALSE(cast({5.0f, 0.0f, 0.0f}, {5.0f, 0.0f, -20.0f}));
    // not far enough
    EXPECT_FALSE(cast({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -8.0f}));
    // nowhere
    EXPECT_FALSE(cast({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));

    EXPECT_EQ(hit.entity, neon::No_Entity);
  }

  TEST_F(JoltPhysicsSystemTest, FindsTheNearestOfWhatACastShapeMeets)
  {
    CreateBody(wall_entity, BodyKind::Static, Box({2.0f, 2.0f, 2.0f}), {0.0f, 0.0f, -20.0f});
    CreateBody(crate_entity, BodyKind::Dynamic, Sphere(1.0f), {0.0f, 0.0f, -10.0f},
               [](BodyInfo &info) { info.gravity_scale = 0.0f; });
    const glm::quat upright{1.0f, 0.0f, 0.0f, 0.0f};

    ShapeCastHit hit;
    ASSERT_TRUE(_physics.CastShape(
      Sphere(0.5f), {0.0f, 0.0f, 0.0f}, upright, {0.0f, 0.0f, -30.0f}, QueryFilter{}, hit));

    EXPECT_EQ(hit.entity, crate_entity);
    EXPECT_NEAR(hit.distance, 8.5f, 1e-2f);
  }

  TEST_F(JoltPhysicsSystemTest, FindsWhatACastShapeTouchesWhereItStarts)
  {
    CreateBody(crate_entity, BodyKind::Static, Box({2.0f, 2.0f, 2.0f}), {0.0f, 0.0f, 0.0f});
    const glm::quat upright{1.0f, 0.0f, 0.0f, 0.0f};

    ShapeCastHit hit;
    ASSERT_TRUE(_physics.CastShape(
      Sphere(0.5f), {0.0f, 0.0f, 0.0f}, upright, {0.0f, 0.0f, -5.0f}, QueryFilter{}, hit));

    EXPECT_EQ(hit.entity, crate_entity);
    EXPECT_EQ(hit.fraction, 0.0f);
  }

  TEST_F(JoltPhysicsSystemTest, CastsAShapeAsItIsTurned)
  {
    // a gap of 1.5 between two posts, which a box of 1 by 3 passes only on
    // its side
    CreateBody(wall_entity, BodyKind::Static, Box({1.0f, 10.0f, 1.0f}), {-1.25f, 0.0f, 0.0f});
    CreateBody(crate_entity, BodyKind::Static, Box({1.0f, 10.0f, 1.0f}), {1.25f, 0.0f, 0.0f});
    const auto box = Box({1.0f, 3.0f, 1.0f});
    ShapeCastHit hit;

    EXPECT_FALSE(_physics.CastShape(
      box, {0.0f, 0.0f, 5.0f}, glm::quat{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -5.0f}, QueryFilter{}, hit));
    EXPECT_TRUE(_physics.CastShape(
      box, {0.0f, 0.0f, 5.0f}, Rotation{.roll = 90.0f}.GetQuaternion(), {0.0f, 0.0f, -5.0f}, QueryFilter{}, hit));
  }

  TEST_F(JoltPhysicsSystemTest, CastsAShapeOnlyAtWhatIsInTheLayersItLooksFor)
  {
    CreateBody(crate_entity, BodyKind::Static, Box({2.0f, 2.0f, 2.0f}), {0.0f, 0.0f, -10.0f},
               [](BodyInfo &info) { info.layers = 0b01; });
    CreateBody(wall_entity, BodyKind::Static, Box({2.0f, 2.0f, 2.0f}), {0.0f, 0.0f, -20.0f},
               [](BodyInfo &info) { info.layers = 0b10; });
    const glm::quat upright{1.0f, 0.0f, 0.0f, 0.0f};
    const glm::vec3 from{0.0f};
    const glm::vec3 to{0.0f, 0.0f, -30.0f};
    ShapeCastHit hit;

    ASSERT_TRUE(_physics.CastShape(Sphere(0.5f), from, upright, to, QueryFilter{.mask = 0b10}, hit));
    EXPECT_EQ(hit.entity, wall_entity);

    ASSERT_TRUE(_physics.CastShape(Sphere(0.5f), from, upright, to, QueryFilter{.mask = 0b11}, hit));
    EXPECT_EQ(hit.entity, crate_entity);

    ASSERT_TRUE(_physics.CastShape(Sphere(0.5f), from, upright, to, QueryFilter{.ignore = crate_entity}, hit));
    EXPECT_EQ(hit.entity, wall_entity);

    EXPECT_FALSE(_physics.CastShape(Sphere(0.5f), from, upright, to, QueryFilter{.mask = 0b100}, hit));
  }

  TEST_F(JoltPhysicsSystemTest, CastsAShapeAtATriggerOnlyWhenAsked)
  {
    CreateTrigger(Box({2.0f, 2.0f, 2.0f}), {0.0f, 0.0f, -10.0f});
    const glm::quat upright{1.0f, 0.0f, 0.0f, 0.0f};
    ShapeCastHit hit;

    EXPECT_FALSE(_physics.CastShape(
      Sphere(0.5f), {0.0f, 0.0f, 0.0f}, upright, {0.0f, 0.0f, -20.0f}, QueryFilter{}, hit));
    ASSERT_TRUE(_physics.CastShape(
      Sphere(0.5f), {0.0f, 0.0f, 0.0f}, upright, {0.0f, 0.0f, -20.0f}, QueryFilter{.triggers = true}, hit));
    EXPECT_EQ(hit.entity, trigger_entity);
    EXPECT_TRUE(hit.trigger);
  }

  TEST_F(JoltPhysicsSystemTest, RefusesToCastAShapeThatHasNoInside)
  {
    CreateBody(crate_entity, BodyKind::Static, Box({1.0f, 1.0f, 1.0f}), {0.0f, 0.0f, 0.0f});
    const glm::quat upright{1.0f, 0.0f, 0.0f, 0.0f};
    ShapeCastHit hit;

    EXPECT_FALSE(_physics.CastShape(
      CubeOf(ShapeKind::Mesh), {0.0f, 0.0f, 5.0f}, upright, {0.0f, 0.0f, -5.0f}, QueryFilter{}, hit));
    EXPECT_TRUE(_logger->Contains(LogLevel::Error, "A shape cannot be cast with a mesh, which has no inside"));
  }

  // joints

  TEST_F(JoltPhysicsSystemTest, LetsADoorTurnOnItsHingeWhenItIsPushed)
  {
    const auto door = CreateDoor();

    // a push at the far edge
    _physics.AddImpulseAt(door, {0.0f, 0.0f, 1.0f}, {1.85f, 1.0f, 0.0f});
    Run(60);

    const auto state = StateOf(door);
    EXPECT_GT(AngleOf(door), 45.0f);
    EXPECT_LT(AngleOf(door), 175.0f);

    // the hinge stays on the post, and the door stays as long
    const auto hinge = state.position + state.rotation * glm::vec3(-0.85f, 0.0f, 0.0f);
    EXPECT_NEAR(hinge.x, 0.2f, 0.05f);
    EXPECT_NEAR(hinge.y, 1.0f, 0.05f);
    EXPECT_NEAR(hinge.z, 0.0f, 0.05f);
    EXPECT_NEAR(length(state.position - glm::vec3(0.2f, 1.0f, 0.0f)), 0.85f, 0.05f);

    // and it turned around the hinge alone
    EXPECT_NEAR(state.angular_velocity.x, 0.0f, 0.05f);
    EXPECT_NEAR(state.angular_velocity.z, 0.0f, 0.05f);
    EXPECT_EQ(_physics.GetJointCount(), 1u);
  }

  TEST_F(JoltPhysicsSystemTest, StopsADoorAtTheLimitsOfItsHinge)
  {
    const auto door = CreateDoor({-glm::quarter_pi<float>(), glm::quarter_pi<float>()});

    _physics.AddImpulseAt(door, {0.0f, 0.0f, 6.0f}, {1.85f, 1.0f, 0.0f});

    // it reaches the limit and is held there, within what one step of a
    // hard push gets past it before it is pulled back
    float widest = 0.0f;
    for (int i = 0; i < 120; i++)
    {
      Run(1);
      widest = std::max(widest, AngleOf(door));
    }

    EXPECT_GT(widest, 42.0f);
    EXPECT_LT(widest, 48.0f);
  }

  TEST_F(JoltPhysicsSystemTest, LetsAPendulumSwingOnAPointJoint)
  {
    const auto bob = CreateBody(ball_entity, BodyKind::Dynamic, Sphere(0.2f), {1.0f, 4.0f, 0.0f});
    const glm::vec3 hook{0.0f, 5.0f, 0.0f};
    Join(JointInfo{.kind = JointKind::Point, .body = bob, .anchor = hook});

    // half a swing takes 1.2 seconds on a string of this length
    float lowest = 4.0f;
    for (int i = 0; i < 72; i++)
    {
      Run(1);
      const auto state = StateOf(bob);
      lowest = std::min(lowest, state.position.y);

      // always as far from the hook as it hung
      EXPECT_NEAR(length(state.position - hook), std::sqrt(2.0f), 0.05f) << "step " << i;
    }

    // it swung down through the middle and up the other side
    const auto state = StateOf(bob);
    EXPECT_LT(lowest, 3.7f);
    EXPECT_LT(state.position.x, -0.5f);
  }

  TEST_F(JoltPhysicsSystemTest, LetsABodyFallOnARopeUntilTheRopeIsTaut)
  {
    // held right at the hook, on a rope of two
    const glm::vec3 hook{0.0f, 5.0f, 0.0f};
    const auto bob = CreateBody(ball_entity, BodyKind::Dynamic, Sphere(0.2f), {0.0f, 4.8f, 0.0f});
    Join(JointInfo{
      .kind = JointKind::Rope,
      .body = bob,
      .anchor = {0.0f, 4.8f, 0.0f},
      .other_anchor = hook,
      .length = 2.0f});

    // slack at first: it falls as if nothing held it
    Run(12);
    const float fallen = 4.8f - StateOf(bob).position.y;
    EXPECT_NEAR(fallen, 0.5f * 9.81f * 0.2f * 0.2f, 0.05f);

    // then the rope holds it, and it comes no further from the hook
    float furthest = 0.0f;
    for (int i = 0; i < 180; i++)
    {
      Run(1);
      furthest = std::max(furthest, length(StateOf(bob).position - hook));
    }
    EXPECT_LT(furthest, 2.1f);
    EXPECT_NEAR(StateOf(bob).position.y, 3.0f, 0.1f);
  }

  TEST_F(JoltPhysicsSystemTest, MakesARopeAsLongAsItsEndsAreApartWhenItHasNoLength)
  {
    // out to the side of the hook: it swings as a pendulum does
    const glm::vec3 hook{0.0f, 5.0f, 0.0f};
    const auto bob = CreateBody(ball_entity, BodyKind::Dynamic, Sphere(0.2f), {1.0f, 4.0f, 0.0f});
    Join(JointInfo{.kind = JointKind::Rope, .body = bob, .anchor = {1.0f, 4.0f, 0.0f}, .other_anchor = hook});

    float lowest = 4.0f;
    for (int i = 0; i < 72; i++)
    {
      Run(1);
      lowest = std::min(lowest, StateOf(bob).position.y);
      EXPECT_LT(length(StateOf(bob).position - hook), std::sqrt(2.0f) + 0.05f) << "step " << i;
    }
    EXPECT_LT(lowest, 3.7f);
  }

  TEST_F(JoltPhysicsSystemTest, LetsABodyMoveAlongASliderAlone)
  {
    const auto sled = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 0.0f, 0.0f},
                                 [](BodyInfo &info) { info.gravity_scale = 0.0f; });
    Join(JointInfo{
      .kind = JointKind::Slider,
      .body = sled,
      .axis = {1.0f, 0.0f, 0.0f},
      .has_limits = true,
      .limit_min = -2.0f,
      .limit_max = 2.0f
    });

    // a kick in every direction, beside the middle
    _physics.AddImpulseAt(sled, {5.0f, 5.0f, 5.0f}, {0.0f, 0.4f, 0.4f});
    Run(90);

    const auto state = StateOf(sled);
    EXPECT_NEAR(state.position.x, 2.0f, 0.05f);
    EXPECT_NEAR(state.position.y, 0.0f, 0.01f);
    EXPECT_NEAR(state.position.z, 0.0f, 0.01f);
    EXPECT_NEAR(std::abs(state.rotation.w), 1.0f, 1e-3f);
  }

  TEST_F(JoltPhysicsSystemTest, MovesTwoBodiesAsOneWithAFixedJoint)
  {
    const auto left = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f},
                                 [](BodyInfo &info) { info.gravity_scale = 0.0f; });
    const auto right = CreateBody(other_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {1.0f, 5.0f, 0.0f},
                                  [](BodyInfo &info) { info.gravity_scale = 0.0f; });
    Join(JointInfo{.kind = JointKind::Fixed, .body = right, .other = left, .anchor = {0.5f, 5.0f, 0.0f}});

    // a kick at the far side of the right one, which would turn it alone
    _physics.AddImpulseAt(right, {0.0f, 4.0f, 0.0f}, {1.4f, 5.0f, 0.0f});
    Run(60);

    // the pair moved and turned as one: still side by side, and both turned
    // the same way
    const auto first = StateOf(left);
    const auto second = StateOf(right);
    EXPECT_GT(first.position.y, 5.5f);
    EXPECT_GT(second.position.y, 5.5f);
    EXPECT_NEAR(length(second.position - first.position), 1.0f, 0.02f);
    EXPECT_NEAR(std::abs(dot(first.rotation, second.rotation)), 1.0f, 1e-3f);
    EXPECT_LT(std::abs(first.rotation.w), 0.999f);
  }

  TEST_F(JoltPhysicsSystemTest, AJointThatIsTurnedOffHoldsNothingAndIsKeptAndHoldsAgain)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});
    const auto joint = Join(JointInfo{.kind = JointKind::Fixed, .body = crate, .anchor = {0.0f, 5.0f, 0.0f}});
    Run(60);
    EXPECT_NEAR(StateOf(crate).position.y, 5.0f, 0.02f);

    _physics.SetJointEnabled(joint, false);
    _physics.SetJointEnabled(joint, false);
    Run(30);

    // it falls, and the joint is still there
    const float fallen = StateOf(crate).position.y;
    EXPECT_LT(fallen, 4.9f);
    EXPECT_TRUE(_physics.HasJoint(joint));
    EXPECT_EQ(_physics.GetJointCount(), 1u);

    // on again, it pulls the body back to where it holds it
    _physics.SetJointEnabled(joint, true);
    Run(120);
    EXPECT_GT(StateOf(crate).position.y, fallen);

    // and one that is not there is left alone
    _physics.SetJointEnabled(12345, false);
  }

  TEST_F(JoltPhysicsSystemTest, ACharacterThatIsTakenOutOfTheWorldIsKeptAndNothingFindsItUntilItIsPutBack)
  {
    CreateFloor();
    const auto player = CreateCharacter({0.0f, 1.0f, 0.0f});
    const Ray ray{.origin = {5.0f, 1.0f, 0.0f}, .direction = {-1.0f, 0.0f, 0.0f}, .distance = 100.0f};
    RayHit hit;
    const bool found_before = _physics.CastRay(ray, QueryFilter{}, hit);
    ASSERT_TRUE(found_before) << "a ray finds the body a character carries";

    _physics.SetCharacterInWorld(player, false);
    _physics.SetCharacterInWorld(player, false);

    EXPECT_FALSE(_physics.CastRay(ray, QueryFilter{}, hit));
    EXPECT_EQ(_physics.GetCharacterCount(), 1u) << "it is kept";

    _physics.SetCharacterInWorld(player, true);
    EXPECT_EQ(_physics.CastRay(ray, QueryFilter{}, hit), found_before);

    // and one that is not there is left alone
    _physics.SetCharacterInWorld(12345, false);
  }

  TEST_F(JoltPhysicsSystemTest, HoldsABodyUpAgainstGravityWithAJointToTheWorld)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});
    const auto joint = Join(JointInfo{.kind = JointKind::Fixed, .body = crate, .anchor = {0.0f, 5.0f, 0.0f}});
    Run(120);

    EXPECT_NEAR(StateOf(crate).position.y, 5.0f, 0.02f);

    // taken apart, it falls
    _physics.DestroyJoint(joint);
    EXPECT_FALSE(_physics.HasJoint(joint));
    EXPECT_EQ(_physics.GetJointCount(), 0u);
    Run(60);
    EXPECT_LT(StateOf(crate).position.y, 4.0f);
  }

  TEST_F(JoltPhysicsSystemTest, TakesAJointApartWithEitherOfItsBodies)
  {
    const auto left = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});
    const auto right = CreateBody(other_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {1.0f, 5.0f, 0.0f});
    const auto first = Join(JointInfo{.kind = JointKind::Fixed, .body = right, .other = left});
    const auto second = Join(JointInfo{.kind = JointKind::Point, .body = left});
    EXPECT_EQ(_physics.GetJointCount(), 2u);

    _physics.DestroyBody(right);
    EXPECT_FALSE(_physics.HasJoint(first));
    EXPECT_TRUE(_physics.HasJoint(second));

    _physics.DestroyBody(left);
    EXPECT_FALSE(_physics.HasJoint(second));
    EXPECT_EQ(_physics.GetJointCount(), 0u);

    // and nothing is left to go wrong
    Run(5);
    _physics.DestroyJoint(first);
  }

  TEST_F(JoltPhysicsSystemTest, RefusesAJointThatCannotHold)
  {
    const auto floor = CreateFloor();
    const auto wall = CreateBody(wall_entity, BodyKind::Static, Box({1.0f, 1.0f, 1.0f}), {0.0f, 3.0f, 0.0f});
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});
    CreateCharacter({3.0f, 1.0f, 0.0f});
    JointId joint = No_Joint;

    EXPECT_FALSE(_physics.CreateJoint(JointInfo{.body = 99}, joint, _error));
    EXPECT_EQ(_error, "the body is not known");

    EXPECT_FALSE(_physics.CreateJoint(JointInfo{.body = crate, .other = 99}, joint, _error));
    EXPECT_EQ(_error, "the other body is not known");

    EXPECT_FALSE(_physics.CreateJoint(JointInfo{.body = wall, .other = floor}, joint, _error));
    EXPECT_EQ(_error, "neither body is dynamic, so there is nothing for the joint to hold");

    EXPECT_FALSE(_physics.CreateJoint(JointInfo{.body = wall}, joint, _error));
    EXPECT_EQ(_error, "neither body is dynamic, so there is nothing for the joint to hold");

    // the character holds body 4
    EXPECT_FALSE(_physics.CreateJoint(JointInfo{.body = crate, .other = 4}, joint, _error));
    EXPECT_THAT(_error, HasSubstr("that of a character"));

    EXPECT_FALSE(_physics.CreateJoint(JointInfo{.kind = JointKind::Hinge, .body = crate, .axis = {}}, joint, _error));
    EXPECT_EQ(_error, "a hinge needs an axis, and this one has none");

    EXPECT_FALSE(_physics.CreateJoint(
      JointInfo{.kind = JointKind::Slider, .body = crate, .has_limits = true, .limit_min = 1.0f, .limit_max = 2.0f},
      joint, _error));
    EXPECT_THAT(_error, HasSubstr("the least is 0 or below"));

    EXPECT_EQ(joint, No_Joint);
    EXPECT_EQ(_physics.GetJointCount(), 0u);
  }

  TEST_F(JoltPhysicsSystemTest, GivesTheSameSwingTwice)
  {
    const auto swing = [](std::vector<BodyState> &states)
    {
      const auto logger = std::make_shared<RecordingLogger>();
      Jolt_PhysicsSystem physics{SettingsConfig{}, logger};
      physics.Initialize();
      std::string error;

      BodyInfo info;
      info.entity = ball_entity;
      info.shapes = {Sphere(0.2f)};
      info.position = {1.0f, 4.0f, 0.0f};
      BodyId bob = No_Body;
      EXPECT_TRUE(physics.CreateBody(info, bob, error)) << error;

      JointId joint = No_Joint;
      EXPECT_TRUE(physics.CreateJoint(
        JointInfo{.kind = JointKind::Point, .body = bob, .anchor = {0.0f, 5.0f, 0.0f}}, joint, error)) << error;

      for (int i = 0; i < 200; i++)
      {
        physics.Step(step);
        BodyState state;
        EXPECT_TRUE(physics.GetBodyState(bob, state));
        states.push_back(state);
      }
      physics.CleanUp();
    };

    std::vector<BodyState> first;
    std::vector<BodyState> second;
    swing(first);
    swing(second);

    ASSERT_EQ(first.size(), second.size());
    for (std::size_t i = 0; i < first.size(); i++)
    {
      EXPECT_EQ(first[i].position, second[i].position) << "step " << i;
      EXPECT_EQ(first[i].rotation, second[i].rotation) << "step " << i;
    }
  }

  TEST_F(JoltPhysicsSystemTest, KeepsADoorAtItsFrameWhenTheTwoAreNotToCollide)
  {
    const auto door = CreateDoorAtItsFrame(false);
    const auto before = StateOf(door);
    Run(60);

    // nothing pushed the door out of the frame, and the two never touched
    const auto after = StateOf(door);
    EXPECT_NEAR(length(after.position - before.position), 0.0f, 0.01f);
    EXPECT_NEAR(AngleOf(door), 0.0f, 1.0f);
    EXPECT_EQ(Count(PhysicsEventKind::Began, crate_entity, wall_entity), 0u);

    // it still swings on its hinge
    _physics.AddImpulseAt(door, {0.0f, 0.0f, 1.0f}, {1.65f, 1.0f, 0.0f});
    Run(60);
    EXPECT_GT(AngleOf(door), 30.0f);
  }

  TEST_F(JoltPhysicsSystemTest, LetsADoorAtItsFrameCollideWithItWhenTold)
  {
    (void) CreateDoorAtItsFrame(true);
    Run(60);

    // the two overlap, so they touch
    EXPECT_GT(Count(PhysicsEventKind::Began, crate_entity, wall_entity), 0u);
  }

  TEST_F(JoltPhysicsSystemTest, LetsTwoJoinedBodiesCollideWithEverythingElse)
  {
    (void) CreateFloor();
    const auto door = CreateDoorAtItsFrame(false);

    // a ball rolls into the door, which is kept apart from the frame alone
    (void) CreateBody(ball_entity, BodyKind::Dynamic, Sphere(0.3f), {1.2f, 0.3f, 3.0f},
                      [](BodyInfo &info) { info.linear_velocity = {0.0f, 0.0f, -6.0f}; });
    Run(90);

    EXPECT_GT(Count(PhysicsEventKind::Began, ball_entity, crate_entity), 0u);
    EXPECT_GT(Count(PhysicsEventKind::Began, ball_entity, floor_entity), 0u);
    EXPECT_GT(AngleOf(door), 10.0f);
  }

  TEST_F(JoltPhysicsSystemTest, LetsTheBodiesTouchAgainWhenTheJointIsGone)
  {
    (void) CreateDoorAtItsFrame(false);
    Run(30);
    EXPECT_EQ(Count(PhysicsEventKind::Began, crate_entity, wall_entity), 0u);

    _physics.DestroyJoint(_door_joint);
    Run(30);

    // the overlap is a touch now
    EXPECT_GT(Count(PhysicsEventKind::Began, crate_entity, wall_entity), 0u);
  }

  // motors and springs

  TEST_F(JoltPhysicsSystemTest, TurnsADoorWithAMotorAtItsVelocity)
  {
    const auto door = CreateDoor({}, [](JointInfo &hinge)
    {
      hinge.motor_velocity = 1.0f;
      hinge.motor_strength = 50.0f;
    });
    Run(60);

    // a radian per second, so a second turns it 57 degrees
    EXPECT_NEAR(AngleOf(door), 57.3f, 3.0f);
    EXPECT_NEAR(StateOf(door).angular_velocity.y, 1.0f, 0.05f);
    EXPECT_NEAR(StateOfDoor().velocity, 1.0f, 0.05f);
  }

  TEST_F(JoltPhysicsSystemTest, StopsAMotoredDoorAtItsLimit)
  {
    const auto door = CreateDoor({-glm::quarter_pi<float>(), glm::quarter_pi<float>()}, [](JointInfo &hinge)
    {
      hinge.motor_velocity = 2.0f;
      hinge.motor_strength = 50.0f;
    });
    Run(120);

    EXPECT_NEAR(AngleOf(door), 45.0f, 2.0f);
    EXPECT_NEAR(StateOfDoor().velocity, 0.0f, 0.1f);
  }

  TEST_F(JoltPhysicsSystemTest, HoldsADoorStillWithAMotorOfNoVelocity)
  {
    const auto door = CreateDoor({}, [](JointInfo &hinge)
    {
      hinge.motor_velocity = 0.0f;
      hinge.motor_strength = 100.0f;
    });

    // a push that would swing a free door wide
    _physics.AddImpulseAt(door, {0.0f, 0.0f, 1.0f}, {1.85f, 1.0f, 0.0f});
    Run(60);

    EXPECT_LT(AngleOf(door), 10.0f);
  }

  TEST_F(JoltPhysicsSystemTest, LetsAWeakMotorBeOverpowered)
  {
    const auto door = CreateDoor({}, [](JointInfo &hinge)
    {
      hinge.motor_velocity = 0.0f;
      hinge.motor_strength = 0.01f;
    });

    _physics.AddImpulseAt(door, {0.0f, 0.0f, 1.0f}, {1.85f, 1.0f, 0.0f});
    Run(60);

    EXPECT_GT(AngleOf(door), 45.0f);
  }

  TEST_F(JoltPhysicsSystemTest, PullsADoorBackToRestWithASpring)
  {
    const auto door = CreateDoor({}, [](JointInfo &hinge)
    {
      hinge.spring_stiffness = 20.0f;
      hinge.spring_damping = 4.0f;
    });

    _physics.AddImpulseAt(door, {0.0f, 0.0f, 2.0f}, {1.85f, 1.0f, 0.0f});
    Run(15);
    const float swung = AngleOf(door);
    EXPECT_GT(swung, 10.0f);

    Run(240);

    // back where it hung, and at rest
    EXPECT_LT(AngleOf(door), 2.0f);
    EXPECT_NEAR(StateOfDoor().position, 0.0f, 0.04f);
    EXPECT_NEAR(StateOfDoor().velocity, 0.0f, 0.1f);
  }

  TEST_F(JoltPhysicsSystemTest, DrivesASledWithAMotorAndPullsItBackWithASpring)
  {
    const auto sled = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 0.0f, 0.0f},
                                 [](BodyInfo &info) { info.gravity_scale = 0.0f; });
    const auto motored = Join(JointInfo{
      .kind = JointKind::Slider,
      .body = sled,
      .axis = {1.0f, 0.0f, 0.0f},
      .motor_velocity = 2.0f,
      .motor_strength = 100.0f
    });
    Run(30);

    JointState state;
    EXPECT_TRUE(_physics.GetJointState(motored, state));
    EXPECT_NEAR(state.position, 1.0f, 0.05f);
    EXPECT_NEAR(state.velocity, 2.0f, 0.05f);
    EXPECT_NEAR(StateOf(sled).position.x, 1.0f, 0.05f);

    // from where it is now, a spring pulls it back there
    _physics.DestroyJoint(motored);
    _physics.SetLinearVelocity(sled, {0.0f, 0.0f, 0.0f});
    const auto sprung = Join(JointInfo{
      .kind = JointKind::Slider,
      .body = sled,
      .anchor = StateOf(sled).position,
      .axis = {1.0f, 0.0f, 0.0f},
      .spring_stiffness = 30.0f,
      .spring_damping = 5.0f
    });
    _physics.AddImpulse(sled, {3.0f, 0.0f, 0.0f});
    Run(10);
    EXPECT_TRUE(_physics.GetJointState(sprung, state));
    EXPECT_GT(state.position, 0.1f);

    Run(240);
    EXPECT_TRUE(_physics.GetJointState(sprung, state));
    EXPECT_NEAR(state.position, 0.0f, 0.02f);
    EXPECT_NEAR(state.velocity, 0.0f, 0.05f);
  }

  TEST_F(JoltPhysicsSystemTest, RefusesAMotorOrASpringThatCannotBe)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});
    JointId joint = No_Joint;

    EXPECT_FALSE(_physics.CreateJoint(
      JointInfo{.kind = JointKind::Hinge, .body = crate, .motor_strength = 1.0f, .spring_stiffness = 1.0f},
      joint, _error));
    EXPECT_EQ(_error, "the joint has a motor and a spring, where it has one or the other");

    EXPECT_FALSE(_physics.CreateJoint(
      JointInfo{.kind = JointKind::Slider, .body = crate, .motor_strength = -1.0f}, joint, _error));
    EXPECT_EQ(_error, "the strength of the motor, the stiffness of the spring, and its damping are 0 or above");

    EXPECT_FALSE(_physics.CreateJoint(
      JointInfo{.kind = JointKind::Hinge, .body = crate, .spring_damping = NAN}, joint, _error));
    EXPECT_EQ(_error, "the motor or the spring is no number");

    // a fixed joint has no motor, and what is written for one is left alone
    EXPECT_TRUE(_physics.CreateJoint(
      JointInfo{.kind = JointKind::Fixed, .body = crate, .motor_strength = 1.0f, .spring_stiffness = 1.0f},
      joint, _error)) << _error;
    EXPECT_EQ(_physics.GetJointCount(), 1u);
  }

  // the state of a joint

  TEST_F(JoltPhysicsSystemTest, ReadsTheAngleOfATurningHinge)
  {
    const auto door = CreateDoor();
    _physics.AddImpulseAt(door, {0.0f, 0.0f, 1.0f}, {1.85f, 1.0f, 0.0f});
    Run(30);

    // the same angle the door was turned by, in radians, and the same
    // speed the door turns at around its axis
    const auto state = StateOfDoor();
    EXPECT_NEAR(glm::degrees(std::abs(state.position)), AngleOf(door), 0.5f);
    EXPECT_NEAR(state.velocity, StateOf(door).angular_velocity.y, 0.01f);
    EXPECT_GT(std::abs(state.position), 0.2f);
    EXPECT_GT(std::abs(state.velocity), 0.5f);

    // and the other way is the other sign
    _physics.AddImpulseAt(door, {0.0f, 0.0f, -3.0f}, {1.85f, 1.0f, 0.0f});
    Run(1);
    EXPECT_LT(StateOfDoor().velocity * state.velocity, 0.0f);
    Run(59);
    EXPECT_LT(StateOfDoor().position * state.position, 0.0f);
  }

  TEST_F(JoltPhysicsSystemTest, ReadsThePositionOfASlider)
  {
    const auto sled = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 0.0f, 0.0f},
                                 [](BodyInfo &info) { info.gravity_scale = 0.0f; });
    const auto joint = Join(JointInfo{
      .kind = JointKind::Slider,
      .body = sled,
      .axis = {-1.0f, 0.0f, 0.0f},
      .has_limits = true,
      .limit_min = -2.0f,
      .limit_max = 2.0f
    });

    _physics.SetLinearVelocity(sled, {-3.0f, 0.0f, 0.0f});
    Run(20);

    // a third of a second at 3 units per second along the axis, which
    // points the other way than x, less what the damping took
    JointState state;
    EXPECT_TRUE(_physics.GetJointState(joint, state));
    EXPECT_NEAR(state.position, 1.0f, 0.02f);
    EXPECT_NEAR(state.velocity, 3.0f, 0.1f);
    EXPECT_NEAR(state.velocity, -StateOf(sled).linear_velocity.x, 1e-4f);
    EXPECT_NEAR(StateOf(sled).position.x, -1.0f, 0.02f);
  }

  TEST_F(JoltPhysicsSystemTest, ReadsZerosForAJointWithoutAStateAndSaysSoOnce)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});
    const auto joint = Join(JointInfo{.kind = JointKind::Point, .body = crate, .anchor = {0.0f, 6.0f, 0.0f}});
    Run(30);

    JointState state{.position = 1.0f, .velocity = 1.0f};
    EXPECT_TRUE(_physics.GetJointState(joint, state));
    EXPECT_EQ(state.position, 0.0f);
    EXPECT_EQ(state.velocity, 0.0f);
    EXPECT_TRUE(_logger->Contains(
      LogLevel::Warn,
      "The state of joint 1 was asked for, and a point joint has none. Only a hinge has an angle and a slider a "
      "position, which read as 0 here"));

    EXPECT_TRUE(_physics.GetJointState(joint, state));
    EXPECT_EQ(_logger->Count(LogLevel::Warn), 1u);

    // and no joint is no state
    EXPECT_FALSE(_physics.GetJointState(99, state));
    _physics.DestroyJoint(joint);
    EXPECT_FALSE(_physics.GetJointState(joint, state));
  }

  // shapes that change while a body lives

  TEST_F(JoltPhysicsSystemTest, LetsABodyGrowSoThatItTouchesWhatItDidNot)
  {
    CreateTrigger(Box({1.0f, 1.0f, 1.0f}), {0.0f, 0.0f, 0.0f});
    const auto crate = CreateBody(crate_entity, BodyKind::Kinematic, Box({1.0f, 1.0f, 1.0f}), {3.0f, 0.0f, 0.0f});
    Run(5);
    EXPECT_EQ(Count(PhysicsEventKind::Began, trigger_entity, crate_entity), 0u);

    ASSERT_TRUE(_physics.SetShape(crate, {Box({8.0f, 1.0f, 1.0f})}, _error)) << _error;
    Run(5);

    EXPECT_EQ(Count(PhysicsEventKind::Began, trigger_entity, crate_entity), 1u);
    EXPECT_EQ(_physics.GetBodyCount(), 2u);

    // and shrinks away again
    ASSERT_TRUE(_physics.SetShape(crate, {Box({1.0f, 1.0f, 1.0f})}, _error)) << _error;
    Run(5);
    EXPECT_EQ(Count(PhysicsEventKind::Ended, trigger_entity, crate_entity), 1u);
  }

  TEST_F(JoltPhysicsSystemTest, LiftsADynamicBodyThatGrowsIntoTheFloor)
  {
    CreateFloor();
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 0.5f, 0.0f});
    Run(120);
    EXPECT_NEAR(StateOf(crate).position.y, 0.5f, 0.03f);

    ASSERT_TRUE(_physics.SetShape(crate, {Box({1.0f, 3.0f, 1.0f})}, _error)) << _error;
    Run(120);

    EXPECT_NEAR(StateOf(crate).position.y, 1.5f, 0.05f);
  }

  TEST_F(JoltPhysicsSystemTest, KeepsTheMassOfADynamicBodyWhoseShapeChanges)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f},
                                  [](BodyInfo &info)
                                  {
                                    info.gravity_scale = 0.0f;
                                    info.linear_damping = 0.0f;
                                    info.mass = 4.0f;
                                  });

    ASSERT_TRUE(_physics.SetShape(crate, {Box({10.0f, 10.0f, 10.0f})}, _error)) << _error;
    _physics.AddImpulse(crate, {8.0f, 0.0f, 0.0f});
    Run(1);

    // an impulse of 8 on a mass of 4 gives 2, whatever the size
    EXPECT_NEAR(StateOf(crate).linear_velocity.x, 2.0f, 1e-3f);
  }

  TEST_F(JoltPhysicsSystemTest, KeepsTheLockedAxesOfABodyWhoseShapeChanges)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f},
                                  [](BodyInfo &info)
                                  {
                                    info.gravity_scale = 0.0f;
                                    info.locked_rotation = neon::Axis_X | neon::Axis_Y | neon::Axis_Z;
                                  });

    ASSERT_TRUE(_physics.SetShape(crate, {Box({1.0f, 2.0f, 1.0f})}, _error)) << _error;
    _physics.AddImpulseAt(crate, {5.0f, 0.0f, 0.0f}, {0.0f, 5.9f, 0.0f});
    Run(10);

    EXPECT_EQ(StateOf(crate).angular_velocity, glm::vec3(0.0f));
  }

  TEST_F(JoltPhysicsSystemTest, RefusesShapesThatCannotBeOnTheBody)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});

    EXPECT_FALSE(_physics.SetShape(crate, {CubeOf(ShapeKind::Mesh)}, _error));
    EXPECT_THAT(_error, HasSubstr("a dynamic body cannot have a mesh"));

    EXPECT_FALSE(_physics.SetShape(crate, {Sphere(0.0f)}, _error));
    EXPECT_THAT(_error, HasSubstr("the sphere has a radius of 0"));

    EXPECT_FALSE(_physics.SetShape(crate, {}, _error));
    EXPECT_EQ(_error, "it has no shape");

    // it is as it was
    BodyState state;
    EXPECT_TRUE(_physics.GetBodyState(crate, state));
  }

  TEST_F(JoltPhysicsSystemTest, RefusesToChangeTheShapeOfWhatIsNotABody)
  {
    EXPECT_FALSE(_physics.SetShape(42, {Sphere(1.0f)}, _error));
    EXPECT_EQ(_error, "the body is not known");

    CreateCharacter({0.0f, 1.0f, 0.0f});
    EXPECT_FALSE(_physics.SetShape(1, {Sphere(1.0f)}, _error));
    EXPECT_THAT(_error, HasSubstr("that of a character"));
  }

  // shapes

  TEST_F(JoltPhysicsSystemTest, MakesEveryShape)
  {
    std::vector<ShapeInfo> shapes;
    for (int kind = 0; kind <= static_cast<int>(ShapeKind::Mesh); kind++)
    {
      ShapeInfo shape = CubeOf(ShapeKind::Mesh);
      shape.kind = static_cast<ShapeKind>(kind);
      shapes.push_back(shape);
    }

    for (const auto &shape : shapes)
    {
      BodyInfo info;
      info.kind = BodyKind::Static;
      info.shapes = {shape};
      BodyId body = No_Body;

      EXPECT_TRUE(_physics.CreateBody(info, body, _error)) << static_cast<int>(shape.kind) << ": " << _error;
      EXPECT_NE(body, No_Body);
    }

    EXPECT_EQ(_physics.GetBodyCount(), shapes.size());
  }

  TEST_F(JoltPhysicsSystemTest, LetsEveryShapeWithAnInsideRestOnTheFloor)
  {
    CreateFloor();

    struct Case
    {
      ShapeInfo shape;
      float lowest;
    };

    ShapeInfo cylinder;
    cylinder.kind = ShapeKind::Cylinder;
    cylinder.radius = 0.5f;
    cylinder.height = 1.0f;
    ShapeInfo tapered_capsule;
    tapered_capsule.kind = ShapeKind::TaperedCapsule;
    tapered_capsule.height = 2.0f;
    tapered_capsule.top_radius = 0.3f;
    tapered_capsule.bottom_radius = 0.6f;
    ShapeInfo cone;
    cone.kind = ShapeKind::TaperedCylinder;
    cone.height = 1.0f;
    cone.top_radius = 0.0f;
    cone.bottom_radius = 0.5f;

    // what is below the middle of each when it stands
    const std::vector<Case> cases = {
      {Box({1.0f, 2.0f, 1.0f}), 1.0f},
      {Sphere(0.5f), 0.5f},
      {cylinder, 0.5f},
      {tapered_capsule, 1.0f},
      {cone, 0.5f},
      {CubeOf(ShapeKind::ConvexHull), 0.5f}
    };

    std::vector<BodyId> bodies;
    for (std::size_t i = 0; i < cases.size(); i++)
    {
      bodies.push_back(CreateBody(
        100 + i, BodyKind::Dynamic, cases[i].shape,
        {-10.0f + 4.0f * static_cast<float>(i), cases[i].lowest + 0.5f, 0.0f}));
    }

    Run(300);

    for (std::size_t i = 0; i < cases.size(); i++)
    {
      // Jolt puts a body of a shape whose middle is not its middle of mass
      // where the shape says, so where the body is is where the shape is
      EXPECT_NEAR(StateOf(bodies[i]).position.y, cases[i].lowest, 0.05f) << "shape " << i;
    }
  }

  TEST_F(JoltPhysicsSystemTest, LetsABallRestOnAMeshThatDoesNotMove)
  {
    auto mesh = CubeOf(ShapeKind::Mesh);
    mesh.scale = {20.0f, 1.0f, 20.0f};
    CreateBody(floor_entity, BodyKind::Static, mesh, {0.0f, -0.5f, 0.0f});
    const auto ball = CreateBody(ball_entity, BodyKind::Dynamic, Sphere(0.5f), {3.0f, 3.0f, -2.0f});

    Run(300);

    EXPECT_NEAR(StateOf(ball).position.y, 0.5f, 0.03f);
  }

  TEST_F(JoltPhysicsSystemTest, LetsAHullFallOntoAMeshThatIsMoved)
  {
    auto mesh = CubeOf(ShapeKind::Mesh);
    mesh.scale = {20.0f, 1.0f, 20.0f};
    const auto lift = CreateBody(floor_entity, BodyKind::Kinematic, mesh, {0.0f, -0.5f, 0.0f});
    const auto rock = CreateBody(crate_entity, BodyKind::Dynamic, CubeOf(ShapeKind::ConvexHull), {0.0f, 2.0f, 0.0f},
                                 [](BodyInfo &info) { info.can_sleep = false; });
    Run(120);
    ASSERT_NEAR(StateOf(rock).position.y, 0.5f, 0.03f);

    // and is lifted with it
    for (int i = 1; i <= 60; i++)
    {
      _physics.MoveBody(lift, {0.0f, -0.5f + static_cast<float>(i) / 60.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), step);
      Run(1);
    }
    Run(60);

    EXPECT_NEAR(StateOf(rock).position.y, 1.5f, 0.05f);
  }

  TEST_F(JoltPhysicsSystemTest, RefusesAMeshOnADynamicBody)
  {
    BodyInfo info;
    info.kind = BodyKind::Dynamic;
    info.shapes = {CubeOf(ShapeKind::Mesh)};
    BodyId body = 99;

    EXPECT_FALSE(_physics.CreateBody(info, body, _error));

    EXPECT_EQ(body, No_Body);
    EXPECT_EQ(_error,
              "a dynamic body cannot have a mesh. A mesh is a surface without an inside, so it has no mass. "
              "Make the body static or kinematic, or give it a convex hull");
    EXPECT_EQ(_physics.GetBodyCount(), 0u);
  }

  TEST_F(JoltPhysicsSystemTest, RefusesAMeshAmongTheShapesOfADynamicBody)
  {
    BodyInfo info;
    info.kind = BodyKind::Dynamic;
    info.shapes = {Box({1.0f, 1.0f, 1.0f}), CubeOf(ShapeKind::Mesh)};
    BodyId body = No_Body;

    EXPECT_FALSE(_physics.CreateBody(info, body, _error));

    EXPECT_THAT(_error, HasSubstr("a dynamic body cannot have a mesh"));
  }

  TEST_F(JoltPhysicsSystemTest, TakesAMeshOnAStaticBodyAKinematicOneAndATrigger)
  {
    for (const auto kind : {BodyKind::Static, BodyKind::Kinematic})
    {
      BodyInfo info;
      info.kind = kind;
      info.shapes = {CubeOf(ShapeKind::Mesh)};
      BodyId body = No_Body;

      EXPECT_TRUE(_physics.CreateBody(info, body, _error)) << _error;
    }

    BodyInfo trigger;
    trigger.kind = BodyKind::Dynamic;
    trigger.trigger = true;
    trigger.shapes = {CubeOf(ShapeKind::Mesh)};
    BodyId body = No_Body;
    EXPECT_TRUE(_physics.CreateBody(trigger, body, _error)) << _error;
  }

  // shapes of a model, shared by the bodies whose colliders name it

  TEST_F(JoltPhysicsSystemTest, HoldsOneShapeForTheBodiesThatNameTheSameModelAtTheSameScale)
  {
    auto mesh = CubeOf(ShapeKind::Mesh);
    mesh.source = "assets://models/rock.glb";
    mesh.scale = {2.0f, 2.0f, 2.0f};

    const auto first = CreateBody(floor_entity, BodyKind::Static, mesh, {0.0f, 0.0f, 0.0f});
    const auto second = CreateBody(wall_entity, BodyKind::Static, mesh, {5.0f, 0.0f, 0.0f});

    EXPECT_EQ(_physics.GetSharedShapeCount(), 1u);

    // the shape goes when the last body that holds it goes
    _physics.DestroyBody(first);
    EXPECT_EQ(_physics.GetSharedShapeCount(), 1u);
    _physics.DestroyBody(second);
    EXPECT_EQ(_physics.GetSharedShapeCount(), 0u);
  }

  TEST_F(JoltPhysicsSystemTest, HoldsAShapeForEveryScaleAndKindAModelIsUsedAt)
  {
    auto mesh = CubeOf(ShapeKind::Mesh);
    mesh.source = "assets://models/rock.glb";
    auto larger = mesh;
    larger.scale = {2.0f, 2.0f, 2.0f};
    auto hull = CubeOf(ShapeKind::ConvexHull);
    hull.source = mesh.source;

    CreateBody(floor_entity, BodyKind::Static, mesh, {0.0f, 0.0f, 0.0f});
    CreateBody(wall_entity, BodyKind::Static, larger, {5.0f, 0.0f, 0.0f});
    CreateBody(crate_entity, BodyKind::Dynamic, hull, {0.0f, 5.0f, 0.0f});

    EXPECT_EQ(_physics.GetSharedShapeCount(), 3u);
  }

  TEST_F(JoltPhysicsSystemTest, SharesNoShapeWhosePointsAreNobodyElses)
  {
    // a mesh a Geometry built names no source, and a box is cheap to make
    CreateBody(floor_entity, BodyKind::Static, CubeOf(ShapeKind::Mesh), {0.0f, 0.0f, 0.0f});
    CreateBody(wall_entity, BodyKind::Static, CubeOf(ShapeKind::Mesh), {5.0f, 0.0f, 0.0f});
    CreateBody(crate_entity, BodyKind::Static, Box({1.0f, 1.0f, 1.0f}), {9.0f, 0.0f, 0.0f});

    EXPECT_EQ(_physics.GetSharedShapeCount(), 0u);
  }

  TEST_F(JoltPhysicsSystemTest, LetsGoOfASharedShapeThatABodyChangedAwayFrom)
  {
    auto hull = CubeOf(ShapeKind::ConvexHull);
    hull.source = "assets://models/rock.glb";

    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, hull, {0.0f, 5.0f, 0.0f});
    EXPECT_EQ(_physics.GetSharedShapeCount(), 1u);

    EXPECT_TRUE(_physics.SetShape(crate, {Box({1.0f, 1.0f, 1.0f})}, _error)) << _error;
    EXPECT_EQ(_physics.GetSharedShapeCount(), 0u);
  }

  TEST_F(JoltPhysicsSystemTest, RefusesAPlaneOnABodyThatMoves)
  {
    ShapeInfo plane;
    plane.kind = ShapeKind::Plane;

    for (const auto kind : {BodyKind::Kinematic, BodyKind::Dynamic})
    {
      BodyInfo info;
      info.kind = kind;
      info.shapes = {plane};
      BodyId body = No_Body;

      EXPECT_FALSE(_physics.CreateBody(info, body, _error));
      EXPECT_EQ(_error, "only a static body can have a plane, which has no end and cannot move");
    }
  }

  TEST_F(JoltPhysicsSystemTest, LetsABoxRestOnAPlane)
  {
    ShapeInfo plane;
    plane.kind = ShapeKind::Plane;
    CreateBody(floor_entity, BodyKind::Static, plane, {0.0f, 2.0f, 0.0f});
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {300.0f, 5.0f, -400.0f});

    Run(300);

    EXPECT_NEAR(StateOf(crate).position.y, 2.5f, 0.03f);
  }

  TEST_F(JoltPhysicsSystemTest, SaysWhatIsWrongWithAShape)
  {
    const auto problem_of = [&](const ShapeInfo &shape)
    {
      BodyInfo info;
      info.kind = BodyKind::Static;
      info.shapes = {shape};
      BodyId body = No_Body;
      _error.clear();
      EXPECT_FALSE(_physics.CreateBody(info, body, _error));
      return _error;
    };

    EXPECT_EQ(problem_of(Box({1.0f, 0.0f, 1.0f})),
              "the box has a size of [1, 0, 1], where every side has to be above 0");
    EXPECT_EQ(problem_of(Sphere(0.0f)), "the sphere has a radius of 0, which has to be above 0");
    EXPECT_EQ(problem_of(Capsule(0.5f, 0.5f)),
              "the capsule has a height of 0.5 and a radius of 0.5, where the height has to be at least "
              "twice the radius, and the radius above 0");

    ShapeInfo cylinder;
    cylinder.kind = ShapeKind::Cylinder;
    cylinder.height = -1.0f;
    EXPECT_EQ(problem_of(cylinder),
              "the cylinder has a height of -1 and a radius of 0.5, which both have to be above 0");

    ShapeInfo tapered_capsule;
    tapered_capsule.kind = ShapeKind::TaperedCapsule;
    tapered_capsule.height = 0.5f;
    EXPECT_EQ(problem_of(tapered_capsule),
              "the tapered capsule has a height of 0.5 and radii of 0.25 and 0.5, where the radii have to "
              "be above 0 and the height above both together");

    ShapeInfo tapered_cylinder;
    tapered_cylinder.kind = ShapeKind::TaperedCylinder;
    tapered_cylinder.top_radius = 0.0f;
    tapered_cylinder.bottom_radius = 0.0f;
    EXPECT_EQ(problem_of(tapered_cylinder),
              "the tapered cylinder has a height of 2 and radii of 0 and 0, where the height and one radius "
              "have to be above 0, and no radius below 0");

    ShapeInfo hull;
    hull.kind = ShapeKind::ConvexHull;
    hull.points = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    EXPECT_EQ(problem_of(hull), "the convex hull has 2 points, where at least 4 are needed");

    ShapeInfo mesh;
    mesh.kind = ShapeKind::Mesh;
    mesh.points = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    EXPECT_EQ(problem_of(mesh),
              "the mesh has 0 corners of triangles, where a number that 3 divides is needed, and at least 3");

    mesh.triangles = {0, 1, 7};
    EXPECT_EQ(problem_of(mesh), "triangle 1 of the mesh names a point that is not there. The mesh has 3 points");

    auto flat = Box({1.0f, 1.0f, 1.0f});
    flat.scale = {1.0f, 0.0f, 1.0f};
    EXPECT_EQ(problem_of(flat), "the box is scaled by [1, 0, 1], where no axis can be 0");

    auto egg = Sphere(0.5f);
    egg.scale = {1.0f, 2.0f, 1.0f};
    EXPECT_EQ(problem_of(egg),
              "the sphere is scaled by [1, 2, 1], and cannot be sized differently along these axes. A sphere "
              "needs the same for all three, and a capsule or a cylinder the same for x and z");

    EXPECT_EQ(_physics.GetBodyCount(), 0u);
  }

  TEST_F(JoltPhysicsSystemTest, SaysWhatIsWrongWithABody)
  {
    BodyInfo without_shape;
    BodyId body = No_Body;
    EXPECT_FALSE(_physics.CreateBody(without_shape, body, _error));
    EXPECT_EQ(_error, "it has no shape");

    BodyInfo without_mass;
    without_mass.shapes = {Box({1.0f, 1.0f, 1.0f})};
    without_mass.mass = 0.0f;
    EXPECT_FALSE(_physics.CreateBody(without_mass, body, _error));
    EXPECT_EQ(_error, "a dynamic body has a mass above 0, and this one has 0");

    BodyInfo nowhere;
    nowhere.shapes = {Box({1.0f, 1.0f, 1.0f})};
    nowhere.position = {0.0f, std::nanf(""), 0.0f};
    EXPECT_FALSE(_physics.CreateBody(nowhere, body, _error));
    EXPECT_EQ(_error, "where it is or how it moves is no number");

    // what does not move needs no mass
    without_mass.kind = BodyKind::Static;
    EXPECT_TRUE(_physics.CreateBody(without_mass, body, _error)) << _error;
  }

  TEST_F(JoltPhysicsSystemTest, SizesAShapeByItsScale)
  {
    CreateFloor();
    auto big = Box({1.0f, 1.0f, 1.0f});
    big.scale = {2.0f, 4.0f, 2.0f};
    auto round = Sphere(0.5f);
    round.scale = {3.0f, 3.0f, 3.0f};
    auto hull = CubeOf(ShapeKind::ConvexHull);
    hull.scale = {1.0f, 0.5f, 3.0f};
    const auto box = CreateBody(crate_entity, BodyKind::Dynamic, big, {-5.0f, 3.0f, 0.0f});
    const auto ball = CreateBody(ball_entity, BodyKind::Dynamic, round, {0.0f, 3.0f, 0.0f});
    const auto rock = CreateBody(other_entity, BodyKind::Dynamic, hull, {5.0f, 3.0f, 0.0f});

    Run(300);

    EXPECT_NEAR(StateOf(box).position.y, 2.0f, 0.03f);
    EXPECT_NEAR(StateOf(ball).position.y, 1.5f, 0.03f);
    EXPECT_NEAR(StateOf(rock).position.y, 0.25f, 0.03f);
  }

  TEST_F(JoltPhysicsSystemTest, PutsTheShapesOfABodyWhereTheySit)
  {
    CreateFloor();
    // a table: a top on four legs
    BodyInfo table;
    table.entity = crate_entity;
    table.shapes = {Box({2.0f, 0.2f, 2.0f})};
    table.shapes[0].position = {0.0f, 0.9f, 0.0f};
    for (const float x : {-0.9f, 0.9f})
    {
      for (const float z : {-0.9f, 0.9f})
      {
        auto leg = Box({0.2f, 0.8f, 0.2f});
        leg.position = {x, 0.4f, z};
        table.shapes.push_back(leg);
      }
    }
    table.position = {0.0f, 1.0f, 0.0f};
    const auto body = Create(table);
    // a ball that fits below the top
    const auto ball = CreateBody(ball_entity, BodyKind::Dynamic, Sphere(0.3f), {0.0f, 0.3f, 0.0f});
    // and one that lands on it
    const auto upper = CreateBody(other_entity, BodyKind::Dynamic, Sphere(0.3f), {0.0f, 4.0f, 0.0f});

    Run(300);

    // where the body is is below its legs
    EXPECT_NEAR(StateOf(body).position.y, 0.0f, 0.03f);
    EXPECT_NEAR(StateOf(ball).position.y, 0.3f, 0.03f);
    EXPECT_NEAR(StateOf(ball).position.x, 0.0f, 0.05f);
    EXPECT_NEAR(StateOf(upper).position.y, 1.3f, 0.05f);
  }

  // releasing

  TEST_F(JoltPhysicsSystemTest, DestroysABody)
  {
    CreateFloor();
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});
    ASSERT_EQ(_physics.GetBodyCount(), 2u);
    ASSERT_TRUE(_physics.HasBody(crate));

    _physics.DestroyBody(crate);

    EXPECT_FALSE(_physics.HasBody(crate));
    EXPECT_EQ(_physics.GetBodyCount(), 1u);
    BodyState state;
    EXPECT_FALSE(_physics.GetBodyState(crate, state));

    RayHit hit;
    EXPECT_FALSE(_physics.CastRay(Ray{.origin = {0.0f, 5.0f, 5.0f}}, QueryFilter{}, hit));
    EXPECT_NO_THROW(Run(10));
  }

  TEST_F(JoltPhysicsSystemTest, IgnoresABodyItDoesNotKnow)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});
    _physics.DestroyBody(crate);

    EXPECT_NO_THROW(_physics.DestroyBody(crate));
    EXPECT_NO_THROW(_physics.DestroyBody(No_Body));
    EXPECT_NO_THROW(_physics.DestroyBody(12345));
    EXPECT_NO_THROW(_physics.SetBodyPlace(crate, {0.0f, 0.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f)));
    EXPECT_NO_THROW(_physics.MoveBody(crate, {0.0f, 0.0f, 0.0f}, glm::quat(1.0f, 0.0f, 0.0f, 0.0f), step));
    EXPECT_NO_THROW(_physics.SetLinearVelocity(crate, {1.0f, 0.0f, 0.0f}));
    EXPECT_NO_THROW(_physics.SetAngularVelocity(crate, {1.0f, 0.0f, 0.0f}));
    EXPECT_NO_THROW(_physics.AddForce(crate, {1.0f, 0.0f, 0.0f}));
    EXPECT_NO_THROW(_physics.AddTorque(crate, {1.0f, 0.0f, 0.0f}));
    EXPECT_NO_THROW(_physics.AddImpulse(crate, {1.0f, 0.0f, 0.0f}));
    EXPECT_NO_THROW(_physics.AddImpulseAt(crate, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}));
    EXPECT_NO_THROW(_physics.DestroyCharacter(12345));
    EXPECT_EQ(_physics.GetBodyCount(), 0u);
  }

  TEST_F(JoltPhysicsSystemTest, NeverHandsOutTheIdOfABodyAgain)
  {
    const auto first = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});
    _physics.DestroyBody(first);

    const auto second = CreateBody(other_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});

    EXPECT_NE(second, first);
    EXPECT_FALSE(_physics.HasBody(first));
  }

  TEST_F(JoltPhysicsSystemTest, LetsWhatStoodOnADestroyedBodyFallOnceItIsWokenUp)
  {
    const auto floor = CreateFloor();
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 0.6f, 0.0f},
                                  [](BodyInfo &info) { info.can_sleep = false; });
    Run(60);

    _physics.DestroyBody(floor);
    Run(60);

    EXPECT_LT(StateOf(crate).position.y, -1.0f);
  }

  TEST_F(JoltPhysicsSystemTest, DestroysEverythingWhenItIsCleanedUp)
  {
    CreateFloor();
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 0.6f, 0.0f});
    CreateCharacter({5.0f, 1.0f, 0.0f});
    Run(30);

    _physics.CleanUp();

    EXPECT_EQ(_physics.GetBodyCount(), 0u);
    EXPECT_EQ(_physics.GetCharacterCount(), 0u);
    EXPECT_FALSE(_physics.HasBody(crate));
    EXPECT_TRUE(_physics.GetStepEvents().empty());
    EXPECT_TRUE(_physics.GetFrameEvents().empty());

    // more than once does no harm
    EXPECT_NO_THROW(_physics.CleanUp());
  }

  TEST_F(JoltPhysicsSystemTest, DoesNothingBeforeItIsInitialized)
  {
    Jolt_PhysicsSystem physics{SettingsConfig{}, _logger};
    BodyInfo info;
    info.shapes = {Box({1.0f, 1.0f, 1.0f})};
    BodyId body = 99;
    CharacterId character = 99;
    CharacterInfo character_info;
    character_info.shapes = {Capsule(0.5f, 2.0f)};

    EXPECT_FALSE(physics.CreateBody(info, body, _error));
    EXPECT_EQ(_error, "the physics is not initialized");
    EXPECT_EQ(body, No_Body);
    EXPECT_FALSE(physics.CreateCharacter(character_info, character, _error));
    EXPECT_EQ(character, No_Character);

    EXPECT_NO_THROW(physics.Step(step));
    EXPECT_NO_THROW(physics.DestroyBody(1));
    EXPECT_NO_THROW(physics.CleanUp());
    EXPECT_EQ(physics.GetBodyCount(), 0u);

    RayHit hit;
    EXPECT_FALSE(physics.CastRay(Ray{}, QueryFilter{}, hit));
    BodyState state;
    EXPECT_FALSE(physics.GetBodyState(1, state));
  }

  TEST_F(JoltPhysicsSystemTest, StartsAgainAfterItWasCleanedUp)
  {
    CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});
    _physics.CleanUp();

    _physics.Initialize();
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});
    Run(10);

    EXPECT_EQ(_physics.GetBodyCount(), 1u);
    EXPECT_LT(StateOf(crate).position.y, 5.0f);
  }

  TEST_F(JoltPhysicsSystemTest, RunsNextToAnotherPhysics)
  {
    Jolt_PhysicsSystem other{SettingsConfig{}, _logger};
    other.Initialize();
    BodyInfo info;
    info.shapes = {Box({1.0f, 1.0f, 1.0f})};
    info.position = {0.0f, 5.0f, 0.0f};
    BodyId in_other = No_Body;
    ASSERT_TRUE(other.CreateBody(info, in_other, _error));
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});

    Run(30);

    // what is stepped moves, and what is not does not
    BodyState state;
    ASSERT_TRUE(other.GetBodyState(in_other, state));
    EXPECT_EQ(state.position, glm::vec3(0.0f, 5.0f, 0.0f));
    EXPECT_LT(StateOf(crate).position.y, 5.0f);

    other.CleanUp();
    EXPECT_NO_THROW(Run(30));
  }

  // steps

  TEST_F(JoltPhysicsSystemTest, IgnoresAStepThatTakesNoTime)
  {
    const auto crate = CreateBody(crate_entity, BodyKind::Dynamic, Box({1.0f, 1.0f, 1.0f}), {0.0f, 5.0f, 0.0f});

    _physics.Step(0.0);
    _physics.Step(-1.0);
    _physics.Step(std::nan(""));

    EXPECT_EQ(StateOf(crate).position, glm::vec3(0.0f, 5.0f, 0.0f));
  }

  /// A world in which much happens: a pile that falls over, balls that
  /// roll, and a character that walks into it.
  std::vector<BodyState> RunAWorld(std::vector<PhysicsEvent> &events)
  {
    const auto logger = std::make_shared<RecordingLogger>();
    Jolt_PhysicsSystem physics{SettingsConfig{}, logger};
    physics.Initialize();
    std::string error;

    const auto create = [&](const BodyInfo &info)
    {
      BodyId body = No_Body;
      EXPECT_TRUE(physics.CreateBody(info, body, error)) << error;
      return body;
    };

    BodyInfo floor;
    floor.entity = floor_entity;
    floor.kind = BodyKind::Static;
    floor.shapes = {Box({100.0f, 1.0f, 100.0f})};
    floor.position = {0.0f, -0.5f, 0.0f};
    create(floor);

    BodyInfo slope;
    slope.entity = wall_entity;
    slope.kind = BodyKind::Static;
    slope.shapes = {CubeOf(ShapeKind::Mesh)};
    slope.shapes[0].scale = {6.0f, 0.5f, 6.0f};
    slope.position = {-6.0f, 1.0f, 0.0f};
    slope.rotation = Rotation{.roll = -25.0f}.GetQuaternion();
    create(slope);

    BodyInfo trigger;
    trigger.entity = trigger_entity;
    trigger.kind = BodyKind::Kinematic;
    trigger.trigger = true;
    trigger.shapes = {Box({4.0f, 4.0f, 4.0f})};
    trigger.position = {0.0f, 2.0f, 0.0f};
    create(trigger);

    std::vector<BodyId> bodies;
    for (int i = 0; i < 12; i++)
    {
      BodyInfo crate;
      crate.entity = 100 + static_cast<Entity>(i);
      crate.shapes = {i % 3 == 0 ? CubeOf(ShapeKind::ConvexHull) : Box({1.0f, 1.0f, 1.0f})};
      // a pile that leans
      crate.position = {0.15f * static_cast<float>(i), 0.5f + 1.01f * static_cast<float>(i), 0.0f};
      crate.rotation = Rotation{.yaw = 7.0f * static_cast<float>(i)}.GetQuaternion();
      bodies.push_back(create(crate));
    }
    for (int i = 0; i < 6; i++)
    {
      BodyInfo ball;
      ball.entity = 200 + static_cast<Entity>(i);
      ball.shapes = {Sphere(0.4f)};
      ball.position = {-7.0f, 4.0f + static_cast<float>(i), -2.0f + 0.8f * static_cast<float>(i)};
      ball.bounce = 0.4f;
      bodies.push_back(create(ball));
    }

    CharacterInfo player;
    player.entity = player_entity;
    player.shapes = {Capsule(0.5f, 2.0f)};
    player.position = {8.0f, 1.1f, 0.3f};
    CharacterId character = No_Character;
    EXPECT_TRUE(physics.CreateCharacter(player, character, error)) << error;

    for (int i = 0; i < 300; i++)
    {
      CharacterState moved;
      EXPECT_TRUE(physics.MoveCharacter(character, {-2.0f, -1.0f, 0.0f}, step, moved));
      physics.Step(step);

      const auto &reported = physics.GetStepEvents();
      events.insert(events.end(), reported.begin(), reported.end());
    }

    std::vector<BodyState> states;
    for (const auto body : bodies)
    {
      BodyState state;
      EXPECT_TRUE(physics.GetBodyState(body, state));
      states.push_back(state);
    }

    physics.CleanUp();
    return states;
  }

  TEST_F(JoltPhysicsSystemTest, GivesTheSameResultForTheSameInput)
  {
    std::vector<PhysicsEvent> first_events;
    std::vector<PhysicsEvent> second_events;

    const auto first = RunAWorld(first_events);
    const auto second = RunAWorld(second_events);

    ASSERT_EQ(first.size(), second.size());
    for (std::size_t i = 0; i < first.size(); i++)
    {
      // the same, and not merely close
      EXPECT_EQ(first[i].position, second[i].position) << "body " << i;
      EXPECT_EQ(first[i].rotation, second[i].rotation) << "body " << i;
      EXPECT_EQ(first[i].linear_velocity, second[i].linear_velocity) << "body " << i;
      EXPECT_EQ(first[i].angular_velocity, second[i].angular_velocity) << "body " << i;
      EXPECT_EQ(first[i].active, second[i].active) << "body " << i;
    }

    // something happened
    EXPECT_GT(first_events.size(), 20u);
    EXPECT_NE(first[11].position.x, 0.15f * 11.0f);

    ASSERT_EQ(first_events.size(), second_events.size());
    for (std::size_t i = 0; i < first_events.size(); i++)
    {
      EXPECT_EQ(first_events[i].kind, second_events[i].kind) << "event " << i;
      EXPECT_EQ(first_events[i].first, second_events[i].first) << "event " << i;
      EXPECT_EQ(first_events[i].second, second_events[i].second) << "event " << i;
      EXPECT_EQ(first_events[i].trigger, second_events[i].trigger) << "event " << i;
      EXPECT_EQ(first_events[i].point, second_events[i].point) << "event " << i;
      EXPECT_EQ(first_events[i].normal, second_events[i].normal) << "event " << i;
    }
  }
}
