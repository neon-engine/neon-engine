#ifndef VK_API_VERSION_HPP
#define VK_API_VERSION_HPP

#include <cstdint>
#include <volk.h>
#include <neon/render/api-version.hpp>

namespace neon
{
  /// Which version of Vulkan is rendered with. It is kept apart from the
  /// device so that it can be checked without a graphics card.
  ///
  /// Three parties have a say: what the settings ask for, what the loader
  /// of the driver offers, and what the graphics card offers. The version
  /// is the lowest of the three, so that nothing is used that one of them
  /// does not have.
  // ReSharper disable once CppInconsistentNaming
  struct VK_ApiVersion
  {
    /// What the engine needs. Frames are drawn with a viewport of negative
    /// height, to turn them the right way up, which Vulkan 1.0 allows only
    /// with an extension and 1.1 has as part of it.
    static constexpr uint32_t kMinimum = VK_API_VERSION_1_1;

    /// The version as Vulkan writes it. The patch is left out, since a
    /// version that is asked for has none.
    [[nodiscard]] static uint32_t FromCore(const ApiVersion &version);

    /// The version as the rest of the engine knows it, with its patch.
    [[nodiscard]] static ApiVersion ToCore(uint32_t version);

    /// The version the instance is created for: what is asked for, unless
    /// the loader offers less. A loader of Vulkan 1.0 refuses an instance of
    /// a higher version.
    [[nodiscard]] static uint32_t ForInstance(uint32_t requested, uint32_t loader);

    /// The version that is rendered with on a graphics card, from the one
    /// of the instance. Only the major and minor version count, the patch
    /// of the graphics card is left out.
    [[nodiscard]] static uint32_t ForDevice(uint32_t instance, uint32_t device);

    /// Whether the engine can render with a version.
    [[nodiscard]] static bool IsEnough(uint32_t version);
  };
} // neon

#endif //VK_API_VERSION_HPP
