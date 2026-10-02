#ifndef VK_MATERIAL_HPP
#define VK_MATERIAL_HPP

#include <functional>
#include <string>
#include <vector>
#include <neon/common/transform.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/render/material-info.hpp>
#include <neon/render/texture-info.hpp>

#include "vk-shader-data.hpp"
#include "vk-texture.hpp"
#include "vk-texture-cache.hpp"

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

    // what the model names, which is shown when the scene names no texture,
    // and the model that names it
    std::vector<TextureInfo> _model_textures;
    std::string _model_path;
    std::vector<VK_Texture> _textures{};

    // Where the textures come from, which holds each once for every
    // material that reads it. Without one the material loads its own.
    VK_TextureCache *_texture_cache = nullptr;

    // for each texture the key it is held under in the cache, or empty
    // for one the material loaded itself or that a render target owns
    std::vector<std::string> _texture_keys;

    /// Loads one texture, or takes it from the cache when there is one.
    /// `file` is the image a model carries, or nothing.
    bool LoadTexture(
      const std::string &path,
      const std::shared_ptr<const std::vector<unsigned char>> &file,
      const VK_TextureOptions &options);
    MaterialInfo _material_info;
    bool _scale_textures = false;
    bool _initialized = false;

    VK_Device *_device = nullptr;
    FileSystemContext *_file_system_context = nullptr;
    std::shared_ptr<Logger> _logger;

    VkPipeline _pipeline = VK_NULL_HANDLE;
    VkPipeline _mirrored_pipeline = VK_NULL_HANDLE;
    VkPipeline _shadow_pipeline = VK_NULL_HANDLE;
    VkPipeline _mirrored_shadow_pipeline = VK_NULL_HANDLE;
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

    /// Hands over the textures the model file at `model_path` names, which
    /// are shown when the scene names none. Call it before Initialize().
    void SetModelTextures(const std::vector<TextureInfo> &textures, const std::string &model_path = "");

    /// Says where the textures are held, so that one is loaded once for
    /// every material that reads it. Call it before Initialize().
    void SetTextureCache(VK_TextureCache *cache);

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

    /// How the texture at a place in the list of a material is kept: the
    /// first as colours, the others as numbers.
    [[nodiscard]] static VK_TextureOptions TextureOptionsFor(std::size_t index);

    [[nodiscard]] const std::string &ShaderPath() const { return _shader_path; }
    [[nodiscard]] AlphaMode GetAlphaMode() const { return _material_info.alpha_mode; }
    [[nodiscard]] const std::vector<VK_Texture> &Textures() const { return _textures; }

    /// Whether both sides are drawn. The render system settles `Model`
    /// against the file before the material is made, so `Model` here is
    /// one side.
    [[nodiscard]] bool IsDoubleSided() const { return _material_info.double_sided == DoubleSided::Always; }

    /// The pipeline that draws an object with this material. A mirrored
    /// object, one whose transform turns it inside out, takes the other one,
    /// which knows that its triangles go round the other way. That one is
    /// VK_NULL_HANDLE until the render system makes it, which it does when
    /// the first mirrored object is drawn with the material.
    [[nodiscard]] VkPipeline Pipeline(const bool mirrored = false) const
    {
      return mirrored ? _mirrored_pipeline : _pipeline;
    }
    [[nodiscard]] VkDescriptorSet DescriptorSet() const { return _descriptor_set; }

    /// The pipeline that draws an object with this material into the
    /// shadow map, and the one for a mirrored object. Each is
    /// VK_NULL_HANDLE until the render system makes it, when the first
    /// object that casts is drawn with the material.
    [[nodiscard]] VkPipeline ShadowPipeline(const bool mirrored) const
    {
      return mirrored ? _mirrored_shadow_pipeline : _shadow_pipeline;
    }

    void SetPipeline(const VkPipeline pipeline) { _pipeline = pipeline; }
    void SetMirroredPipeline(const VkPipeline pipeline) { _mirrored_pipeline = pipeline; }

    void SetShadowPipeline(const bool mirrored, const VkPipeline pipeline)
    {
      (mirrored ? _mirrored_shadow_pipeline : _shadow_pipeline) = pipeline;
    }
    void SetDescriptorSet(const VkDescriptorSet descriptor_set) { _descriptor_set = descriptor_set; }
  };
} // neon

#endif //VK_MATERIAL_HPP
