#ifndef VK_SWAPCHAIN_SIZING_HPP
#define VK_SWAPCHAIN_SIZING_HPP

#include <volk.h>
#include <neon/window/window-context.hpp>

namespace neon
{
  /// What a frame that is shown in a window does about the size of the
  /// window.
  // ReSharper disable once CppInconsistentNaming
  enum class VK_FrameSizing
  {
    /// The swapchain fits the window, and the frame is drawn.
    Draw,

    /// The swapchain no longer fits the window. It is made again, with
    /// everything that has its size, before the frame is drawn.
    Recreate,

    /// The window has no area, as when it is minimized. Nothing is drawn
    /// until it has one again.
    Skip,
  };

  /// Decides when the swapchain has to be made again, and at which size.
  /// It is kept apart from the renderer so that it can be checked without
  /// a graphics card.
  // ReSharper disable once CppInconsistentNaming
  struct VK_SwapchainSizing
  {
    /// `window` is the size of what is drawn to now, and `made_for` the
    /// size the swapchain was made for. `is_stale` is whether Vulkan said
    /// that the swapchain no longer fits.
    [[nodiscard]] static VK_FrameSizing Decide(WindowSize window, WindowSize made_for, bool is_stale);

    /// Whether what acquiring or presenting an image returned says that the
    /// swapchain has to be made again.
    [[nodiscard]] static bool IsStale(VkResult result);

    /// The size of the images of the swapchain. The window usually
    /// dictates it. When it leaves the size open, it is `wanted`, within
    /// what the window allows. Either is 0 by 0 for a minimized window on
    /// some systems, and no swapchain can be made then.
    [[nodiscard]] static VkExtent2D ExtentOf(const VkSurfaceCapabilitiesKHR &capabilities, VkExtent2D wanted);
  };
} // neon

#endif //VK_SWAPCHAIN_SIZING_HPP
