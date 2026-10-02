#ifndef RIGID_BODY_HPP
#define RIGID_BODY_HPP

#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <neon/physics/physics-types.hpp>
#include <neon/reflection/type-builder.hpp>

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

    /// The axes of the world a dynamic body cannot move along, and those it
    /// cannot turn around, each a list of `x`, `y`, and `z`. A crate that
    /// cannot tip over has `lock_rotation: [x, y, z]`.
    std::vector<std::string> lock_position;
    std::vector<std::string> lock_rotation;

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

  /// What the engine fills in, the body and whether it failed, is not
  /// described. Neither is written in a file.
  inline void Describe(TypeBuilder<RigidBody> &type)
  {
    type.Named("RigidBody", "Makes an entity a body of the physics");

    // the kind is what a body is, so it is written even when it is the
    // default
    type.Choice("kind", &RigidBody::kind, {"static", "kinematic", "dynamic"})
        .AlwaysWritten()
        .Describe("Moved by the simulation, by its Transform, or not at all");

    type.Field("mass", &RigidBody::mass)
        .Above(0)
        .Describe("Of a dynamic body");

    type.Field("friction", &RigidBody::friction)
        .AtLeast(0)
        .Describe("0 slides without end, 1 grips");

    type.Field("bounce", &RigidBody::bounce)
        .AtLeast(0)
        .AtMost(1)
        .Describe("How much of its speed the body keeps when it bounces");

    type.Field("linear_damping", &RigidBody::linear_damping)
        .AtLeast(0)
        .Describe("How fast motion dies down by itself");

    type.Field("angular_damping", &RigidBody::angular_damping)
        .AtLeast(0)
        .Describe("How fast turning dies down by itself");

    type.Field("gravity_scale", &RigidBody::gravity_scale)
        .Describe("0 floats, 1 falls as everything does");

    type.Field("linear_velocity", &RigidBody::linear_velocity)
        .Describe("Units per second");

    type.Field("angular_velocity", &RigidBody::angular_velocity)
        .Describe("Degrees per second around each axis");

    type.Field("continuous", &RigidBody::continuous)
        .Describe("Whether hits are looked for along the whole way of a step, for what is small and fast");

    type.Field("can_sleep", &RigidBody::can_sleep)
        .Describe("Whether a body that came to rest stops being simulated until something touches it");

    type.Field("lock_position", &RigidBody::lock_position)
        .Describe("The axes of the world a dynamic body cannot move along: x, y, or z");

    type.Field("lock_rotation", &RigidBody::lock_rotation)
        .Describe("The axes of the world a dynamic body cannot turn around: x, y, or z");

    type.Layers("layers", &RigidBody::layers)
        .Describe("The layers the body is in");

    type.Layers("mask", &RigidBody::mask)
        .Describe("The layers the body looks for");

    // a word that is no axis would otherwise lock nothing without a word
    const auto axes = [](const std::string &name, std::vector<std::string> RigidBody::*axes)
    {
      return [name, axes](const RigidBody &body, const std::string &where) -> std::string
      {
        for (const auto &axis : body.*axes)
        {
          if (axis != "x" && axis != "y" && axis != "z")
          {
            return std::format("'{}' of {} holds '{}', where x, y, or z was expected", name, where, axis);
          }
        }
        return {};
      };
    };

    type.Rule("lock_position", axes("lock_position", &RigidBody::lock_position));
    type.Rule("lock_rotation", axes("lock_rotation", &RigidBody::lock_rotation));
  }
} // neon

#endif //RIGID_BODY_HPP
