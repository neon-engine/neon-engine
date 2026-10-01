#include "vk-draw-order.hpp"

#include <algorithm>

namespace neon
{
  float VK_DrawOrder::DistanceOf(const glm::mat4 &view, const glm::vec3 &position)
  {
    // the camera looks along -z of its own space
    return -(view * glm::vec4(position, 1.0f)).z;
  }

  void VK_DrawOrder::BackToFront(std::vector<VK_SeeThroughDraw> &draws)
  {
    std::ranges::stable_sort(draws, [](const VK_SeeThroughDraw &a, const VK_SeeThroughDraw &b)
    {
      return a.distance > b.distance;
    });
  }
} // neon
