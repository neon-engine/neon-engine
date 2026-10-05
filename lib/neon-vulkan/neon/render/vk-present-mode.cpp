#include "vk-present-mode.hpp"

#include <algorithm>

namespace neon
{
  VkPresentModeKHR VK_PresentMode::Choose(const std::span<const VkPresentModeKHR> offered, const bool vertical_sync)
  {
    if (!vertical_sync && std::ranges::find(offered, VK_PRESENT_MODE_IMMEDIATE_KHR) != offered.end())
    {
      return VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    // waiting for the screen is the one mode every driver has
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  const char *VK_PresentMode::NameOf(const VkPresentModeKHR mode)
  {
    switch (mode)
    {
      case VK_PRESENT_MODE_IMMEDIATE_KHR: return "immediate";
      case VK_PRESENT_MODE_MAILBOX_KHR: return "mailbox";
      case VK_PRESENT_MODE_FIFO_KHR: return "fifo";
      case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return "fifo relaxed";
      default: return "another mode";
    }
  }
} // neon
