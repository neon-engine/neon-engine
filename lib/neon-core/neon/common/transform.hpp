#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <glm/glm.hpp>

#include <neon/reflection/type-builder.hpp>

#include "rotation.hpp"

namespace neon
{
  struct Transform
  {
    glm::vec3 position{0.0f};
    Rotation rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
    glm::mat4 world_coordinates{1.0f};
    [[nodiscard]] glm::vec3 Forward() const { return normalize(glm::mat3(world_coordinates) * World_Forward()); }
    // ReSharper disable once CppMemberFunctionMayBeStatic
    [[nodiscard]] glm::vec3 Up() const { return World_Up(); } // NOLINT(*-convert-member-functions-to-static)
    [[nodiscard]] glm::vec3 Right() const { return normalize(cross(Forward(), Up())); }
    static glm::vec3 World_Forward() { return {0.0f, 0.0f, -1.0f}; }
    static glm::vec3 World_Up() { return {0.0f, 1.0f, 0.0f}; }
  };

  /// Where a Transform puts an entity in the world is worked out every
  /// frame, so it is not described.
  inline void Describe(TypeBuilder<Transform> &type)
  {
    type.Named("Transform", "Where an entity is, how it is turned, and how large it is");

    type.Field("position", &Transform::position)
        .Describe("Relative to the parent");

    type.Field<glm::vec3>(
          "rotation",
          [](const Transform &transform)
          {
            return glm::vec3{transform.rotation.pitch, transform.rotation.yaw, transform.rotation.roll};
          },
          [](Transform &transform, const glm::vec3 &rotation)
          {
            transform.rotation.pitch = rotation.x;
            transform.rotation.yaw = rotation.y;
            transform.rotation.roll = rotation.z;
          })
        .Describe("Pitch, yaw, and roll in degrees");

    type.Field("scale", &Transform::scale)
        .OneNumberForAll()
        .Describe("One number stands for all three directions");
  }
} // neon

#endif //TRANSFORM_HPP
