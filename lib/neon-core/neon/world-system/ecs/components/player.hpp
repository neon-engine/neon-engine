#ifndef PLAYER_HPP
#define PLAYER_HPP

#include <glm/glm.hpp>

#include <neon/reflection/type-builder.hpp>

namespace neon
{
  /// Makes the entity the player, seen from the first person and driven by
  /// the input: `move` walks it, `look` turns it, `jump` and `run` do what
  /// they say. The entity carries a CharacterBody, which the physics moves
  /// by what this sets, and a child with a Camera, which is lifted to the
  /// eyes and pitched by the look. The body turns with the yaw alone, so
  /// that its capsule stays upright.
  ///
  /// The yaw lives in the rotation of the entity's Transform and the pitch
  /// in that of the camera, so a scene sets where the player looks at the
  /// start by writing them. What the component keeps of its own is the
  /// motion between frames, which the engine writes and no file holds: the
  /// velocity it left the ground with, and how far the eyes lag behind a
  /// step.
  struct Player
  {
    /// Meters per second along the ground while `run` is not down.
    float walk_speed = 4.0f;

    /// Meters per second along the ground while `run` is down.
    float run_speed = 6.0f;

    /// Meters per second upward that a jump starts with.
    float jump_speed = 5.0f;

    /// How much the keys steer the body while it is in the air, from 0 to
    /// 1. The body keeps the velocity it left the ground with, and the keys
    /// pull it by this much towards what they ask for: 0 keeps the take-off
    /// velocity whatever is pressed, 1 steers as on the ground.
    float air_control = 0.3f;

    /// Radians the view turns for every pixel of `look`.
    float look_speed = 0.0025f;

    /// Meters from the feet to the eyes, where the camera is put.
    float eye_height = 1.6f;

    /// Meters the camera is moved from the eyes, to the right, up, and back
    /// in the frame of the entity, so that a scene leans the view or looks
    /// at the player from behind.
    glm::vec3 camera_offset{0.0f};

    /// How quickly the eyes catch up with a step, per second: a step up of
    /// `step_height` lifts the body at once, and the eyes glide up after it
    /// over a few frames. 0 lifts the eyes with the body.
    float step_smoothing = 10.0f;

    /// Degrees the view can turn up or down, below 90 so that the camera
    /// never looks straight along its own up.
    float max_pitch = 89.0f;

    /// What `move` last asked for on the ground, which the body keeps in
    /// the air. Written by the engine.
    glm::vec3 ground_velocity{0.0f};

    /// Meters the eyes lag behind where the last step put them, below for
    /// a step up and above for a step down. Written by the engine.
    float eye_glide = 0.0f;

    /// Where the body stood in the frame before, and whether on the floor,
    /// so that a step is seen as the difference. Written by the engine.
    float last_height = 0.0f;
    bool was_on_floor = false;
  };

  /// The motion between frames is written by the engine and not described.
  inline void Describe(TypeBuilder<Player> &type)
  {
    type.Named("Player", "Makes the entity the player, seen from the first person and driven by the input");

    type.Field("walk_speed", &Player::walk_speed)
        .AtLeast(0)
        .Describe("Meters per second along the ground");

    type.Field("run_speed", &Player::run_speed)
        .AtLeast(0)
        .Describe("Meters per second along the ground while run is down");

    type.Field("jump_speed", &Player::jump_speed)
        .AtLeast(0)
        .Describe("Meters per second upward that a jump starts with");

    type.Field("air_control", &Player::air_control)
        .AtLeast(0)
        .AtMost(1)
        .Describe("How much the keys steer the body in the air: 0 keeps the take-off velocity, 1 steers as on the ground");

    type.Field("look_speed", &Player::look_speed)
        .AtLeast(0)
        .Describe("Radians the view turns for every pixel of look");

    type.Field("eye_height", &Player::eye_height)
        .Describe("Meters from the feet to the eyes, where the camera is put");

    type.Field("camera_offset", &Player::camera_offset)
        .Describe("Meters the camera is moved from the eyes, to the right, up, and back in the frame of the entity");

    type.Field("step_smoothing", &Player::step_smoothing)
        .AtLeast(0)
        .Describe("How quickly the eyes catch up with a step, per second. 0 lifts them with the body");

    type.Field("max_pitch", &Player::max_pitch)
        .AtLeast(0)
        .AtMost(90)
        .Unit("degrees")
        .Describe("How far the view can turn up or down");
  }
} // neon

#endif //PLAYER_HPP
