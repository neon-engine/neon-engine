#ifndef COLLIDER_HPP
#define COLLIDER_HPP

#include <format>
#include <string>

#include <glm/glm.hpp>

#include <neon/common/rotation.hpp>
#include <neon/physics/physics-types.hpp>
#include <neon/reflection/type-builder.hpp>
#include <neon/render/model-fit.hpp>

namespace neon
{
  /// Gives a shape to the body of the entity. That is the RigidBody, the
  /// Trigger, or the CharacterBody of the entity itself, or of the nearest
  /// entity above it that has one. So a body of several shapes is an entity
  /// with children that each carry a Collider.
  ///
  /// The shape is sized by the scale of the Transform, as what is drawn is.
  /// Which of the values count depends on the shape.
  struct Collider
  {
    ShapeKind shape = ShapeKind::Box;

    /// The lengths of the sides of a box.
    glm::vec3 size{1.0f};

    /// Of a sphere, a capsule, and a cylinder.
    float radius = 0.5f;

    /// Of a capsule and a cylinder, and of their tapered kinds, from end to
    /// end.
    float height = 2.0f;

    /// Of the tapered kinds.
    float top_radius = 0.25f;
    float bottom_radius = 0.5f;

    /// Virtual path of the model a convex hull or a mesh is made from.
    std::string model;

    /// How the model is sized, which is to be the `fit` of the Renderable
    /// that draws it, so that the shape lies where the model is drawn.
    ModelFit fit = ModelFit::None;

    /// Where the shape sits on the entity.
    glm::vec3 offset{0.0f};
    Rotation rotation{};
  };

  /// Only what belongs to the shape is read and written. A radius that is
  /// written for a box is then reported as a name that is not known,
  /// instead of being ignored without a word.
  inline void Describe(TypeBuilder<Collider> &type)
  {
    type.Named("Collider", "Gives a shape to the body of the entity");

    // in the order of ShapeKind. The shape is what a collider is, so it is
    // written even when it is the default
    type.Choice(
          "shape",
          &Collider::shape,
          {"box", "sphere", "capsule", "cylinder", "tapered_capsule", "tapered_cylinder", "plane", "convex_hull", "mesh"})
        .AlwaysWritten()
        .Describe("What the shape is. Which of the other values count depends on it");

    type.Field("size", &Collider::size)
        .OneNumberForAll()
        .Above(0)
        .OnlyWhen("shape", {"box"})
        .Describe("The lengths of the sides of a box");

    type.Field("radius", &Collider::radius)
        .Above(0)
        .OnlyWhen("shape", {"sphere", "capsule", "cylinder"});

    type.Field("height", &Collider::height)
        .Above(0)
        .OnlyWhen("shape", {"capsule", "cylinder", "tapered_capsule", "tapered_cylinder"})
        .Describe("From end to end");

    // one of the two may be 0 for a tapered cylinder, which makes a cone.
    // That a tapered capsule needs both above 0 is a rule below
    type.Field("top_radius", &Collider::top_radius)
        .AtLeast(0)
        .OnlyWhen("shape", {"tapered_capsule", "tapered_cylinder"});

    type.Field("bottom_radius", &Collider::bottom_radius)
        .AtLeast(0)
        .OnlyWhen("shape", {"tapered_capsule", "tapered_cylinder"});

    type.Field("model", &Collider::model)
        .AlwaysWritten()
        .OnlyWhen("shape", {"convex_hull", "mesh"})
        .Describe("Virtual path of the model the shape is made from. Left out, the shape is the entity's Geometry");

    type.Choice("fit", &Collider::fit, {"none", "unit"})
        .OnlyWhen("shape", {"convex_hull", "mesh"})
        .Describe(
          "How the model is sized, as the fit of the Renderable that draws it: none takes it as the file says it, "
          "unit moves its middle to the origin and scales it so that its longest side is 1");

    type.Field("offset", &Collider::offset)
        .Describe("Where the shape sits on the entity");

    type.Field<glm::vec3>(
          "rotation",
          [](const Collider &collider)
          {
            return glm::vec3{collider.rotation.pitch, collider.rotation.yaw, collider.rotation.roll};
          },
          [](Collider &collider, const glm::vec3 &degrees)
          {
            collider.rotation = {degrees.x, degrees.y, degrees.z};
          })
        .Describe("How the shape is turned on the entity: pitch, yaw, and roll in degrees");

    // a tapered capsule ends in a sphere at both ends, which cannot be
    // without a radius
    const auto above_zero = [](const std::string &name, float Collider::*radius)
    {
      return [name, radius](const Collider &collider, const std::string &where) -> std::string
      {
        if (collider.shape != ShapeKind::TaperedCapsule || collider.*radius > 0.0f) { return {}; }

        return std::format("'{}' of {} is {}, where a number above 0 was expected", name, where, collider.*radius);
      };
    };

    type.Rule("top_radius", above_zero("top_radius", &Collider::top_radius));
    type.Rule("bottom_radius", above_zero("bottom_radius", &Collider::bottom_radius));

    type.Rule([](const Collider &collider, const std::string &where) -> std::string
    {
      if (collider.shape != ShapeKind::TaperedCylinder || collider.top_radius != 0.0f || collider.bottom_radius != 0.0f)
      {
        return {};
      }

      return std::format(
        "'top_radius' and 'bottom_radius' of {} are both 0, where one above 0 was expected", where);
    });

    type.Rule([](const Collider &collider, const std::string &where) -> std::string
    {
      if (collider.shape != ShapeKind::Capsule || collider.height >= 2.0f * collider.radius) { return {}; }

      return std::format(
        "'height' of {} is {}, which is less than twice its 'radius' of {}",
        where, collider.height, collider.radius);
    });

    type.Rule([](const Collider &collider, const std::string &where) -> std::string
    {
      if (collider.shape != ShapeKind::TaperedCapsule
          || collider.height >= collider.top_radius + collider.bottom_radius)
      {
        return {};
      }

      return std::format(
        "'height' of {} is {}, which is less than its 'top_radius' and 'bottom_radius' together",
        where, collider.height);
    });
  }
} // neon

#endif //COLLIDER_HPP
