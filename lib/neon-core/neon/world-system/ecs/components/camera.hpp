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

    /// Whether the view rolls with the entity: what is up for the camera
    /// then turns with the rotation of its entity, so that leaning the
    /// entity leans the view. It is what a view from the eyes wants, a head
    /// that tilts. A camera that looks at something from outside, over a
    /// shoulder or from above, leaves it off and stays level however what it
    /// hangs from is turned.
    bool rolls_with_entity = false;

    /// The effects that are run over the whole picture the camera drew,
    /// one after the other: fragment shaders a game brings, named as a
    /// shader is, such as `extensions://quake/shaders/under-water`.
    /// These are run on the light of the scene, before the tonemapper.
    std::vector<std::string> effects;

    /// As `effects`, but run on the colours a screen is given: after the
    /// tonemapper, and before the user interface is drawn on top.
    std::vector<std::string> screen_effects;

    /// For `target: texture`: what the texture is called, and its size in
    /// pixels. A model shows what the camera sees as the texture
    /// `surface://` and the name, and so does an image of a user
    /// interface.
    std::string texture;
    int texture_width = 512;
    int texture_height = 512;

    /// For a texture: the most levels of smaller copies it has, which are
    /// made again in every frame the camera draws: 1 for none, 0 for as
    /// many as its size allows. A picture that is only seen from close by,
    /// such as that of a monitor, needs few. The setting
    /// rendering.target_mipmaps lowers it, never raises it.
    int mipmaps = 0;
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

    type.Field("rolls_with_entity", &Camera::rolls_with_entity)
        .Describe("Whether the view rolls with the rotation of its entity, as a view from the eyes does");

    type.Field("effects", &Camera::effects)
        .Describe("Shaders that are run over the picture of the camera, on the light of its scene before the tonemapper");

    type.Field("screen_effects", &Camera::screen_effects)
        .Describe("Shaders that are run over the picture of the camera, on the colours of the screen after the tonemapper");

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

    type.Field("mipmaps", &Camera::mipmaps)
        .AtLeast(0)
        .AtMost(16)
        .Describe("For a texture: the most levels of smaller copies it has, 1 for none, 0 for as many as its size allows");
  }
} // neon

#endif //CAMERA_HPP
