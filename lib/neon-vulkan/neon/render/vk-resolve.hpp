#ifndef VK_RESOLVE_HPP
#define VK_RESOLVE_HPP

#include <array>
#include <memory>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/render/tonemapper.hpp>

#include "vk-device.hpp"
#include "vk-samplers.hpp"
#include "vk-shader.hpp"

namespace neon
{
  /// The resolve step: the one place where the linear light of a scene
  /// becomes the colours of the image that is shown. A shader that covers
  /// the image reads the scene image pixel by pixel, multiplies the
  /// exposure in, maps it through the tonemapper, and writes it in sRGB.
  ///
  /// What changes how light looks on a screen belongs here or just before
  /// it: light that bleeds around what is bright is added to the scene
  /// image before the resolve. The tonemapper and the exposure are
  /// settings of the run, handed to the shader in one small uniform
  /// buffer, see Data.
  // ReSharper disable once CppInconsistentNaming
  class VK_Resolve
  {
    VK_Device *_device = nullptr;
    const VK_Samplers *_samplers = nullptr;
    std::shared_ptr<Logger> _logger;

    VK_Shader _shader;
    VkBuffer _data_buffer = VK_NULL_HANDLE;
    VkDeviceMemory _data_memory = VK_NULL_HANDLE;
    VkDescriptorSetLayout _descriptor_layout = VK_NULL_HANDLE;
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;

  public:
    static constexpr const char *kShader_Path = "engine://shaders/resolve";

    /// The bindings of the one set of the shader, which have to match
    /// resolve.frag: the scene image, the sampler it is read through,
    /// bound apart, see shaders.md, and the settings of the step.
    static constexpr uint32_t kImage_Binding = 0;
    static constexpr uint32_t kSampler_Binding = 1;
    static constexpr uint32_t kData_Binding = 2;

    static constexpr std::array<VkDescriptorSetLayoutBinding, 3> kBindings{{
      {kImage_Binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kSampler_Binding, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kData_Binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
    }};

    /// What the shader is told about the run, in the block `ResolveData`
    /// of resolve.frag: one vec4, so that the layout rules of the shading
    /// language add no padding.
    struct Data
    {
      /// x the tonemapper, as `Tonemapper` numbers them; y the exposure.
      /// z and w are not read.
      float tonemapper = 0.0f;
      float exposure = 1.0f;
      float unused_z = 0.0f;
      float unused_w = 0.0f;
    };

    static_assert(sizeof(Data) == 16);

    /// Makes the pipeline for `render_pass`, which writes the image that
    /// is shown, and room for `max_images` scene images to be read. They
    /// are read pixel by pixel through one of `samplers`. Every scene
    /// image is mapped through `tonemapper` at `exposure`, which hold for
    /// the run.
    bool Initialize(
      VK_Device *device,
      FileSystemContext *file_system_context,
      VkRenderPass render_pass,
      const VK_Samplers *samplers,
      uint32_t max_images,
      Tonemapper tonemapper,
      float exposure,
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
