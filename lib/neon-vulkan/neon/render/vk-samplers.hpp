#ifndef VK_SAMPLERS_HPP
#define VK_SAMPLERS_HPP

#include <array>
#include <memory>
#include <neon/logging/logger.hpp>

#include "vk-device.hpp"
#include "vk-sampling.hpp"

namespace neon
{
  /// The samplers the render system shares: one for every way a texture is
  /// read, made once, and bound next to every texture that is read that
  /// way. A texture owns none of its own.
  // ReSharper disable once CppInconsistentNaming
  class VK_Samplers
  {
    VK_Device *_device = nullptr;
    std::shared_ptr<Logger> _logger;
    std::array<VkSampler, kSampling_Count> _samplers{};

  public:
    /// The most samples a sampler takes across a surface seen from the
    /// side, when the graphics card can.
    static constexpr float kMax_Anisotropy = 8.0f;

    /// Makes every sampler. Returns false when one could not be made.
    bool Initialize(VK_Device *device, const std::shared_ptr<Logger> &logger);

    void CleanUp();

    /// The sampler of a way of reading. VK_NULL_HANDLE until Initialize().
    [[nodiscard]] VkSampler Of(VK_Sampling sampling) const;

    /// What a sampler of a way of reading is made with. `max_anisotropy`
    /// is what the graphics card allows, or 0 when it cannot read a
    /// surface seen from the side without blurring.
    [[nodiscard]] static VkSamplerCreateInfo DescriptionOf(VK_Sampling sampling, float max_anisotropy);
  };
} // neon

#endif //VK_SAMPLERS_HPP
