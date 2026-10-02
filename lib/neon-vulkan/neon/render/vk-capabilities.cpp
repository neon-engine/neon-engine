#include "vk-capabilities.hpp"

#include <algorithm>
#include <format>

#include "vk-api-version.hpp"

namespace neon
{
  // Helpers of VK_Capabilities, for this file alone.
  namespace
  {
    bool has_features(const VkFormatProperties &format, const VkFormatFeatureFlags wanted)
    {
      return (format.optimalTilingFeatures & wanted) == wanted;
    }

    const char *name_of(const PresentMode mode)
    {
      switch (mode)
      {
        case PresentMode::Immediate: return "immediate";
        case PresentMode::Mailbox: return "mailbox";
        case PresentMode::Fifo: return "fifo";
        case PresentMode::FifoRelaxed: return "fifo relaxed";
      }
      return "unknown";
    }

    const char *yes_or_no(const bool value)
    {
      return value ? "yes" : "no";
    }
  }

  RenderCapabilities VK_Capabilities::Fill(const VK_DeviceAnswers &answers)
  {
    const VkPhysicalDeviceLimits &limits = answers.properties.limits;

    RenderCapabilities capabilities;
    capabilities.api_version = VK_ApiVersion::ToCore(answers.api_version);
    capabilities.device_api_version = VK_ApiVersion::ToCore(answers.properties.apiVersion);
    capabilities.device_name = answers.properties.deviceName;
    capabilities.max_texture_size = static_cast<int>(limits.maxImageDimension2D);

    // the scene is drawn with colour and depth, so both have to have them
    capabilities.max_samples = MostSamples(
      limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts);

    capabilities.max_anisotropy = answers.features.samplerAnisotropy ? limits.maxSamplerAnisotropy : 0.0f;

    // in the order of the enum, each once, so that a menu lists them the
    // same way on every machine
    for (const VkPresentModeKHR mode : answers.present_modes)
    {
      if (const auto known = PresentModeOf(mode); known && std::ranges::find(capabilities.present_modes, *known) ==
                                                           capabilities.present_modes.end())
      {
        capabilities.present_modes.push_back(*known);
      }
    }
    std::ranges::sort(capabilities.present_modes);

    // The scene is drawn into, blended for what can be seen through, and
    // read by the resolve step. Textures are read with filtering.
    capabilities.has_scene_format = has_features(
      answers.scene_format,
      VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT | VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT |
      VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
    capabilities.has_srgb_textures = has_features(
      answers.srgb_format,
      VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
    capabilities.has_mutable_format_views = answers.has_mutable_format_views;

    return capabilities;
  }

  int VK_Capabilities::MostSamples(const VkSampleCountFlags counts)
  {
    for (const VkSampleCountFlagBits count : {
           VK_SAMPLE_COUNT_64_BIT, VK_SAMPLE_COUNT_32_BIT, VK_SAMPLE_COUNT_16_BIT, VK_SAMPLE_COUNT_8_BIT,
           VK_SAMPLE_COUNT_4_BIT, VK_SAMPLE_COUNT_2_BIT
         })
    {
      // the bit of a count is the count itself
      if (counts & count) { return static_cast<int>(count); }
    }
    return 1;
  }

  std::optional<PresentMode> VK_Capabilities::PresentModeOf(const VkPresentModeKHR mode)
  {
    switch (mode)
    {
      case VK_PRESENT_MODE_IMMEDIATE_KHR: return PresentMode::Immediate;
      case VK_PRESENT_MODE_MAILBOX_KHR: return PresentMode::Mailbox;
      case VK_PRESENT_MODE_FIFO_KHR: return PresentMode::Fifo;
      case VK_PRESENT_MODE_FIFO_RELAXED_KHR: return PresentMode::FifoRelaxed;
      default: return std::nullopt;
    }
  }

  std::vector<std::string> VK_Capabilities::Describe(const RenderCapabilities &capabilities)
  {
    std::string anisotropy = "no anisotropic filtering";
    if (capabilities.max_anisotropy > 0.0f)
    {
      anisotropy = std::format("anisotropic filtering up to {:g}", capabilities.max_anisotropy);
    }

    std::string present_modes = "none without a window";
    if (!capabilities.present_modes.empty())
    {
      present_modes.clear();
      for (const PresentMode mode : capabilities.present_modes)
      {
        if (!present_modes.empty()) { present_modes += ", "; }
        present_modes += name_of(mode);
      }
    }

    return {
      std::format(
        "Textures up to {} pixels wide, anti-aliasing up to {} samples, {}",
        capabilities.max_texture_size, capabilities.max_samples, anisotropy),
      "Present modes: " + present_modes,
      std::format(
        "Scene image of R16G16B16A16_SFLOAT: {}, sRGB textures: {}, views of another format: {}",
        yes_or_no(capabilities.has_scene_format),
        yes_or_no(capabilities.has_srgb_textures),
        yes_or_no(capabilities.has_mutable_format_views))
    };
  }
} // neon
