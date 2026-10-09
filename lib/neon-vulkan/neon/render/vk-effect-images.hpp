#ifndef VK_EFFECT_IMAGES_HPP
#define VK_EFFECT_IMAGES_HPP

#include <array>

#include "vk-device.hpp"
#include "vk-effect-kind.hpp"
#include "vk-effects.hpp"
#include "vk-resolve.hpp"

namespace neon
{
  /// The two pictures the effects of one kind of a canvas are run between:
  /// an effect reads one and writes the other, and the next one the other
  /// way around. They are as large as the canvas and hold what the kind
  /// holds: the light of the scene, or the colors a screen is given.
  ///
  /// A canvas makes them when a camera that draws into it first names an
  /// effect, so a camera without effects costs nothing.
  // ReSharper disable once CppInconsistentNaming
  class VK_EffectImages
  {
    /// One picture, what it is drawn into through, and what it is read
    /// through: by an effect, and by the resolve step.
    struct Picture
    {
      VkImage image = VK_NULL_HANDLE;
      VkDeviceMemory memory = VK_NULL_HANDLE;
      VkImageView view = VK_NULL_HANDLE;
      VkFramebuffer framebuffer = VK_NULL_HANDLE;
      VkDescriptorSet effect_set = VK_NULL_HANDLE;
      VkDescriptorSet resolve_set = VK_NULL_HANDLE;
    };

    VK_Device *_device = nullptr;
    const VK_Effects *_effects = nullptr;
    const VK_Resolve *_resolve = nullptr;
    std::array<Picture, 2> _pictures{};
    bool _is_ready = false;

  public:
    /// Makes both pictures in `format`, for the render pass of `kind`.
    /// With a `resolve`, the resolve step can read them as well, which it
    /// does with the light of a scene. Returns false, and holds nothing,
    /// when they cannot be made.
    bool Initialize(
      VK_Device *device,
      const VK_Effects *effects,
      const VK_Resolve *resolve,
      VK_EffectKind kind,
      VkFormat format,
      VkExtent2D extent);

    void CleanUp();

    [[nodiscard]] bool IsReady() const { return _is_ready; }

    /// What picture 0 or 1 is drawn into through.
    [[nodiscard]] VkFramebuffer Framebuffer(const std::size_t picture) const { return _pictures[picture].framebuffer; }

    /// What an effect reads picture 0 or 1 through.
    [[nodiscard]] VkDescriptorSet EffectSet(const std::size_t picture) const { return _pictures[picture].effect_set; }

    /// What the resolve step reads picture 0 or 1 through. VK_NULL_HANDLE
    /// when they were made without one.
    [[nodiscard]] VkDescriptorSet ResolveSet(const std::size_t picture) const { return _pictures[picture].resolve_set; }
  };
} // neon

#endif //VK_EFFECT_IMAGES_HPP
