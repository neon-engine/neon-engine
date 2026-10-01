#include "vk-render-target.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>

namespace neon
{
  VK_RenderTarget::VK_RenderTarget(
    const std::string &name,
    VK_Device *device,
    const std::shared_ptr<Logger> &logger)
  {
    _name = name;
    _device = device;
    _logger = logger;
  }

  uint32_t VK_RenderTarget::LevelsFor(const uint32_t width, const uint32_t height)
  {
    uint32_t levels = 1;
    uint32_t side = std::max(width, height);

    while (side > 1)
    {
      side /= 2;
      levels++;
    }
    return levels;
  }

  std::string VK_RenderTarget::NameOf(const std::string &texture_path)
  {
    const std::size_t length = std::strlen(kScheme);
    if (texture_path.compare(0, length, kScheme) != 0) { return ""; }

    return texture_path.substr(length);
  }

  bool VK_RenderTarget::Initialize(
    const uint32_t width,
    const uint32_t height,
    const VkRenderPass render_pass,
    const VkFormat color_format)
  {
    if (_device == nullptr || _device->Device() == VK_NULL_HANDLE || render_pass == VK_NULL_HANDLE)
    {
      _logger->Error("The render target '{}' needs a renderer that is initialized", _name);
      return false;
    }

    const VkDevice device = _device->Device();

    // The image is made in the sRGB format, so that its smaller copies are
    // the mean of the light, and may be seen through views of either
    // format. Scaling has to work with it.
    const VkFormat sampled_format = SampledFormatOf(color_format);
    VkFormatProperties properties;
    vkGetPhysicalDeviceFormatProperties(_device->PhysicalDevice(), sampled_format, &properties);
    const bool can_scale =
      (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) &&
      (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) &&
      (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

    _extent = {width, height};
    _mip_levels = can_scale ? LevelsFor(width, height) : 1;

    constexpr VkImageAspectFlags color = VK_IMAGE_ASPECT_COLOR_BIT;

    if (!_device->CreateImage(
          width, height, _mip_levels, sampled_format,
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
          VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
          _color_image, _color_memory, VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) ||
        !_device->CreateImageView(_color_image, color_format, color, 1, _attachment_view) ||
        !_device->CreateImageView(_color_image, sampled_format, color, _mip_levels, _view) ||
        !_device->CreateImageView(_color_image, color_format, color, _mip_levels, _bytes_view))
    {
      _logger->Error("Could not create the images of the render target '{}'", _name);
      CleanUp();
      return false;
    }

    VkFramebufferCreateInfo framebuffer{};
    framebuffer.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer.renderPass = render_pass;
    framebuffer.attachmentCount = 1;
    framebuffer.pAttachments = &_attachment_view;
    framebuffer.width = width;
    framebuffer.height = height;
    framebuffer.layers = 1;

    if (vkCreateFramebuffer(device, &framebuffer, nullptr, &_framebuffer) != VK_SUCCESS)
    {
      _logger->Error("Could not create the framebuffer of the render target '{}'", _name);
      CleanUp();
      return false;
    }

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(_device->PhysicalDevice(), &features);

    VkSamplerCreateInfo sampler{};
    sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler.magFilter = VK_FILTER_LINEAR;
    sampler.minFilter = VK_FILTER_LINEAR;
    sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler.anisotropyEnable = features.samplerAnisotropy;
    sampler.maxAnisotropy = features.samplerAnisotropy
      ? std::min(8.0f, _device->Properties().limits.maxSamplerAnisotropy)
      : 1.0f;
    sampler.maxLod = static_cast<float>(_mip_levels);

    if (vkCreateSampler(device, &sampler, nullptr, &_sampler) != VK_SUCCESS)
    {
      _logger->Error("Could not create the sampler of the render target '{}'", _name);
      CleanUp();
      return false;
    }

    // What shows the target before it was drawn to reads an image, and
    // not what happens to be in the memory.
    const VkCommandBuffer commands = _device->BeginCommands();

    VK_Device::TransitionImage(
      commands, _color_image, color, 0, _mip_levels,
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    constexpr VkClearColorValue nothing{{0.0f, 0.0f, 0.0f, 0.0f}};
    const VkImageSubresourceRange all{color, 0, _mip_levels, 0, 1};
    vkCmdClearColorImage(commands, _color_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &nothing, 1, &all);

    VK_Device::TransitionImage(
      commands, _color_image, color, 0, _mip_levels,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    if (!_device->EndCommands(commands))
    {
      _logger->Error("Could not clear the render target '{}'", _name);
      CleanUp();
      return false;
    }

    return true;
  }

  bool VK_RenderTarget::PrepareScene(const VkRenderPass scene_pass, const VkFormat depth_format)
  {
    if (_scene.IsReady()) { return true; }

    if (!_scene.Initialize(_device, _extent.width, _extent.height, scene_pass, depth_format))
    {
      _logger->Error("Could not create the scene image of the render target '{}'", _name);
      return false;
    }
    return true;
  }

  VkFormat VK_RenderTarget::SampledFormatOf(const VkFormat format)
  {
    switch (format)
    {
      case VK_FORMAT_R8G8B8A8_UNORM: return VK_FORMAT_R8G8B8A8_SRGB;
      case VK_FORMAT_B8G8R8A8_UNORM: return VK_FORMAT_B8G8R8A8_SRGB;
      default: return format;
    }
  }

  void VK_RenderTarget::Finish(const VkCommandBuffer commands) const
  {
    constexpr VkImageAspectFlags color = VK_IMAGE_ASPECT_COLOR_BIT;

    // The render pass left the image itself ready to be copied from. Its
    // smaller copies hold what they held, which is about to be replaced.
    if (_mip_levels > 1)
    {
      VK_Device::TransitionImage(
        commands, _color_image, color, 1, _mip_levels - 1,
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    }

    auto level_width = static_cast<int32_t>(_extent.width);
    auto level_height = static_cast<int32_t>(_extent.height);

    // each level is scaled down from the one above it
    for (uint32_t level = 1; level < _mip_levels; level++)
    {
      const int32_t next_width = std::max(level_width / 2, 1);
      const int32_t next_height = std::max(level_height / 2, 1);

      VkImageBlit blit{};
      blit.srcSubresource = {color, level - 1, 0, 1};
      blit.srcOffsets[1] = {level_width, level_height, 1};
      blit.dstSubresource = {color, level, 0, 1};
      blit.dstOffsets[1] = {next_width, next_height, 1};

      vkCmdBlitImage(
        commands,
        _color_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        _color_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &blit, VK_FILTER_LINEAR);

      VK_Device::TransitionImage(
        commands, _color_image, color, level, 1,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

      level_width = next_width;
      level_height = next_height;
    }

    VK_Device::TransitionImage(
      commands, _color_image, color, 0, _mip_levels,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }

  void VK_RenderTarget::CleanUp()
  {
    if (_device == nullptr || _device->Device() == VK_NULL_HANDLE) { return; }

    const VkDevice device = _device->Device();

    if (_sampler != VK_NULL_HANDLE) { vkDestroySampler(device, _sampler, nullptr); }
    if (_framebuffer != VK_NULL_HANDLE) { vkDestroyFramebuffer(device, _framebuffer, nullptr); }
    _scene.CleanUp();
    if (_view != VK_NULL_HANDLE) { vkDestroyImageView(device, _view, nullptr); }
    if (_bytes_view != VK_NULL_HANDLE) { vkDestroyImageView(device, _bytes_view, nullptr); }
    if (_attachment_view != VK_NULL_HANDLE) { vkDestroyImageView(device, _attachment_view, nullptr); }
    if (_color_image != VK_NULL_HANDLE) { vkDestroyImage(device, _color_image, nullptr); }
    if (_color_memory != VK_NULL_HANDLE) { vkFreeMemory(device, _color_memory, nullptr); }

    _sampler = VK_NULL_HANDLE;
    _framebuffer = VK_NULL_HANDLE;
    _view = VK_NULL_HANDLE;
    _bytes_view = VK_NULL_HANDLE;
    _attachment_view = VK_NULL_HANDLE;
    _color_image = VK_NULL_HANDLE;
    _color_memory = VK_NULL_HANDLE;
  }
} // neon
