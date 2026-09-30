#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <neon/render/render-target.hpp>

namespace neon
{
  /// Makes an entity the point the world is seen from. It looks along the
  /// forward direction of its Transform.
  struct Camera
  {
    RenderTarget target = RenderTarget::Window;

    /// Vertical field of view, in degrees.
    float fov = 45.0f;

    float near_plane = 0.1f;
    float far_plane = 1000.f;
    glm::vec3 up{0.0f, 1.0f, 0.0f};
  };
} // neon

#endif //CAMERA_HPP
