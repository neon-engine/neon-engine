#include "vk-swapchain.hpp"

#include "vk-present-mode.hpp"

#include <algorithm>

#include "vk-surface-format.hpp"
#include "vk-swapchain-sizing.hpp"

namespace neon
{
  bool VK_Swapchain::Initialize(VK_Device *device, const WindowSize window, const std::shared_ptr<Logger> &logger)
  {
    _device = device;
    _logger = logger;

    if (!Create({static_cast<uint32_t>(window.width), static_cast<uint32_t>(window.height)})) { return false; }

    _window_size = window;
    return true;
  }

  bool VK_Swapchain::Create(const VkExtent2D wanted)
  {
    const VkPhysicalDevice physical_device = _device->PhysicalDevice();
    const VkSurfaceKHR surface = _device->Surface();

    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &capabilities);

    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physical_device, surface, &count, formats.data());

    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, nullptr);
    std::vector<VkPresentModeKHR> modes(count);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physical_device, surface, &count, modes.data());

    if (formats.empty() || modes.empty())
    {
      _logger->Critical("The window offers no format to present in");
      return false;
    }

    if (!(capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
    {
      _logger->Critical("The window does not accept copied images");
      return false;
    }

    // the frame is copied as it is, and its bytes are sRGB already
    const VkSurfaceFormatKHR format = ChooseSurfaceFormat(formats);
    if (IsSrgbFormat(format.format) && !_warned_about_format)
    {
      _warned_about_format = true;
      _logger->Warn("The window offers only formats that convert to sRGB, frames are shown too light");
    }

    // with vertical sync a frame waits for the screen, and without it is
    // shown as soon as it is done, where the driver can, see VK_PresentMode
    const VkPresentModeKHR mode = VK_PresentMode::Choose(modes, _vertical_sync);
    if (mode != _present_mode || !_said_present_mode)
    {
      _said_present_mode = true;
      const char *name = VK_PresentMode::NameOf(mode);
      if (!_vertical_sync && mode != VK_PRESENT_MODE_IMMEDIATE_KHR)
      {
        _logger->Warn("Vertical sync stays on: the driver shows no frame before the screen is ready. Presenting with {}", name);
      } else
      {
        const char *sync = _vertical_sync ? "on" : "off";
        _logger->Info("Presenting with {}, vertical sync {}", name, sync);
      }
    }
    _present_mode = mode;

    // The window usually dictates the size. A minimized one can have none,
    // and keeps the swapchain it has until it is shown again.
    const VkExtent2D extent = VK_SwapchainSizing::ExtentOf(capabilities, wanted);
    if (extent.width == 0 || extent.height == 0) { return false; }

    uint32_t image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0) { image_count = std::min(image_count, capabilities.maxImageCount); }

    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = surface;
    info.minImageCount = image_count;
    info.imageFormat = format.format;
    info.imageColorSpace = format.colorSpace;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = capabilities.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = mode;
    info.clipped = VK_TRUE;
    // a swapchain that is made again can take over from the one before
    info.oldSwapchain = _swapchain;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    if (vkCreateSwapchainKHR(_device->Device(), &info, nullptr, &swapchain) != VK_SUCCESS)
    {
      _logger->Critical("Could not create the Vulkan swapchain");

      // the one before is retired all the same, and cannot be drawn to
      if (_swapchain != VK_NULL_HANDLE) { vkDestroySwapchainKHR(_device->Device(), _swapchain, nullptr); }
      _swapchain = VK_NULL_HANDLE;
      return false;
    }

    if (_swapchain != VK_NULL_HANDLE) { vkDestroySwapchainKHR(_device->Device(), _swapchain, nullptr); }
    _swapchain = swapchain;
    _extent = extent;

    vkGetSwapchainImagesKHR(_device->Device(), _swapchain, &count, nullptr);
    _images.resize(count);
    vkGetSwapchainImagesKHR(_device->Device(), _swapchain, &count, _images.data());

    // the semaphores stay when the swapchain is made again
    if (_image_available != VK_NULL_HANDLE) { return true; }

    VkSemaphoreCreateInfo semaphore{};
    semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    return vkCreateSemaphore(_device->Device(), &semaphore, nullptr, &_image_available) == VK_SUCCESS &&
           vkCreateSemaphore(_device->Device(), &semaphore, nullptr, &_render_finished) == VK_SUCCESS;
  }

  void VK_Swapchain::CleanUp()
  {
    if (_device == nullptr) { return; }

    const VkDevice device = _device->Device();

    if (_image_available != VK_NULL_HANDLE) { vkDestroySemaphore(device, _image_available, nullptr); }
    if (_render_finished != VK_NULL_HANDLE) { vkDestroySemaphore(device, _render_finished, nullptr); }
    if (_swapchain != VK_NULL_HANDLE) { vkDestroySwapchainKHR(device, _swapchain, nullptr); }

    _image_available = VK_NULL_HANDLE;
    _render_finished = VK_NULL_HANDLE;
    _swapchain = VK_NULL_HANDLE;
    _images.clear();
  }

  bool VK_Swapchain::Fit(const WindowSize window)
  {
    switch (VK_SwapchainSizing::Decide(window, _window_size, _stale))
    {
      case VK_FrameSizing::Draw: return true;
      case VK_FrameSizing::Skip: return false;
      case VK_FrameSizing::Recreate: break;
    }

    // nothing that is made again may still be in use
    vkDeviceWaitIdle(_device->Device());

    if (!Create({static_cast<uint32_t>(window.width), static_cast<uint32_t>(window.height)}))
    {
      // tried again in the next frame
      _stale = true;
      return false;
    }
    _stale = false;
    _window_size = window;
    return true;
  }

  bool VK_Swapchain::Acquire(uint32_t &image_index)
  {
    const VkResult acquired = vkAcquireNextImageKHR(
      _device->Device(), _swapchain, UINT64_MAX, _image_available, VK_NULL_HANDLE, &image_index);

    // the swapchain is made again before the next frame
    if (VK_SwapchainSizing::IsStale(acquired)) { _stale = true; }

    if (acquired == VK_SUCCESS || acquired == VK_SUBOPTIMAL_KHR) { return true; }

    if (acquired != VK_ERROR_OUT_OF_DATE_KHR)
    {
      const int code = acquired;
      _logger->Warn("Could not get an image of the window to draw to, error {}", code);
    }
    return false;
  }

  void VK_Swapchain::CopyFrom(
    const VkCommandBuffer commands,
    const VkImage frame,
    const VkExtent2D frame_extent,
    const uint32_t image_index) const
  {
    const VkImage target = _images[image_index];
    constexpr VkImageAspectFlags color = VK_IMAGE_ASPECT_COLOR_BIT;

    VK_Device::TransitionImage(
      commands, target, color, 0, 1,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // a scaling copy, which also converts between the color orders that the
    // renderer and the window may use
    VkImageBlit blit{};
    blit.srcSubresource = {color, 0, 0, 1};
    blit.srcOffsets[1] = {static_cast<int32_t>(frame_extent.width), static_cast<int32_t>(frame_extent.height), 1};
    blit.dstSubresource = {color, 0, 0, 1};
    blit.dstOffsets[1] = {static_cast<int32_t>(_extent.width), static_cast<int32_t>(_extent.height), 1};

    vkCmdBlitImage(
      commands,
      frame, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1, &blit, VK_FILTER_NEAREST);

    VK_Device::TransitionImage(
      commands, target, color, 0, 1,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  }

  void VK_Swapchain::Present(const uint32_t image_index)
  {
    VkPresentInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &_render_finished;
    info.swapchainCount = 1;
    info.pSwapchains = &_swapchain;
    info.pImageIndices = &image_index;

    if (VK_SwapchainSizing::IsStale(vkQueuePresentKHR(_device->Queue(), &info))) { _stale = true; }
  }
} // neon
