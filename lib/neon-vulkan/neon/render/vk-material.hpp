#ifndef VK_MATERIAL_HPP
#define VK_MATERIAL_HPP

#include <functional>
#include <optional>
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
    std::optional<TextureInfo> _model_emissive_texture;
    std::string _model_path;
    std::vector<VK_Texture> _textures{};

    // what the surface gives off, apart from the textures of its colours:
    // the one the scene names, or the model's, held as the others are
    VK_Texture _emissive_texture;
    std::string _emissive_key;
    std::string _emissive_surface;
    bool _has_emissive_texture = false;

    // Where the textures come from, which holds each once for every
    // material that reads it. Without one the material loads its own.
    VK_TextureCache *_texture_cache = nullptr;

    // for each texture the key it is held under in the cache, or empty
    // for one the material loaded itself or that a render target owns
    std::vector<std::string> _texture_keys;

    /// Loads one texture into `texture`, or takes it from the cache when
    /// there is one, and says in `key` what it is held under there. `file`
    /// is the image a model carries, or nothing.
    bool LoadTexture(
      const std::string &path,
      const std::shared_ptr<const std::vector<unsigned char>> &file,
      const VK_TextureOptions &options,
      VK_Texture &texture,
      std::string &key) const;

    /// Loads one of the textures of the colours of the surface, at its
    /// place in the list.
    bool LoadListedTexture(
      const std::string &path,
      const std::shared_ptr<const std::vector<unsigned char>> &file,
      const VK_TextureOptions &options);

    /// Loads what the surface gives off: the texture the scene names, a
    /// file or a render target, or else the model's when it has one.
    bool LoadEmissiveTexture();
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

    // the pool the set was taken from, which is where it goes back
    VkDescriptorPool _descriptor_pool = VK_NULL_HANDLE;

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
    /// are shown when the scene names none, and the texture of what the
    /// file's material gives off, shown when the scene names no
    /// `emissive_texture`. Call it before Initialize().
    void SetModelTextures(
      const std::vector<TextureInfo> &textures,
      const std::string &model_path = "",
      const std::optional<TextureInfo> &emissive_texture = std::nullopt);

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

    /// How the texture of what a surface gives off is kept: as colours.
    [[nodiscard]] static VK_TextureOptions EmissiveTextureOptions();

    [[nodiscard]] const std::string &ShaderPath() const { return _shader_path; }
    [[nodiscard]] AlphaMode GetAlphaMode() const { return _material_info.alpha_mode; }
    [[nodiscard]] const std::vector<VK_Texture> &Textures() const { return _textures; }

    /// Whether the surface gives off a texture, and the texture. Without
    /// one the emissive colour alone glows, and the shaders are told so.
    [[nodiscard]] bool HasEmissiveTexture() const { return _has_emissive_texture; }
    [[nodiscard]] const VK_Texture &EmissiveTexture() const { return _emissive_texture; }

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

    [[nodiscard]] VkDescriptorPool DescriptorPool() const { return _descriptor_pool; }

    void SetDescriptorPool(const VkDescriptorPool pool) { _descriptor_pool = pool; }
  };
} // neon

#endif //VK_MATERIAL_HPP
