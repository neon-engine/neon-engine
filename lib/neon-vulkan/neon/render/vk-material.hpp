#ifndef VK_MATERIAL_HPP
#define VK_MATERIAL_HPP

#include <string>
#include <vector>
#include <neon/common/transform.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/render/material-info.hpp>

#include "vk-shader-data.hpp"
#include "vk-texture.hpp"

namespace neon
{
  /// What an object looks like: which shader draws it, with which textures
  /// and which settings. The render system supplies the pipeline and the
  /// descriptor set, since it owns what they are made from.
  // ReSharper disable once CppInconsistentNaming
  class VK_Material
  {
    std::string _shader_path;
    std::vector<std::string> _texture_paths;
    std::vector<VK_Texture> _textures{};
    MaterialInfo _material_info;
    bool _scale_textures = false;
    bool _initialized = false;

    VK_Device *_device = nullptr;
    FileSystemContext *_file_system_context = nullptr;
    std::shared_ptr<Logger> _logger;

    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkDescriptorSet _descriptor_set = VK_NULL_HANDLE;

    static glm::vec2 GetMaxPositiveComponents(const glm::vec3 &vector);

  public:
    VK_Material() = default;

    VK_Material(
      const std::string &shader_path,
      const std::vector<std::string> &texture_paths,
      const MaterialInfo &material_info,
      bool scale_textures,
      FileSystemContext *file_system_context,
      VK_Device *device,
      const std::shared_ptr<Logger> &logger);

    /// Loads the textures.
    bool Initialize();

    void CleanUp();

    /// Fills in what the shaders need to know about one object drawn with
    /// this material.
    [[nodiscard]] VK_ObjectData GetObjectData(const glm::mat4 &model, const Transform &transform) const;

    [[nodiscard]] const std::string &ShaderPath() const { return _shader_path; }
    [[nodiscard]] const std::vector<VK_Texture> &Textures() const { return _textures; }

    [[nodiscard]] VkPipeline Pipeline() const { return _pipeline; }
    [[nodiscard]] VkDescriptorSet DescriptorSet() const { return _descriptor_set; }

    void SetPipeline(const VkPipeline pipeline) { _pipeline = pipeline; }
    void SetDescriptorSet(const VkDescriptorSet descriptor_set) { _descriptor_set = descriptor_set; }
  };
} // neon

#endif //VK_MATERIAL_HPP
