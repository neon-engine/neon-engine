#ifndef VK_SHADOW_FIT_HPP
#define VK_SHADOW_FIT_HPP

#include <array>
#include <glm/glm.hpp>

#include "vk-shadow-cascades.hpp"

namespace neon
{
  /// Where the shadow map of the direction light looks: cascades, each a
  /// box around one slice of what the camera sees, seen along the light
  /// and drawn without perspective. The nearest slice is the thinnest, so
  /// its texels are the smallest where the shadows are looked at most, and
  /// the slices grow with the distance, up to `rendering.shadow_distance`
  /// from the camera, past which everything is lit. It is kept apart from
  /// the graphics card so that it can be checked.
  ///
  /// A slice is fitted by the sphere around its corners, so that the box
  /// has one size however the camera turns, and the box moves in whole
  /// texels of the map, so that the edge of a shadow does not shimmer as
  /// the camera moves by less than one. The box reaches back towards the
  /// light as far as the distance, so that what casts into the slice from
  /// above it, out of the camera's sight, is in the map.
  // ReSharper disable once CppInconsistentNaming
  struct VK_ShadowFit
  {
    /// How far the shadows reach when nothing says, in metres: the default
    /// of the setting `rendering.shadow_distance`.
    static constexpr float kDistance = 50.0f;

    /// How many cascades when nothing says: the default of the setting
    /// `rendering.shadow_cascades`.
    static constexpr int kCascades = 4;

    /// Where the slices end, from the camera along its view: `count` of
    /// them, the last at `distance`, spaced between evenly and
    /// logarithmically, half each, as Unity and Godot space theirs.
    [[nodiscard]] static std::array<float, kMax_Shadow_Cascades> Splits(float near, float distance, int count);

    /// The eight corners of the slice of the camera's view between `from`
    /// and `to` along it, in the world. `inverse_view_projection` undoes
    /// the camera's projection and view, the projection as glm makes it,
    /// with the depth from -1 to 1; `near` and `far` are its planes.
    [[nodiscard]] static std::array<glm::vec3, 8> SliceCorners(
      const glm::mat4 &inverse_view_projection,
      float near,
      float far,
      float from,
      float to);

    /// Where the light looks from, for a box around `centre` of `radius`:
    /// a view along `direction` with the centre at the origin of its plane,
    /// standing back from it by `radius` and `reach`, and up along the
    /// world's y unless the light is vertical, when it is along x.
    [[nodiscard]] static glm::mat4 View(const glm::vec3 &direction, const glm::vec3 &centre, float radius, float reach);

    /// The view and the projection of the light for a box around `centre`
    /// of `radius` across and along the light, reaching `reach` further
    /// back towards the light for casters above, moved by whole texels of
    /// a map `map_size` texels wide. The depth runs from -1 to 1 as glm has
    /// it, and the render system moves it onto what Vulkan expects.
    [[nodiscard]] static glm::mat4 BoxViewProjection(
      const glm::vec3 &direction,
      const glm::vec3 &centre,
      float radius,
      float reach,
      float map_size);

    /// The cascades for a camera: `view` and `projection` as the scene has
    /// them, the projection as glm makes it. `count` from 1 to
    /// kMax_Shadow_Cascades, `distance` how far the last reaches.
    [[nodiscard]] static VK_ShadowCascades Cascades(
      const glm::mat4 &view,
      const glm::mat4 &projection,
      const glm::vec3 &direction,
      float distance,
      int count,
      float map_size);
  };
} // neon

#endif //VK_SHADOW_FIT_HPP
