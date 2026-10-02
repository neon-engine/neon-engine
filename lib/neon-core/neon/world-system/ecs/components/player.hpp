#ifndef PLAYER_HPP
#define PLAYER_HPP

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
  /// start by writing them, and nothing is kept here that a file does not
  /// hold.
  struct Player
  {
    /// Metres per second along the ground while `run` is not down.
    float walk_speed = 4.0f;

    /// Metres per second along the ground while `run` is down.
    float run_speed = 6.0f;

    /// Metres per second upward that a jump starts with.
    float jump_speed = 5.0f;

    /// Radians the view turns for every pixel of `look`.
    float look_speed = 0.0025f;

    /// Metres from the feet to the eyes, where the camera is put.
    float eye_height = 1.6f;

    /// Degrees the view can turn up or down, below 90 so that the camera
    /// never looks straight along its own up.
    float max_pitch = 89.0f;
  };

  inline void Describe(TypeBuilder<Player> &type)
  {
    type.Named("Player", "Makes the entity the player, seen from the first person and driven by the input");

    type.Field("walk_speed", &Player::walk_speed)
        .AtLeast(0)
        .Describe("Metres per second along the ground");

    type.Field("run_speed", &Player::run_speed)
        .AtLeast(0)
        .Describe("Metres per second along the ground while run is down");

    type.Field("jump_speed", &Player::jump_speed)
        .AtLeast(0)
        .Describe("Metres per second upward that a jump starts with");

    type.Field("look_speed", &Player::look_speed)
        .AtLeast(0)
        .Describe("Radians the view turns for every pixel of look");

    type.Field("eye_height", &Player::eye_height)
        .Describe("Metres from the feet to the eyes, where the camera is put");

    type.Field("max_pitch", &Player::max_pitch)
        .AtLeast(0)
        .AtMost(90)
        .Unit("degrees")
        .Describe("How far the view can turn up or down");
  }
} // neon

#endif //PLAYER_HPP
