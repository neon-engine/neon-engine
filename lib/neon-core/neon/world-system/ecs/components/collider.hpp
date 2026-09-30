#ifndef COLLIDER_HPP
#define COLLIDER_HPP

#include <string>

#include <glm/glm.hpp>

#include <neon/common/rotation.hpp>
#include <neon/physics/physics-types.hpp>

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

    /// Virtual path of the model a convex hull or a mesh is made from. The
    /// model is moved and sized as the renderer does it, so that the shape
    /// lies where the model is drawn.
    std::string model;

    /// Where the shape sits on the entity.
    glm::vec3 offset{0.0f};
    Rotation rotation{};
  };
} // neon

#endif //COLLIDER_HPP
