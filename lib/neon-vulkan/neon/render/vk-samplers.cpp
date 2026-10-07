#include "vk-samplers.hpp"

#include <algorithm>

namespace neon
{
  bool VK_Samplers::Initialize(VK_Device *device, const int anisotropy, const std::shared_ptr<Logger> &logger)
  {
    _device = device;
    _logger = logger;

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(_device->PhysicalDevice(), &features);
    _most = features.samplerAnisotropy ? _device->Properties().limits.maxSamplerAnisotropy : 0.0f;
    _anisotropy = LevelOf(anisotropy, _most);

    for (int i = 0; i < kSampling_Count; i++)
    {
      if (!Make(static_cast<VK_Sampling>(i)))
      {
        _logger->Critical("Could not create the samplers the textures are read through");
        CleanUp();
        return false;
      }
    }

    if (anisotropy > 1 && _anisotropy < static_cast<float>(anisotropy))
    {
      const int allowed = GetAnisotropy();
      _logger->Info("Anisotropic filtering of {}x was asked for, and the graphics card allows {}x", anisotropy, allowed);
    } else
    {
      const int level = GetAnisotropy();
      _logger->Info("Textures are read with anisotropic filtering of {}x", level);
    }
    return true;
  }

  bool VK_Samplers::Make(const VK_Sampling sampling)
  {
    VkSampler &sampler = _samplers[static_cast<int>(sampling)];
    if (sampler != VK_NULL_HANDLE) { vkDestroySampler(_device->Device(), sampler, nullptr); }
    sampler = VK_NULL_HANDLE;

    const VkSamplerCreateInfo description = DescriptionOf(sampling, _anisotropy);
    if (vkCreateSampler(_device->Device(), &description, nullptr, &sampler) != VK_SUCCESS)
    {
      sampler = VK_NULL_HANDLE;
      return false;
    }
    return true;
  }

  bool VK_Samplers::SetAnisotropy(const int anisotropy)
  {
    if (_device == nullptr || _device->Device() == VK_NULL_HANDLE) { return false; }

    _anisotropy = LevelOf(anisotropy, _most);
    for (int i = 0; i < kSampling_Count; i++)
    {
      const auto sampling = static_cast<VK_Sampling>(i);
      if (ReadsFromTheSide(sampling) && !Make(sampling))
      {
        _logger->Error("Could not make the samplers that read textures from the side again");
        return false;
      }
    }

    if (anisotropy > 1 && _anisotropy < static_cast<float>(anisotropy))
    {
      const int allowed = GetAnisotropy();
      _logger->Info("Anisotropic filtering of {}x was asked for, and the graphics card allows {}x", anisotropy, allowed);
    } else
    {
      const int level = GetAnisotropy();
      _logger->Info("Textures are read with anisotropic filtering of {}x", level);
    }
    return true;
  }

  int VK_Samplers::GetAnisotropy() const
  {
    return _anisotropy > 0.0f ? static_cast<int>(_anisotropy) : 1;
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

  bool VK_Samplers::ReadsFromTheSide(const VK_Sampling sampling)
  {
    return sampling == VK_Sampling::AnisotropicRepeat || sampling == VK_Sampling::AnisotropicClamp;
  }

  float VK_Samplers::LevelOf(const int asked, const float most)
  {
    // a level the graphics card does not offer is held to the most it does
    if (asked <= 1 || most < 1.0f) { return 0.0f; }
    const float level = std::min(static_cast<float>(asked), most);
    return level > 1.0f ? level : 0.0f;
  }

  VkSamplerCreateInfo VK_Samplers::DescriptionOf(const VK_Sampling sampling, const float anisotropy)
  {
    const bool repeats = sampling == VK_Sampling::AnisotropicRepeat || sampling == VK_Sampling::LinearRepeat;
    const bool from_the_side = anisotropy > 0.0f && ReadsFromTheSide(sampling);
    const bool pixel_by_pixel = sampling == VK_Sampling::NearestClamp;
    const bool compared = sampling == VK_Sampling::ShadowCompare;

    const VkFilter filter = pixel_by_pixel ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    const VkSamplerAddressMode address_mode = repeats
      ? VK_SAMPLER_ADDRESS_MODE_REPEAT
      : compared
      ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
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
    description.maxAnisotropy = from_the_side ? anisotropy : 1.0f;

    // every smaller copy a texture has is read, however many that is
    description.maxLod = pixel_by_pixel || compared ? 0.0f : VK_LOD_CLAMP_NONE;

    // The shadow map is not read but compared: a depth is lit when it is
    // no further than what the map holds, and the linear filter blends the
    // four answers around the place. Past the edge of the map everything
    // is lit, which a border of white says.
    description.compareEnable = compared ? VK_TRUE : VK_FALSE;
    description.compareOp = compared ? VK_COMPARE_OP_LESS_OR_EQUAL : VK_COMPARE_OP_NEVER;
    description.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    return description;
  }
} // neon
