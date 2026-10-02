#ifndef JOINT_INFO_HPP
#define JOINT_INFO_HPP

#include <cstdint>

#include <glm/glm.hpp>

#include "joint-kind.hpp"
#include "physics-types.hpp"

namespace neon
{
  /// Names a joint, as handed out when it was created.
  using JointId = std::uint32_t;

  // ReSharper disable once CppInconsistentNaming
  inline constexpr JointId No_Joint = 0;

  /// What a joint between two bodies is made from. Everything is in the
  /// space of the world, as the bodies are when the joint is made.
  struct JointInfo
  {
    JointKind kind = JointKind::Fixed;

    /// The body the joint belongs to. At least one of the two is dynamic.
    BodyId body = No_Body;

    /// The body it is joined to, or No_Body for the world itself, which
    /// holds it where it is.
    BodyId other = No_Body;

    /// Where the joint sits.
    glm::vec3 anchor{0.0f};

    /// Of a hinge, what it turns around. Of a slider, what it moves along.
    glm::vec3 axis{0.0f, 1.0f, 0.0f};

    /// Whether the hinge or the slider stops somewhere.
    bool has_limits = false;

    /// How far `body` may turn around the axis, in radians, or move along
    /// it, in units, from where it is when the joint is made. The least is
    /// 0 or below, the most 0 or above.
    float limit_min = 0.0f;
    float limit_max = 0.0f;
  };
} // neon

#endif //JOINT_INFO_HPP
