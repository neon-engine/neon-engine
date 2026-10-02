#include "vk-shadow-fit.hpp"

#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

namespace neon
{
  glm::mat4 VK_ShadowFit::View(const glm::vec3 &direction, const glm::vec3 &camera_position)
  {
    const glm::vec3 along = glm::normalize(direction);

    // a light straight down has no sideways from y, and takes x
    const glm::vec3 up = std::abs(along.y) > 0.99f ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);

    // The light stands as far back from the camera as the box reaches,
    // and looks along its direction through it.
    return glm::lookAt(camera_position - along * kDepth, camera_position, up);
  }

  glm::mat4 VK_ShadowFit::ViewProjection(
    const glm::vec3 &direction,
    const glm::vec3 &camera_position,
    const float map_size)
  {
    // The camera is at the origin of the light's plane. The box around it
    // is moved so that its corners sit on whole texels, which has the
    // shadows stay where they are while the camera moves within a texel.
    // The move is worked out from where the origin of the world lands,
    // which is what moves with the camera.
    const glm::mat4 view = View(direction, camera_position);
    const float texel = kBox / map_size;

    const glm::vec4 origin = view * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    const float snap_x = std::round(origin.x / texel) * texel - origin.x;
    const float snap_y = std::round(origin.y / texel) * texel - origin.y;

    constexpr float half = kBox / 2.0f;
    const glm::mat4 projection = glm::ortho(
      -half - snap_x, half - snap_x, -half - snap_y, half - snap_y, 0.0f, 2.0f * kDepth);

    return projection * view;
  }
} // neon
