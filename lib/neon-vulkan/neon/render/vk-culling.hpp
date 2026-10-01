#ifndef VK_CULLING_HPP
#define VK_CULLING_HPP

#include <string>
#include <glm/glm.hpp>
#include <volk.h>

#include <neon/render/material-info.hpp>

namespace neon
{
  /// Which side of a triangle is drawn. It is kept apart from the renderer
  /// so that it can be checked without a graphics card.
  ///
  /// The front of a triangle is the side its corners go round anticlockwise
  /// on, as models are made. A transform that mirrors an object, with an
  /// odd number of its axes scaled below 0, turns that round, so a mirrored
  /// object is drawn with the opposite front.
  // ReSharper disable once CppInconsistentNaming
  struct VK_Culling
  {
    /// The back is left out, unless the material is drawn from both sides.
    [[nodiscard]] static VkCullModeFlags CullModeFor(bool double_sided);

    /// Which way round the corners of a front go, as the graphics card sees
    /// them.
    [[nodiscard]] static VkFrontFace FrontFaceFor(bool mirrored);

    /// Whether a transform mirrors what it places.
    [[nodiscard]] static bool IsMirrored(const glm::mat4 &model);

    /// What tells pipelines apart: materials with the same shader that cover
    /// alike and are culled alike share one.
    [[nodiscard]] static std::string PipelineKey(
      const std::string &shader_path, AlphaMode alpha_mode, bool double_sided, bool mirrored);
  };
} // neon

#endif //VK_CULLING_HPP
