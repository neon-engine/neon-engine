#include "vk-effect-images.hpp"

namespace neon
{
  bool VK_EffectImages::Initialize(
    VK_Device *device,
    const VK_Effects *effects,
    const VK_Resolve *resolve,
    const VK_EffectKind kind,
    const VkFormat format,
    const VkExtent2D extent)
  {
    _device = device;
    _effects = effects;
    _resolve = resolve;

    for (Picture &picture : _pictures)
    {
      if (!_device->CreateImage(
            extent.width, extent.height, 1, format,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            picture.image, picture.memory) ||
          !_device->CreateImageView(picture.image, format, VK_IMAGE_ASPECT_COLOR_BIT, 1, picture.view))
      {
        CleanUp();
        return false;
      }

      VkFramebufferCreateInfo framebuffer{};
      framebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer.renderPass = _effects->PassOf(kind);
      framebuffer.attachmentCount = 1;
      framebuffer.pAttachments = &picture.view;
      framebuffer.width = extent.width;
      framebuffer.height = extent.height;
      framebuffer.layers = 1;

      if (vkCreateFramebuffer(_device->Device(), &framebuffer, nullptr, &picture.framebuffer) != VK_SUCCESS)
      {
        CleanUp();
        return false;
      }

      picture.effect_set = _effects->Keep(picture.view);
      if (_resolve != nullptr) { picture.resolve_set = _resolve->Keep(picture.view); }
      if (picture.effect_set == VK_NULL_HANDLE || (_resolve != nullptr && picture.resolve_set == VK_NULL_HANDLE))
      {
        CleanUp();
        return false;
      }
    }

    _is_ready = true;
    return true;
  }

  void VK_EffectImages::CleanUp()
  {
    _is_ready = false;
    if (_device == nullptr || _device->Device() == VK_NULL_HANDLE) { return; }

    const VkDevice device = _device->Device();

    for (Picture &picture : _pictures)
    {
      if (_effects != nullptr) { _effects->Release(picture.effect_set); }
      if (_resolve != nullptr) { _resolve->Release(picture.resolve_set); }
      if (picture.framebuffer != VK_NULL_HANDLE) { vkDestroyFramebuffer(device, picture.framebuffer, nullptr); }
      if (picture.view != VK_NULL_HANDLE) { vkDestroyImageView(device, picture.view, nullptr); }
      if (picture.image != VK_NULL_HANDLE) { vkDestroyImage(device, picture.image, nullptr); }
      if (picture.memory != VK_NULL_HANDLE) { vkFreeMemory(device, picture.memory, nullptr); }
      picture = {};
    }
  }
} // neon
