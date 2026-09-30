#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>
#include <neon/reflection/type-builder.hpp>
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

  inline void Describe(TypeBuilder<Camera> &type)
  {
    type.Named("Camera", "Makes an entity the point the world is seen from");

    type.Choice("target", &Camera::target, {"window", "texture"})
        .Describe("What is drawn to");

    type.Field("fov", &Camera::fov)
        .Describe("Vertical field of view, in degrees");

    type.Field("near", &Camera::near_plane)
        .Describe("What is closer than this is not visible");

    type.Field("far", &Camera::far_plane)
        .Describe("What is further away than this is not visible");

    type.Field("up", &Camera::up)
        .Describe("The direction that is up");
  }
} // neon

#endif //CAMERA_HPP
