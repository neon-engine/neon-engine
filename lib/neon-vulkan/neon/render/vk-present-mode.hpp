#ifndef VK_PRESENT_MODE_HPP
#define VK_PRESENT_MODE_HPP

#include <span>

#include "vk-device.hpp"

namespace neon
{
  /// How frames reach the screen, chosen from what the driver offers. It
  /// is kept apart from the swapchain so that it can be checked without a
  /// graphics card.
  ///
  /// Two modes, which every platform of the engine has (#356): with
  /// vertical sync a frame waits for the screen, FIFO, which every driver
  /// offers. Without it a frame is shown as soon as it is done, immediate,
  /// where the driver offers that, and waits for the screen where not.
  // ReSharper disable once CppInconsistentNaming
  struct VK_PresentMode
  {
    [[nodiscard]] static VkPresentModeKHR Choose(std::span<const VkPresentModeKHR> offered, bool vertical_sync);

    /// What a mode is called in the log.
    [[nodiscard]] static const char *NameOf(VkPresentModeKHR mode);
  };
} // neon

#endif //VK_PRESENT_MODE_HPP
