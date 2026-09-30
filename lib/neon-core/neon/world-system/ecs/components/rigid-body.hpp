#ifndef RIGID_BODY_HPP
#define RIGID_BODY_HPP

#include <cstdint>

#include <glm/glm.hpp>

#include <neon/physics/physics-types.hpp>

namespace neon
{
  /// Makes an entity a body of the physics. Its shape is what the Collider
  /// of the entity says, together with those of the entities below it.
  ///
  /// A dynamic body is moved by the simulation, which writes where it is
  /// into the Transform of the entity in every step. A kinematic body and a
  /// static one are where their Transform puts them. Moving the Transform of
  /// a kinematic body pushes what is in the way. Moving that of a dynamic
  /// body puts it there at once.
  ///
  /// The values are read when the body is created. After that, the physics
  /// follows the two velocities and nothing else.
  struct RigidBody
  {
    BodyKind kind = BodyKind::Dynamic;

    /// Of a dynamic body.
    float mass = 1.0f;

    /// 0 slides without end, 1 grips.
    float friction = 0.5f;

    /// How much of its speed the body keeps when it bounces, from 0 to 1.
    float bounce = 0.0f;

    /// How fast motion and turning die down by themselves.
    float linear_damping = 0.05f;
    float angular_damping = 0.05f;

    /// 0 floats, 1 falls as everything does.
    float gravity_scale = 1.0f;

    /// Units per second. The physics writes it in every step. What a game
    /// writes here is what the body moves with from the next step on.
    glm::vec3 linear_velocity{0.0f};

    /// Degrees per second around each axis. Written and read as the linear
    /// velocity is.
    glm::vec3 angular_velocity{0.0f};

    /// Looks for hits along the whole way of a step. For what is small and
    /// fast.
    bool continuous = false;

    /// A body that came to rest stops being simulated until something
    /// touches it.
    bool can_sleep = true;

    /// The layers the body is in, and the layers it looks for. One bit for
    /// each of the 32 layers, the lowest for layer 1.
    std::uint32_t layers = 1;
    std::uint32_t mask = 1;

    /// What the physics knows the entity as. Filled in by the engine when
    /// the body is created. No_Body until then.
    BodyId body = No_Body;

    /// Set by the engine when the body could not be created, which was said
    /// in the log. It is not tried again.
    bool failed = false;
  };
} // neon

#endif //RIGID_BODY_HPP
