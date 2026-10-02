#include "vk-api-version.hpp"

#include <algorithm>

namespace neon
{
  namespace
  {
    /// The major and minor version, which is what decides what may be used.
    uint32_t without_patch(const uint32_t version)
    {
      return VK_MAKE_API_VERSION(0, VK_API_VERSION_MAJOR(version), VK_API_VERSION_MINOR(version), 0);
    }
  }

  uint32_t VK_ApiVersion::FromCore(const ApiVersion &version)
  {
    return VK_MAKE_API_VERSION(0, static_cast<uint32_t>(version.major), static_cast<uint32_t>(version.minor), 0);
  }

  ApiVersion VK_ApiVersion::ToCore(const uint32_t version)
  {
    return {
      static_cast<int>(VK_API_VERSION_MAJOR(version)),
      static_cast<int>(VK_API_VERSION_MINOR(version)),
      static_cast<int>(VK_API_VERSION_PATCH(version))
    };
  }

  uint32_t VK_ApiVersion::ForInstance(const uint32_t requested, const uint32_t loader)
  {
    return std::min(without_patch(requested), without_patch(loader));
  }

  uint32_t VK_ApiVersion::ForDevice(const uint32_t instance, const uint32_t device)
  {
    return std::min(without_patch(instance), without_patch(device));
  }

  bool VK_ApiVersion::IsEnough(const uint32_t version)
  {
    return without_patch(version) >= kMinimum;
  }
} // neon
