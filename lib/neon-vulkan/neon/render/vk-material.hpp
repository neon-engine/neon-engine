#ifndef VK_MATERIAL_HPP
#define VK_MATERIAL_HPP

#include <functional>
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
    /// Hands over what a render target was drawn to, by the name of the
    /// target. Returns false when there is no such target.
    using SurfaceLookup = std::function<bool(const std::string &name, VK_Texture &texture)>;

  private:
    SurfaceLookup _surface_lookup;

    // for each texture the name of the render target it shows, or empty
    // for one that is a file
    std::vector<std::string> _surface_names;

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

    /// Says where the textures come from that are no files: those whose
    /// path starts with `surface://`. Call it before Initialize().
    void SetSurfaceLookup(const SurfaceLookup &lookup);

    /// Whether one of the textures shows the render target of that name.
    [[nodiscard]] bool Shows(const std::string &surface_name) const;

    /// Whether one of the textures shows a render target at all.
    [[nodiscard]] bool ShowsSurfaces() const;

    /// Asks again for what the render targets were drawn to, which is due
    /// when one was created or destroyed. A target that is not there is
    /// shown as plain white. The descriptor set has to be written again
    /// afterwards.
    void ResolveSurfaces(const VK_Texture &fallback);

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
