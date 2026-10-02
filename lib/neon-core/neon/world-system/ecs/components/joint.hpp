#ifndef JOINT_HPP
#define JOINT_HPP

#include <cstddef>
#include <format>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include <neon/physics/joint-info.hpp>
#include <neon/reflection/type-builder.hpp>

namespace neon
{
  /// Holds the body of the entity to another body, or to the world. A door
  /// on a hinge, a pendulum on a point, a drawer on a slider, a load that is
  /// fixed to a cart.
  ///
  /// The joint is made once both bodies exist, and is gone when either of
  /// them is. It is made again when both are back. The anchor and the axis
  /// are on the entity, and go with it where it is when the joint is made.
  struct Joint
  {
    JointKind type = JointKind::Fixed;

    /// Path of the entity the body is joined to, from the top, such as
    /// `house/frame`. Empty for the world itself.
    std::string other;

    /// Where the joint sits on the entity. It is sized by the scale of the
    /// Transform, as the offset of a Collider is, so `[-0.5, 0, 0]` is the
    /// left edge of a scaled cube.
    glm::vec3 anchor{0.0f};

    /// Of a hinge, what it turns around. Of a slider, what it moves along.
    /// On the entity.
    glm::vec3 axis{0.0f, 1.0f, 0.0f};

    /// How far the body may go, as `[least, most]`: degrees around the axis
    /// for a hinge, units along it for a slider, from where the body is
    /// when the joint is made. Empty for no limit.
    std::vector<float> limits;

    /// What the physics knows the joint as. Filled in by the engine.
    JointId joint = No_Joint;

    /// Set by the engine when the joint could not be made, which was said in
    /// the log. It is not tried again.
    bool failed = false;
  };

  /// What the engine fills in, the joint and whether it failed, is not
  /// described.
  inline void Describe(TypeBuilder<Joint> &type)
  {
    type.Named("Joint", "Holds the body of the entity to another body, or to the world");

    // the type is what a joint is, so it is written even when it is the
    // default
    type.Choice("type", &Joint::type, {"fixed", "hinge", "slider", "point"})
        .AlwaysWritten()
        .Describe("Moves as one with the other body, turns around an axis, moves along one, or turns around a point");

    type.Field("other", &Joint::other)
        .Describe("Path of the entity the body is joined to, from the top. Empty for the world itself");

    type.Field("anchor", &Joint::anchor)
        .Describe("Where the joint sits on the entity, sized by the scale of the Transform");

    type.Field("axis", &Joint::axis)
        .OnlyWhen("type", {"hinge", "slider"})
        .Describe("What a hinge turns around, or a slider moves along, on the entity");

    type.Field("limits", &Joint::limits)
        .OnlyWhen("type", {"hinge", "slider"})
        .Describe("The least and the most, as [least, most]: degrees for a hinge, units for a slider. Empty for none");

    type.Rule("axis", [](const Joint &joint, const std::string &where) -> std::string
    {
      if ((joint.type != JointKind::Hinge && joint.type != JointKind::Slider) || joint.axis != glm::vec3{0.0f})
      {
        return {};
      }

      return std::format("'axis' of {} is [0, 0, 0], where a direction was expected", where);
    });

    type.Rule("limits", [](const Joint &joint, const std::string &where) -> std::string
    {
      if ((joint.type != JointKind::Hinge && joint.type != JointKind::Slider) || joint.limits.empty()) { return {}; }

      if (joint.limits.size() != 2)
      {
        return std::format(
          "'limits' of {} holds {} number{}, where [least, most] or an empty list was expected",
          where, joint.limits.size(), joint.limits.size() == 1 ? "" : "s");
      }

      const float least = joint.limits[0];
      const float most = joint.limits[1];

      if (joint.type == JointKind::Hinge && (least < -180.0f || most > 180.0f))
      {
        return std::format(
          "'limits' of {} is [{}, {}], where degrees from -180 to 180 were expected",
          where, least, most);
      }

      if (least > 0.0f || most < 0.0f)
      {
        return std::format(
          "'limits' of {} is [{}, {}], where the least is 0 or below and the most 0 or above",
          where, least, most);
      }
      return {};
    });
  }
} // neon

#endif //JOINT_HPP
