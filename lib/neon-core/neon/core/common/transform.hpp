#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <glm/glm.hpp>

#include "rotation.hpp"

namespace core
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
} // core

#endif //TRANSFORM_HPP
