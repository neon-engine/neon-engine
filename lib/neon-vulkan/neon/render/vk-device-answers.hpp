#ifndef VK_DEVICE_ANSWERS_HPP
#define VK_DEVICE_ANSWERS_HPP

#include <cstdint>
#include <vector>

#include <volk.h>

namespace neon
{
  /// What Vulkan says about a graphics card, as it is asked for. The device
  /// asks, and VK_Capabilities turns the answers into what the rest of the
  /// engine reads.
  // ReSharper disable once CppInconsistentNaming
  struct VK_DeviceAnswers
  {
    VkPhysicalDeviceProperties properties{};
    VkPhysicalDeviceFeatures features{};

    /// The version of Vulkan that is rendered with, see VK_ApiVersion.
    uint32_t api_version = 0;

    /// What the window offers. Empty without one.
    std::vector<VkPresentModeKHR> present_modes;

    /// What can be done with images of the scene, and with sRGB textures,
    /// in optimal tiling.
    VkFormatProperties scene_format{};
    VkFormatProperties srgb_format{};

    /// Whether an sRGB image of a render target, that is also seen as
    /// plain bytes, can be made.
    bool has_mutable_format_views = false;
  };
} // neon

#endif //VK_DEVICE_ANSWERS_HPP
