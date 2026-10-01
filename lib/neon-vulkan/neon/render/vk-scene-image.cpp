#include "vk-scene-image.hpp"

#include <array>

namespace neon
{
  bool VK_SceneImage::Initialize(
    VK_Device *device,
    const uint32_t width,
    const uint32_t height,
    const VkRenderPass scene_pass,
    const VkFormat depth_format)
  {
    _device = device;
    _extent = {width, height};

    if (!_device->CreateImage(
          width, height, 1, kFormat,
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
          _color_image, _color_memory) ||
        !_device->CreateImageView(_color_image, kFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1, _color_view) ||
        !_device->CreateImage(
          width, height, 1, depth_format,
          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
          _depth_image, _depth_memory) ||
        !_device->CreateImageView(_depth_image, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, 1, _depth_view))
    {
      CleanUp();
      return false;
    }

    const std::array views{_color_view, _depth_view};

    VkFramebufferCreateInfo framebuffer{};
    framebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer.renderPass = scene_pass;
    framebuffer.attachmentCount = static_cast<uint32_t>(views.size());
    framebuffer.pAttachments = views.data();
    framebuffer.width = width;
    framebuffer.height = height;
    framebuffer.layers = 1;

    if (vkCreateFramebuffer(_device->Device(), &framebuffer, nullptr, &_framebuffer) != VK_SUCCESS)
    {
      CleanUp();
      return false;
    }
    return true;
  }

  void VK_SceneImage::CleanUp()
  {
    if (_device == nullptr || _device->Device() == VK_NULL_HANDLE) { return; }

    const VkDevice device = _device->Device();

    if (_framebuffer != VK_NULL_HANDLE) { vkDestroyFramebuffer(device, _framebuffer, nullptr); }
    if (_depth_view != VK_NULL_HANDLE) { vkDestroyImageView(device, _depth_view, nullptr); }
    if (_depth_image != VK_NULL_HANDLE) { vkDestroyImage(device, _depth_image, nullptr); }
    if (_depth_memory != VK_NULL_HANDLE) { vkFreeMemory(device, _depth_memory, nullptr); }
    if (_color_view != VK_NULL_HANDLE) { vkDestroyImageView(device, _color_view, nullptr); }
    if (_color_image != VK_NULL_HANDLE) { vkDestroyImage(device, _color_image, nullptr); }
    if (_color_memory != VK_NULL_HANDLE) { vkFreeMemory(device, _color_memory, nullptr); }

    _framebuffer = VK_NULL_HANDLE;
    _depth_view = VK_NULL_HANDLE;
    _depth_image = VK_NULL_HANDLE;
    _depth_memory = VK_NULL_HANDLE;
    _color_view = VK_NULL_HANDLE;
    _color_image = VK_NULL_HANDLE;
    _color_memory = VK_NULL_HANDLE;
    _extent = {};
  }
} // neon
