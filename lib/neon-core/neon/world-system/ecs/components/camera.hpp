#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <string>
#include <vector>

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

    /// For `target: texture`: what the texture is called, and its size in
    /// pixels. A model shows what the camera sees as the texture
    /// `surface://` and the name, and so does an image of a user
    /// interface.
    std::string texture;
    int texture_width = 512;
    int texture_height = 512;
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

    type.Field("texture", &Camera::texture)
        .Describe("For a texture: what it is called, which a model shows as surface:// and the name");

    // two whole numbers, seen as the list [width, height]
    type.Field<std::vector<float>>(
          "size",
          [](const Camera &camera)
          {
            return std::vector{static_cast<float>(camera.texture_width), static_cast<float>(camera.texture_height)};
          },
          [](Camera &camera, const std::vector<float> &size)
          {
            camera.texture_width = static_cast<int>(size[0]);
            camera.texture_height = static_cast<int>(size[1]);
          })
        .Count(2)
        .AtLeast(1)
        .Describe("For a texture: its width and height in pixels");
  }
} // neon

#endif //CAMERA_HPP
