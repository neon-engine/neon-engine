#ifndef VK_SHADOW_FIT_HPP
#define VK_SHADOW_FIT_HPP

#include <glm/glm.hpp>

namespace neon
{
  /// Where the shadow map of the direction light looks: a box of fixed
  /// size around the camera, seen along the light, drawn without
  /// perspective. It is kept apart from the graphics card so that it can
  /// be checked.
  ///
  /// The box follows the camera, and it moves in whole texels of the map,
  /// so that the edge of a shadow does not shimmer as the camera moves
  /// by less than one. What is further from the camera than half the box
  /// is lit, and so is what is nearer the light than the box reaches,
  /// which the comparison sampler says with its border.
  // ReSharper disable once CppInconsistentNaming
  struct VK_ShadowFit
  {
    /// The width and the height of the box, in metres, across the light.
    static constexpr float kBox = 40.0f;

    /// How far the box reaches along the light, in metres, to either side
    /// of the camera: as far as it is wide, so that what is above the
    /// camera as high as the box is wide casts into it.
    static constexpr float kDepth = 40.0f;

    /// Where the light looks from: a view along `direction` with the
    /// camera at the origin of its plane, and up along the world's y
    /// unless the light is vertical, when it is along x.
    [[nodiscard]] static glm::mat4 View(const glm::vec3 &direction, const glm::vec3 &camera_position);

    /// The view and the projection of the light together, with the box
    /// moved by whole texels of a map `map_size` texels wide. The depth
    /// runs from -1 to 1 as glm has it, like the projection of a camera,
    /// and the render system moves it onto what Vulkan expects.
    [[nodiscard]] static glm::mat4 ViewProjection(
      const glm::vec3 &direction,
      const glm::vec3 &camera_position,
      float map_size);
  };
} // neon

#endif //VK_SHADOW_FIT_HPP
