#ifndef VK_EFFECTS_HPP
#define VK_EFFECTS_HPP

#include <array>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <glm/glm.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/render/render-context.hpp>

#include "vk-device.hpp"
#include "vk-effect-kind.hpp"
#include "vk-samplers.hpp"

namespace neon
{
  /// The effects of the cameras: fragment shaders a game brings that are
  /// run over the whole picture a camera drew, one after the other. This
  /// holds what every canvas runs them with: the render pass of each kind,
  /// the pipeline of every effect that was asked for, and what the shaders
  /// are told of the game.
  ///
  /// An effect is named as a shader is, without an extension, and read
  /// from the file with `.frag.spv` added. The vertex half is the engine's,
  /// `effect.vert`. A shader of a material never meets an effect: effects
  /// are bound to a layout of their own, effect.glsl, and are run once the
  /// scene of a camera is drawn.
  // ReSharper disable once CppInconsistentNaming
  class VK_Effects
  {
    VK_Device *_device = nullptr;
    FileSystemContext *_file_system_context = nullptr;
    const VK_Samplers *_samplers = nullptr;
    std::shared_ptr<Logger> _logger;

    VkShaderModule _vertex = VK_NULL_HANDLE;
    std::array<VkRenderPass, 2> _passes{};
    VkDescriptorSetLayout _descriptor_layout = VK_NULL_HANDLE;
    VkPipelineLayout _pipeline_layout = VK_NULL_HANDLE;
    VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;
    VkBuffer _data_buffer = VK_NULL_HANDLE;
    VkDeviceMemory _data_memory = VK_NULL_HANDLE;
    void *_data_mapped = nullptr;

    /// An effect that was asked for: its fragment half and its pipeline.
    /// One that could not be made is kept without, so that it is tried,
    /// and said, once.
    struct Effect
    {
      VkShaderModule fragment = VK_NULL_HANDLE;
      VkPipeline pipeline = VK_NULL_HANDLE;
    };

    std::map<std::pair<VK_EffectKind, std::string>, Effect> _effects;

    bool LoadModule(const std::string &path, VkShaderModule &module) const;

    bool CreatePass(VkFormat format, VkRenderPass &pass) const;

    bool CreatePipeline(VkRenderPass pass, VkShaderModule fragment, VkPipeline &pipeline) const;

  public:
    static constexpr const char *kVertex_Path = "engine://shaders/effect.vert.spv";

    /// The bindings of the one set of an effect, which have to match
    /// effect.glsl: the picture, the sampler it is read through, bound
    /// apart, see shaders.md, and what the shaders are told of the game.
    static constexpr uint32_t kImage_Binding = 0;
    static constexpr uint32_t kSampler_Binding = 1;
    static constexpr uint32_t kData_Binding = 2;

    static constexpr std::array<VkDescriptorSetLayoutBinding, 3> kBindings{{
      {kImage_Binding, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kSampler_Binding, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
      {kData_Binding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
    }};

    /// What an effect is told of the game, in the block `FrameData` of
    /// effect.glsl: what the shaders of a material read as `scene.time`
    /// and `scene.numbers`.
    struct Data
    {
      glm::vec4 time{0.0f};
      std::array<glm::vec4, RenderContext::kShader_Number_Places> numbers{};
    };

    static_assert(sizeof(Data) == 16 * (1 + RenderContext::kShader_Number_Places));

    /// Makes the render passes and the layout. `scene_format` is what the
    /// light of a scene is kept in, and `screen_format` what the image that
    /// is shown has. There is room for `max_images` pictures to be read.
    bool Initialize(
      VK_Device *device,
      FileSystemContext *file_system_context,
      const VK_Samplers *samplers,
      VkFormat scene_format,
      VkFormat screen_format,
      uint32_t max_images,
      const std::shared_ptr<Logger> &logger);

    void CleanUp();

    /// Tells the effects of the frame the time and the numbers of the game.
    void SetData(const Data &data) const;

    /// The render pass an effect of a kind draws in: one image, of the
    /// format of the kind, that is read afterwards.
    [[nodiscard]] VkRenderPass PassOf(VK_EffectKind kind) const;

    /// The pipeline of an effect of a kind, made the first time it is asked
    /// for. VK_NULL_HANDLE for one that cannot be read or made, which is
    /// said once.
    [[nodiscard]] VkPipeline Find(VK_EffectKind kind, const std::string &path);

    /// What an effect reads a picture through. VK_NULL_HANDLE when there
    /// is no room for another.
    [[nodiscard]] VkDescriptorSet Keep(VkImageView view) const;

    void Release(VkDescriptorSet set) const;

    /// Covers what is drawn to with what an effect makes of the picture
    /// `set` reads. Call it in a render pass of the kind of the pipeline,
    /// or in one that holds the same format.
    void Draw(VkCommandBuffer commands, VkPipeline pipeline, VkDescriptorSet set, VkExtent2D extent) const;
  };
} // neon

#endif //VK_EFFECTS_HPP
