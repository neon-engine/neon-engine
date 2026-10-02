#include "vk-samplers.hpp"

#include <algorithm>

namespace neon
{
  bool VK_Samplers::Initialize(VK_Device *device, const std::shared_ptr<Logger> &logger)
  {
    _device = device;
    _logger = logger;

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(_device->PhysicalDevice(), &features);
    const float max_anisotropy = features.samplerAnisotropy
      ? _device->Properties().limits.maxSamplerAnisotropy
      : 0.0f;

    for (int i = 0; i < kSampling_Count; i++)
    {
      const VkSamplerCreateInfo description = DescriptionOf(static_cast<VK_Sampling>(i), max_anisotropy);
      if (vkCreateSampler(_device->Device(), &description, nullptr, &_samplers[i]) != VK_SUCCESS)
      {
        _logger->Critical("Could not create the samplers the textures are read through");
        CleanUp();
        return false;
      }
    }
    return true;
  }

  void VK_Samplers::CleanUp()
  {
    if (_device == nullptr || _device->Device() == VK_NULL_HANDLE) { return; }

    for (auto &sampler : _samplers)
    {
      if (sampler != VK_NULL_HANDLE) { vkDestroySampler(_device->Device(), sampler, nullptr); }
      sampler = VK_NULL_HANDLE;
    }
  }

  VkSampler VK_Samplers::Of(const VK_Sampling sampling) const
  {
    return _samplers[static_cast<int>(sampling)];
  }

  VkSamplerCreateInfo VK_Samplers::DescriptionOf(const VK_Sampling sampling, const float max_anisotropy)
  {
    const bool repeats = sampling == VK_Sampling::AnisotropicRepeat || sampling == VK_Sampling::LinearRepeat;
    const bool from_the_side = max_anisotropy > 0.0f &&
      (sampling == VK_Sampling::AnisotropicRepeat || sampling == VK_Sampling::AnisotropicClamp);
    const bool pixel_by_pixel = sampling == VK_Sampling::NearestClamp;

    const VkFilter filter = pixel_by_pixel ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    const VkSamplerAddressMode address_mode = repeats
      ? VK_SAMPLER_ADDRESS_MODE_REPEAT
      : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

    VkSamplerCreateInfo description{};
    description.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    description.magFilter = filter;
    description.minFilter = filter;
    description.mipmapMode = pixel_by_pixel ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
    description.addressModeU = address_mode;
    description.addressModeV = address_mode;
    description.addressModeW = address_mode;
    description.anisotropyEnable = from_the_side ? VK_TRUE : VK_FALSE;
    description.maxAnisotropy = from_the_side ? std::min(kMax_Anisotropy, max_anisotropy) : 1.0f;

    // every smaller copy a texture has is read, however many that is
    description.maxLod = pixel_by_pixel ? 0.0f : VK_LOD_CLAMP_NONE;
    return description;
  }
} // neon
