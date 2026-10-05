#ifndef VK_SWAPCHAIN_HPP
#define VK_SWAPCHAIN_HPP

#include <cstdint>
#include <memory>
#include <vector>
#include <volk.h>
#include <neon/logging/logger.hpp>
#include <neon/window/window-context.hpp>

#include "vk-device.hpp"

namespace neon
{
  /// The images of a window, which a finished frame is copied into and
  /// shown through. There is none without a window.
  ///
  /// The swapchain follows the window: when the window changes its size,
  /// or Vulkan says that the swapchain no longer fits, it is made again
  /// before the next frame, with everything that has its size. What those
  /// are is left to the renderer, which asks with Fit().
  // ReSharper disable once CppInconsistentNaming
  class VK_Swapchain
  {
    VK_Device *_device = nullptr;
    std::shared_ptr<Logger> _logger;

    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    VkExtent2D _extent{};
    std::vector<VkImage> _images;
    VkSemaphore _image_available = VK_NULL_HANDLE;
    VkSemaphore _render_finished = VK_NULL_HANDLE;

    // The size of the window the swapchain was made for, and whether Vulkan
    // said since that it no longer fits. Either makes it again.
    WindowSize _window_size{};
    bool _stale = false;

    // whether a frame waits for the screen, the mode that was taken for it,
    // and whether that was said
    bool _vertical_sync = true;
    VkPresentModeKHR _present_mode = VK_PRESENT_MODE_FIFO_KHR;
    bool _said_present_mode = false;
    bool _warned_about_format = false;

    bool Create(VkExtent2D wanted);

  public:
    /// Makes the swapchain for the window, of the size of the window.
    bool Initialize(VK_Device *device, WindowSize window, const std::shared_ptr<Logger> &logger);

    void CleanUp();

    [[nodiscard]] bool IsReady() const { return _swapchain != VK_NULL_HANDLE; }

    /// The size of the images, which is usually that of the window.
    [[nodiscard]] VkExtent2D Extent() const { return _extent; }

    /// Makes the swapchain again when the window changed its size. Returns
    /// false when there is nothing to draw to, as while the window is
    /// minimized. The renderer makes what has the size of the frame again
    /// when the size changed.
    bool Fit(WindowSize window);

    /// Has the swapchain made again before the next frame, as when what
    /// has its size could not be made.
    void MakeAgain() { _stale = true; }

    /// Has a frame wait for the screen, or not. The swapchain is made again
    /// before the next frame when that changes. Call it before Initialize()
    /// for what holds from the start.
    void SetVerticalSync(const bool enabled)
    {
      if (enabled == _vertical_sync) { return; }
      _vertical_sync = enabled;
      _stale = true;
    }

    [[nodiscard]] bool GetVerticalSync() const { return _vertical_sync; }

    /// Takes the image the next frame is shown in. Returns false when the
    /// window has none to give, in which case the frame is not shown.
    bool Acquire(uint32_t &image_index);

    /// Copies the frame into the image taken with Acquire(), scaled to the
    /// window, and leaves it ready to be shown.
    void CopyFrom(VkCommandBuffer commands, VkImage frame, VkExtent2D frame_extent, uint32_t image_index) const;

    /// What the commands of a frame wait for before they copy into the
    /// image, and what they signal when they are done.
    [[nodiscard]] VkSemaphore ImageAvailable() const { return _image_available; }
    [[nodiscard]] VkSemaphore RenderFinished() const { return _render_finished; }

    /// Shows the image taken with Acquire(), once the commands of the frame
    /// have signalled that they are done.
    void Present(uint32_t image_index);
  };
} // neon

#endif //VK_SWAPCHAIN_HPP
