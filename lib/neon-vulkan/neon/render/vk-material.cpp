#include "vk-material.hpp"

#include <neon/render/texture-source.hpp>
#include <utility>
#include <neon/common/color-space.hpp>

#include "vk-render-target.hpp"

namespace neon
{
  VK_Material::VK_Material(
    const std::string &shader_path,
    const std::vector<std::string> &texture_paths,
    const MaterialInfo &material_info,
    const bool scale_textures,
    FileSystemContext *file_system_context,
    VK_Device *device,
    const std::shared_ptr<Logger> &logger)
  {
    _shader_path = shader_path;
    _texture_paths = texture_paths;
    _material_info = material_info;
    _scale_textures = scale_textures;
    _file_system_context = file_system_context;
    _device = device;
    _logger = logger;
  }

  bool VK_Material::Initialize()
  {
    if (_initialized)
    {
      _logger->Error("Material is already initialized");
      return true;
    }

    // the scene has the last word, the model fills in what it leaves out
    if (_texture_paths.empty())
    {
      for (std::size_t i = 0; i < _model_textures.size(); i++)
      {
        const TextureInfo &info = _model_textures[i];
        if (!LoadListedTexture(info.path, info.file, TextureOptionsFor(i)))
        {
          _logger->Error("Could not initialize the texture {} of the model", info.path);
          CleanUp();
          return false;
        }
      }
    }

    for (std::size_t i = 0; i < _texture_paths.size(); i++)
    {
      const std::string &texture_path = _texture_paths[i];

      // what a render target was drawn to is no file, and belongs to the
      // target
      if (const std::string surface = VK_RenderTarget::NameOf(texture_path); !surface.empty())
      {
        VK_Texture shown;
        if (!_surface_lookup || !_surface_lookup(surface, shown)) { shown = VK_Texture(); }

        _textures.push_back(shown);
        _surface_names.push_back(surface);
        _texture_keys.emplace_back();
        continue;
      }

      if (!LoadListedTexture(texture_path, nullptr, TextureOptionsFor(i)))
      {
        _logger->Error("Could not initialize texture");
        CleanUp();
        return false;
      }
    }

    if (!LoadEmissiveTexture())
    {
      CleanUp();
      return false;
    }

    _initialized = true;
    return true;
  }

  bool VK_Material::LoadTexture(
    const std::string &path,
    const std::shared_ptr<const std::vector<unsigned char>> &file,
    const VK_TextureOptions &options,
    VK_Texture &texture,
    std::string &key) const
  {
    texture = VK_Texture(path, _file_system_context, _device, _logger);
    key.clear();

    // an image that was made at run time is in no file: its pixels are
    // asked for by its name
    if (const TextureSource source = TextureSource::Of(path); source.kind == TextureSourceKind::Image)
    {
      const std::string &name = source.name;
      const std::shared_ptr<const ImagePixels> pixels = _image_lookup ? _image_lookup(name) : nullptr;
      if (pixels == nullptr)
      {
        _logger->Error("There is no image '{}' for the texture {}. An image is made before what shows it", name, path);
        return false;
      }

      if (_texture_cache != nullptr)
      {
        key = VK_TextureCache::KeyOf(path, "", options);
        return _texture_cache->AcquirePixels(key, path, *pixels, options, texture);
      }

      return texture.InitializeWithPixels(
        pixels->pixels.data(),
        static_cast<uint32_t>(pixels->width),
        static_cast<uint32_t>(pixels->height),
        options);
    }

    if (_texture_cache != nullptr)
    {
      // an image a model carries is known by the model as well
      key = VK_TextureCache::KeyOf(path, file != nullptr ? _model_path : "", options);
      return _texture_cache->Acquire(key, path, file, options, texture);
    }

    return file != nullptr ? texture.InitializeWithFile(*file, options) : texture.Initialize(options);
  }

  bool VK_Material::LoadListedTexture(
    const std::string &path,
    const std::shared_ptr<const std::vector<unsigned char>> &file,
    const VK_TextureOptions &options)
  {
    VK_Texture texture;
    std::string key;
    if (!LoadTexture(path, file, options, texture, key)) { return false; }

    _textures.push_back(texture);
    _surface_names.emplace_back();
    _texture_keys.push_back(key);
    return true;
  }

  bool VK_Material::LoadEmissiveTexture()
  {
    // the scene has the last word here too
    if (const std::string &path = _material_info.emissive_texture; !path.empty())
    {
      if (const std::string surface = VK_RenderTarget::NameOf(path); !surface.empty())
      {
        if (!_surface_lookup || !_surface_lookup(surface, _emissive_texture)) { _emissive_texture = VK_Texture(); }
        _emissive_surface = surface;
      } else if (!LoadTexture(path, nullptr, EmissiveTextureOptions(), _emissive_texture, _emissive_key))
      {
        _logger->Error("Could not initialize the emissive texture {}", path);
        return false;
      }
      _has_emissive_texture = true;
      return true;
    }

    if (_model_emissive_texture.has_value())
    {
      const TextureInfo &info = *_model_emissive_texture;
      if (!LoadTexture(info.path, info.file, EmissiveTextureOptions(), _emissive_texture, _emissive_key))
      {
        _logger->Error("Could not initialize the emissive texture {} of the model", info.path);
        return false;
      }
      _has_emissive_texture = true;
    }
    return true;
  }

