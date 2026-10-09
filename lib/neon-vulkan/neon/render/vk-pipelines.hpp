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
  /// camera and the lights, the object, three textures, the three
  /// samplers they are read through, and the shadow map with the sampler
  /// it is compared through.
  ///
  /// The pass that draws the shadow map has variants of its own, which
  /// write depth alone with the one shader of the pass, and are told
  /// apart by how they are culled.
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
    VkRenderPass _shadow_pass = VK_NULL_HANDLE;
    VkDescriptorSetLayout _descriptor_layout = VK_NULL_HANDLE;
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
    std::map<std::string, Entry> _pipelines;

    /// Makes the pipeline of a variant, or finds the one that was made.
    /// A shadow variant draws into the pass of the shadow map, depth
    /// alone, with a bias along the slope of a surface.
    bool Make(
      const std::string &key,
      const std::string &shader_path,
      AlphaMode alpha_mode,
      bool double_sided,
      bool mirrored,
      bool shadow,
      VkPipeline &pipeline);

  public:
    /// The bindings of the one set of every shader of a model, which have
    /// to match scene-data.glsl and the shaders: the scene and the object,
    /// then the textures, then the sampler of each texture two bindings
    /// on. A texture and its sampler are bound apart, so that every target
    /// of the shaders binds what the source says, see shaders.md.
    static constexpr uint32_t kScene_Binding = 0;
    static constexpr uint32_t kObject_Binding = 1;
    static constexpr uint32_t kFirst_Texture_Binding = 2;
    static constexpr uint32_t kTexture_Count = 3;
    static constexpr uint32_t kFirst_Sampler_Binding = kFirst_Texture_Binding + kTexture_Count;

    /// The textures by their place: the colors of the surface, the
    /// metallic-roughness or specular map, and what the surface gives off.
    static constexpr uint32_t kDiffuse_Texture = 0;
    static constexpr uint32_t kSecond_Texture = 1;
    static constexpr uint32_t kEmissive_Texture = 2;

    /// The shadow map of the direction light, and the sampler it is
    /// compared through, after the textures of the material: shadows.glsl.
    static constexpr uint32_t kShadow_Map_Binding = kFirst_Sampler_Binding + kTexture_Count;
    static constexpr uint32_t kShadow_Sampler_Binding = kShadow_Map_Binding + 1;

    /// The lightmap of a material and its sampler, after everything that
    /// was there before, so that no binding a shader names has moved.
    static constexpr uint32_t kLightmap_Binding = kShadow_Sampler_Binding + 1;
    static constexpr uint32_t kLightmap_Sampler_Binding = kLightmap_Binding + 1;

    static constexpr std::array<VkDescriptorSetLayoutBinding, 12> kBindings{{
      {kScene_Binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kObject_Binding, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kFirst_Texture_Binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kFirst_Texture_Binding + 1, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kFirst_Texture_Binding + 2, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kFirst_Sampler_Binding, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kFirst_Sampler_Binding + 1, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kFirst_Sampler_Binding + 2, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kShadow_Map_Binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kShadow_Sampler_Binding, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kLightmap_Binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kLightmap_Sampler_Binding, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
    }};

    /// The shader of the pass that draws the shadow map.
    static constexpr const char *kShadow_Shader_Path = "engine://shaders/shadow";

    /// The bias of the pass that draws the shadow map, along the slope of
    /// a surface: how many texels of depth, as the surface falls away
    /// from the light, a caster is pushed back by. Two cover the texels
    /// the nine-texel comparison of the shaders reaches across, so that a
    /// surface that slopes away from the light does not shadow itself.
    static constexpr float kShadow_Slope_Bias = 2.0f;

    /// Makes the layout. The pipelines are made for `scene_pass`, which
    /// draws into the scene image, and those of the shadow pass for
    /// `shadow_pass`, which draws the shadow map.
    bool Initialize(
      VK_Device *device,
      FileSystemContext *file_system_context,
      VkRenderPass scene_pass,
      VkRenderPass shadow_pass,
      const std::shared_ptr<Logger> &logger);

    void CleanUp();

    /// The pipeline of a variant: a shader, and how it covers and is
    /// culled. Returns false when its shader cannot be loaded or the
    /// pipeline cannot be made.
    bool Get(const std::string &shader_path, AlphaMode alpha_mode, bool double_sided, bool mirrored, VkPipeline &pipeline);

    /// The pipeline that draws a caster into the shadow map, culled as
    /// the material is drawn: a plane that is seen from one side casts
    /// from that side alone, and a double-sided one from both.
    bool GetShadow(bool double_sided, bool mirrored, VkPipeline &pipeline);

    [[nodiscard]] VkDescriptorSetLayout DescriptorLayout() const { return _descriptor_layout; }
    [[nodiscard]] VkPipelineLayout Layout() const { return _pipeline_layout; }
  };
} // neon

#endif //VK_PIPELINES_HPP
