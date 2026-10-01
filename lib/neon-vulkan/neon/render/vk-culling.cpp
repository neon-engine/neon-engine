#include "vk-culling.hpp"

namespace neon
{
  VkCullModeFlags VK_Culling::CullModeFor(const bool double_sided)
  {
    return double_sided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
  }

  VkFrontFace VK_Culling::FrontFaceFor(const bool mirrored)
  {
    return mirrored ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
  }

  bool VK_Culling::IsMirrored(const glm::mat4 &model)
  {
    // the sign of the volume a transform gives a unit cube, which is below
    // 0 when it is turned inside out
    return determinant(glm::mat3(model)) < 0.0f;
  }

  std::string VK_Culling::PipelineKey(
    const std::string &shader_path,
    const AlphaMode alpha_mode,
    const bool double_sided,
    const bool mirrored)
  {
    std::string key = shader_path;
    if (alpha_mode == AlphaMode::Blend) { key += " blended"; }

    // both sides of a triangle look the same mirrored or not
    if (double_sided) { key += " double-sided"; } else if (mirrored) { key += " mirrored"; }
    return key;
  }
} // neon
