#include "vk-surface-format.hpp"

namespace neon
{
  bool IsSrgbFormat(const VkFormat format)
  {
    return format == VK_FORMAT_B8G8R8A8_SRGB || format == VK_FORMAT_R8G8B8A8_SRGB;
  }

  VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &formats)
  {
    for (const auto &candidate : formats)
    {
      if (candidate.format == VK_FORMAT_B8G8R8A8_UNORM || candidate.format == VK_FORMAT_R8G8B8A8_UNORM)
      {
        return candidate;
      }
    }

    return formats.front();
  }
} // neon
