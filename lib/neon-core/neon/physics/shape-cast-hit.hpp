#ifndef SHAPE_CAST_HIT_HPP
#define SHAPE_CAST_HIT_HPP

#include <glm/glm.hpp>

#include "physics-types.hpp"

namespace neon
{
  /// The first body a shape meets when it is moved along a way. See
  /// PhysicsContext::CastShape().
  struct ShapeCastHit
  {
    Entity entity = No_Entity;
    BodyId body = No_Body;
    bool trigger = false;

    /// Where the shape touches what it hit, on the surface of what it hit.
    glm::vec3 point{0.0f};

    /// Points away from what was hit.
    glm::vec3 normal{0.0f};

    /// How far along the way the shape got before it touched, from 0 at the
    /// start to 1 at the end. 0 when the shape touched already where it
    /// started.
    float fraction = 0.0f;

    /// The same as a length.
    float distance = 0.0f;
  };
} // neon

#endif //SHAPE_CAST_HIT_HPP
