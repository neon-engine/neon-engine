#ifndef VK_RESOLVE_HPP
#define VK_RESOLVE_HPP

#include <memory>
#include <neon/filesystem/file-system-context.hpp>

#include "vk-device.hpp"
#include "vk-shader.hpp"

namespace neon
{
  /// The resolve step: the one place where the linear light of a scene
  /// becomes the colours of the image that is shown. A shader that covers
  /// the image reads the scene image pixel by pixel, keeps what a screen
  /// can show of it, and writes it in sRGB.
  ///
  /// What changes how light looks on a screen belongs here or just before
  /// it: light that bleeds around what is bright is added to the scene
  /// image before the resolve, and a curve that keeps what is brighter
  /// than white from turning flat replaces the plain clamp in its shader.
  // ReSharper disable once CppInconsistentNaming
  class VK_Resolve
  {
    VK_Device *_device = nullptr;
    std::shared_ptr<Logger> _logger;

    VK_Shader _shader;
    VkDescriptorSetLayout _descriptor_layout = VK_NULL_HANDLE;
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;
    VkSampler _sampler = VK_NULL_HANDLE;

  public:
    static constexpr const char *kShader_Path = "assets://shaders/resolve";

    /// Makes the pipeline for `render_pass`, which writes the image that
    /// is shown, and room for `max_images` scene images to be read.
    bool Initialize(
      VK_Device *device,
      FileSystemContext *file_system_context,
      VkRenderPass render_pass,
      uint32_t max_images,
      const std::shared_ptr<Logger> &logger);

    void CleanUp();

    /// What the resolve reads a scene image through. VK_NULL_HANDLE when
    /// there is no room for another.
    [[nodiscard]] VkDescriptorSet Keep(VkImageView scene_view) const;

    void Release(VkDescriptorSet set) const;

    /// Covers what is drawn to with the scene image `set` reads. Call it in
    /// the render pass given to Initialize().
    void Draw(VkCommandBuffer commands, VkDescriptorSet set, VkExtent2D extent) const;
  };
} // neon

#endif //VK_RESOLVE_HPP
