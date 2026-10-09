#ifndef VK_CAPABILITIES_HPP
#define VK_CAPABILITIES_HPP

#include <optional>
#include <string>
#include <vector>
#include <volk.h>
#include <neon/render/render-capabilities.hpp>

#include "vk-scene-image.hpp"
#include "vk-texture.hpp"
#include "vk-device-answers.hpp"

namespace neon
{

  /// Turns what Vulkan says about a graphics card into RenderCapabilities.
  /// It is kept apart from the device so that it can be checked without a
  /// graphics card.
  // ReSharper disable once CppInconsistentNaming
  struct VK_Capabilities
  {
    /// The formats the device asks about: those of the scene image and of
    /// the textures that hold colors, as the renderer makes them, so that
    /// what is checked is what is used.
    static constexpr VkFormat kSceneFormat = VK_SceneImage::kFormat;
    static constexpr VkFormat kSrgbFormat = VK_Texture::kColor_Format;

    [[nodiscard]] static RenderCapabilities Fill(const VK_DeviceAnswers &answers);

    /// The most samples of a set of sample counts, 1 for none.
    [[nodiscard]] static int MostSamples(VkSampleCountFlags counts);

    /// What a present mode of Vulkan is to the engine. Nothing for the ones
    /// that are not choices of vertical sync, such as those that present
    /// only on demand.
    [[nodiscard]] static std::optional<PresentMode> PresentModeOf(VkPresentModeKHR mode);

    /// The lines that are logged when the renderer starts.
    [[nodiscard]] static std::vector<std::string> Describe(const RenderCapabilities &capabilities);
  };
} // neon

#endif //VK_CAPABILITIES_HPP
