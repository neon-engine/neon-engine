#ifndef PLAYER_MOVEMENT_HPP
#define PLAYER_MOVEMENT_HPP

#include <neon/input/input-context.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Drives every entity that carries a Player and a CharacterBody by the
  /// input, and places the camera below it at the eyes.
  ///
  /// Once a frame it turns the body by `look`, sets the velocity of the
  /// body from `move` in the direction the body faces, at the walking or
  /// the running speed, and starts a jump when `jump` is pressed while the
  /// body stands on the ground. The physics moves the body by that in the
  /// steps that follow, and stops it at walls. The camera, a child of the
  /// body with a Camera, is lifted to `eye_height` above the feet and
  /// pitched by `look`, within `max_pitch`.
  class PlayerMovement final : public EntitySystem
  {
    InputContext *_input_context;
    QueryId _players = 0;
    QueryId _cameras = 0;

  public:
    explicit PlayerMovement(InputContext *input_context);

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //PLAYER_MOVEMENT_HPP
