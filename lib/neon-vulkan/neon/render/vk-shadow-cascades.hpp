#ifndef VK_SHADOW_CASCADES_HPP
#define VK_SHADOW_CASCADES_HPP

#include <array>
#include <glm/glm.hpp>

#include "vk-shader-data.hpp"

namespace neon
{
  /// Where the shadow map looks, cascade by cascade: the matrix of each,
  /// how far from the camera each reaches along its view, and how many
  /// there are. What VK_ShadowFit works out and the scene data carries.
  // ReSharper disable once CppInconsistentNaming
  struct VK_ShadowCascades
  {
    std::array<glm::mat4, kMax_Shadow_Cascades> view_projections{glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f), glm::mat4(1.0f)};
    std::array<float, kMax_Shadow_Cascades> splits{};
    int count = 0;
  };
} // neon

#endif //VK_SHADOW_CASCADES_HPP