  VK_TextureOptions VK_Material::TextureOptionsFor(const std::size_t index)
  {
    // The first texture holds the colours of the surface. The second says
    // how much each part of it shines, which is a number and not a colour.
    VK_TextureOptions options;
    options.is_color = index == 0;
    return options;
  }

  VK_TextureOptions VK_Material::EmissiveTextureOptions()
  {
    // the light a surface gives off is kept in sRGB, as an image file is
    VK_TextureOptions options;
    options.is_color = true;
    return options;
  }

  void VK_Material::CleanUp()
  {
    _logger->Info("Cleaning up vulkan material");

    while (!_textures.empty())
    {
      const std::size_t last = _textures.size() - 1;

      // what a render target was drawn to is released by the target, and
      // what the cache holds is given back to it
      const bool is_surface = last < _surface_names.size() && !_surface_names[last].empty();
      const bool is_shared = last < _texture_keys.size() && !_texture_keys[last].empty();

      if (is_shared)
      {
        _texture_cache->Release(_texture_keys[last]);
      } else if (!is_surface)
      {
        _textures.back().CleanUp();
      }
      _textures.pop_back();
    }
    _surface_names.clear();
    _texture_keys.clear();

    // what the surface gives off, held the same three ways
    if (_has_emissive_texture)
    {
      if (!_emissive_key.empty())
      {
        _texture_cache->Release(_emissive_key);
      } else if (_emissive_surface.empty())
      {
        _emissive_texture.CleanUp();
      }
    }
    _emissive_texture = VK_Texture();
    _emissive_key.clear();
    _emissive_surface.clear();
    _has_emissive_texture = false;
    _initialized = false;
  }

  void VK_Material::SetModelTextures(
    const std::vector<TextureInfo> &textures,
    const std::string &model_path,
    const std::optional<TextureInfo> &emissive_texture)
  {
    _model_textures = textures;
    _model_path = model_path;
    _model_emissive_texture = emissive_texture;
  }

  void VK_Material::SetTextureCache(VK_TextureCache *cache)
  {
    _texture_cache = cache;
  }

  void VK_Material::SetSurfaceLookup(const SurfaceLookup &lookup)
  {
    _surface_lookup = lookup;
  }

  void VK_Material::SetImageLookup(const ImageLookup &lookup)
  {
    _image_lookup = lookup;
  }

  bool VK_Material::Shows(const std::string &surface_name) const
  {
    if (!_emissive_surface.empty() && _emissive_surface == surface_name) { return true; }
    for (const auto &name : _surface_names)
    {
      if (!name.empty() && name == surface_name) { return true; }
    }
    return false;
  }

  bool VK_Material::ShowsSurfaces() const
  {
    if (!_emissive_surface.empty()) { return true; }
    for (const auto &name : _surface_names)
    {
      if (!name.empty()) { return true; }
    }
    return false;
  }

  void VK_Material::ResolveSurfaces(const VK_Texture &fallback)
  {
    const auto find = [this, &fallback](const std::string &surface_name, VK_Texture &texture)
    {
      VK_Texture shown;
      if (!_surface_lookup || !_surface_lookup(surface_name, shown)) { shown = fallback; }
      texture = shown;
    };

    for (std::size_t i = 0; i < _textures.size() && i < _surface_names.size(); i++)
    {
      if (!_surface_names[i].empty()) { find(_surface_names[i], _textures[i]); }
    }
    if (!_emissive_surface.empty()) { find(_emissive_surface, _emissive_texture); }
  }

  VK_ObjectData VK_Material::GetObjectData(const glm::mat4 &model, const Transform &transform) const
  {
    // the colour is written as a screen shows it, and lit in linear light
    const auto [red, green, blue, alpha] = SrgbToLinear(_material_info.color);

    VK_ObjectData data;
    data.model = model;
    data.normal_matrix = glm::transpose(glm::inverse(model));
    data.color = {red, green, blue, alpha};
    data.texture_scale = _scale_textures
      ? glm::vec4(GetMaxPositiveComponents(transform.scale), 0.0f, 0.0f)
      : glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);
    data.material = {
      _material_info.shininess,
      _material_info.use_textures ? 1.0f : 0.0f,
      _material_info.alpha_mode == AlphaMode::Blend ? 1.0f : 0.0f,
      0.0f};
    data.surface = {_material_info.metallic, _material_info.roughness, 0.0f, 0.0f};

    // what the surface gives off is written as a screen shows it too, and
    // its strength multiplies the light; w tells the shaders whether a
    // texture of it is bound
    const auto [glow_red, glow_green, glow_blue, glow_alpha] = SrgbToLinear(_material_info.emissive);
    const float strength = _material_info.emissive_strength;
    data.emissive = {
      glow_red * strength,
      glow_green * strength,
      glow_blue * strength,
      _has_emissive_texture ? 1.0f : 0.0f};
    return data;
  }

  // the two largest components, so that a texture repeats across the two
  // long sides of a stretched object
  glm::vec2 VK_Material::GetMaxPositiveComponents(const glm::vec3 &vector)
  {
    float a = (vector.x > 0) ? vector.x : 0.0f;
    float b = (vector.y > 0) ? vector.y : 0.0f;
    float c = (vector.z > 0) ? vector.z : 0.0f;

    if (b > a) { std::swap(a, b); }
    if (c > a) { std::swap(a, c); }
    if (c > b) { std::swap(b, c); }

    return {a, b};
  }
} // neon
