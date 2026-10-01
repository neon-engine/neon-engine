#include "vk-swapchain-sizing.hpp"

#include <algorithm>
#include <cstdint>

namespace neon
{
  VK_FrameSizing VK_SwapchainSizing::Decide(const WindowSize window, const WindowSize made_for, const bool is_stale)
  {
    // a swapchain cannot have images without pixels
    if (window.width <= 0 || window.height <= 0) { return VK_FrameSizing::Skip; }

    if (is_stale || window.width != made_for.width || window.height != made_for.height)
    {
      return VK_FrameSizing::Recreate;
    }
    return VK_FrameSizing::Draw;
  }

  bool VK_SwapchainSizing::IsStale(const VkResult result)
  {
    // A suboptimal swapchain still shows the frame, but stretched or at
    // the wrong size. It is made again all the same.
    return result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR;
  }

  VkExtent2D VK_SwapchainSizing::ExtentOf(const VkSurfaceCapabilitiesKHR &capabilities, const VkExtent2D wanted)
  {
    if (capabilities.currentExtent.width != UINT32_MAX) { return capabilities.currentExtent; }

    return {
      std::clamp(wanted.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
      std::clamp(wanted.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
  }
} // neon
