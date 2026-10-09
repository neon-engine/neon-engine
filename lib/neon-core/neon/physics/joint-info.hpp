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

    /// Of a rope, where it is held on `other`, or in the world; `anchor` is
    /// its end on `body`.
    glm::vec3 other_anchor{0.0f};

    /// Of a rope, how far its two ends may be apart. 0 or below for as far
    /// as they are when the joint is made.
    float length = 0.0f;

    /// Of a hinge, what it turns around. Of a slider, what it moves along.
    glm::vec3 axis{0.0f, 1.0f, 0.0f};

    /// Whether the hinge or the slider stops somewhere.
    bool has_limits = false;

    /// How far `body` may turn around the axis, in radians, or move along
    /// it, in units, from where it is when the joint is made. The least is
    /// 0 or below, the most 0 or above.
    float limit_min = 0.0f;
    float limit_max = 0.0f;

    /// Whether the two bodies collide with each other. A door that is
    /// hinged to its frame does not, so the hinge can sit at the frame.
    bool collide_with_other = true;

    /// Of a hinge or a slider, a motor that drives it: how fast, in
    /// radians per second around the axis or units per second along it,
    /// and with how much at most, in newton meters or newtons. There is no
    /// motor while the strength is 0. A motor with a velocity of 0 holds
    /// the joint where it is, as far as its strength reaches.
    float motor_velocity = 0.0f;
    float motor_strength = 0.0f;

    /// Of a hinge or a slider, a spring that pulls it back to where it was
    /// made. The stiffness is the torque in newton meters for every radian
    /// it is turned, or the force in newtons for every unit it is moved,
    /// and the damping is the same against its speed. There is no spring
    /// while the stiffness is 0. A joint has a motor or a spring, not both.
    float spring_stiffness = 0.0f;
    float spring_damping = 0.0f;
  };
} // neon

#endif //JOINT_INFO_HPP
