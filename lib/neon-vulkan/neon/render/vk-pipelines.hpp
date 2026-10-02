#ifndef VK_PIPELINES_HPP
#define VK_PIPELINES_HPP

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
  /// camera and the lights, the object, and two textures.
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
