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

    // the most samples the graphics card takes from the side, 0 when it
    // cannot; and what the samplers are made with now
    float _most = 0.0f;
    float _anisotropy = 0.0f;

    /// Makes the sampler of a way of reading with the anisotropy there is.
    bool Make(VK_Sampling sampling);

  public:
    /// Makes every sampler, those that read from the side with `anisotropy`
    /// samples, see Anisotropy, held to what the graphics card allows.
    /// Returns false when one could not be made.
    bool Initialize(VK_Device *device, int anisotropy, const std::shared_ptr<Logger> &logger);

    void CleanUp();

    /// Makes the samplers that read from the side again with `anisotropy`
    /// samples; the others stay as they are. Nothing may read through them
    /// while this runs, and what holds them has to be told of the new ones.
    /// Returns false when one could not be made.
    bool SetAnisotropy(int anisotropy);

    /// The samples a sampler takes from the side, as the graphics card
    /// allows: 1 for none.
    [[nodiscard]] int GetAnisotropy() const;

    /// The sampler of a way of reading. VK_NULL_HANDLE until Initialize().
    [[nodiscard]] VkSampler Of(VK_Sampling sampling) const;

    /// Whether a sampler of a way of reading reads from the side, and is
    /// made again when the anisotropy changes.
    [[nodiscard]] static bool ReadsFromTheSide(VK_Sampling sampling);

    /// The samples to take from the side for `asked` of Anisotropy, where
    /// the graphics card takes at most `most`, or 0 when it cannot read a
    /// surface seen from the side. 0 for none.
    [[nodiscard]] static float LevelOf(int asked, float most);

    /// What a sampler of a way of reading is made with. `anisotropy` is
    /// the samples it takes from the side, see LevelOf(), 0 for none.
    [[nodiscard]] static VkSamplerCreateInfo DescriptionOf(VK_Sampling sampling, float anisotropy);
  };
} // neon

#endif //VK_SAMPLERS_HPP
