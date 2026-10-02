#ifndef VK_PIPELINES_HPP
#define VK_PIPELINES_HPP

#include <array>
#include <map>
#include <memory>
#include <string>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>
#include <neon/render/material-info.hpp>

#include "vk-device.hpp"
#include "vk-shader.hpp"

namespace neon
{
  /// The pipelines that draw the models of a scene, and the layout they
  /// share. Materials that name the same shader, and cover and are culled
  /// alike, share one pipeline: there is one for every variant that is
  /// asked for, made when it is first asked for, and kept until the end.
  ///
  /// The layout is what every material's descriptor set is made to: the
  /// camera and the lights, the object, two textures, and the two samplers
  /// they are read through.
  // ReSharper disable once CppInconsistentNaming
  class VK_Pipelines
  {
    struct Entry
    {
      VK_Shader shader;
      VkPipeline pipeline = VK_NULL_HANDLE;
    };

    VK_Device *_device = nullptr;
    FileSystemContext *_file_system_context = nullptr;
    std::shared_ptr<Logger> _logger;

    VkRenderPass _scene_pass = VK_NULL_HANDLE;
    VkDescriptorSetLayout _descriptor_layout = VK_NULL_HANDLE;
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
    std::map<std::string, Entry> _pipelines;

  public:
    /// The bindings of the one set of every shader of a model, which have
    /// to match scene-data.glsl and the shaders: the scene and the object,
    /// then the textures, then the sampler of each texture two bindings
    /// on. A texture and its sampler are bound apart, so that every target
    /// of the shaders binds what the source says, see shaders.md.
    static constexpr uint32_t kScene_Binding = 0;
    static constexpr uint32_t kObject_Binding = 1;
    static constexpr uint32_t kFirst_Texture_Binding = 2;
    static constexpr uint32_t kFirst_Sampler_Binding = 4;
    static constexpr uint32_t kTexture_Count = 2;

    static constexpr std::array<VkDescriptorSetLayoutBinding, 6> kBindings{{
      {kScene_Binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kObject_Binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kFirst_Texture_Binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kFirst_Texture_Binding + 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kFirst_Sampler_Binding, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kFirst_Sampler_Binding + 1, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
    }};

    /// Makes the layout. The pipelines are made for `scene_pass`, which
    /// draws into the scene image.
    bool Initialize(
      VK_Device *device,
      FileSystemContext *file_system_context,
      VkRenderPass scene_pass,
      const std::shared_ptr<Logger> &logger);

    void CleanUp();

    /// The pipeline of a variant: a shader, and how it covers and is
    /// culled. Returns false when its shader cannot be loaded or the
    /// pipeline cannot be made.
    bool Get(const std::string &shader_path, AlphaMode alpha_mode, bool double_sided, bool mirrored, VkPipeline &pipeline);

    [[nodiscard]] VkDescriptorSetLayout DescriptorLayout() const { return _descriptor_layout; }
    [[nodiscard]] VkPipelineLayout Layout() const { return _pipeline_layout; }
  };
} // neon

#endif //VK_PIPELINES_HPP
