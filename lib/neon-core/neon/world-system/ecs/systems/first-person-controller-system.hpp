#ifndef FIRST_PERSON_CONTROLLER_SYSTEM_HPP
#define FIRST_PERSON_CONTROLLER_SYSTEM_HPP

#include <neon/input/input-context.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Drives every entity that carries a FirstPersonController and a CharacterBody by the
  /// input, and places the camera below it at the eyes.
  ///
  /// Once a frame it turns the body by `look`, sets the velocity of the
  /// body from `move` in the direction the body faces, at the walking or
  /// the running speed, and starts a jump when `jump` is pressed while the
  /// body stands on the ground. In the air the body keeps the velocity it
  /// left the ground with, and `move` steers it by `air_control`. The
  /// physics moves the body by that in the steps that follow, and stops it
  /// at walls. The camera, a child of the body with a Camera, is lifted to
  /// `eye_height` above the feet, moved by `camera_offset`, and pitched by
  /// `look`, within `max_pitch`. A step the physics took the body up or
  /// down is seen over a few frames: the eyes lag behind it and catch up
  /// at `step_smoothing`.
  class FirstPersonControllerSystem final : public EntitySystem
  {
    InputContext *_input_context;
    QueryId _players = 0;
    QueryId _cameras = 0;

  public:
    explicit FirstPersonControllerSystem(InputContext *input_context);

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //FIRST_PERSON_CONTROLLER_SYSTEM_HPP
