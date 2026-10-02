#include "vk-material.hpp"

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
        continue;
      }

      _surface_names.emplace_back();

      VK_Texture texture(texture_path, _file_system_context, _device, _logger);
      if (!texture.Initialize(TextureOptionsFor(i)))
      {
        _logger->Error("Could not initialize texture");
        CleanUp();
        return false;
      }
      _textures.push_back(texture);
    }

    _initialized = true;
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

  void VK_Material::CleanUp()
  {
    _logger->Info("Cleaning up vulkan material");

    while (!_textures.empty())
    {
      // what a render target was drawn to is released by the target
      const bool is_surface = _textures.size() <= _surface_names.size() &&
                              !_surface_names[_textures.size() - 1].empty();

      if (!is_surface) { _textures.back().CleanUp(); }
      _textures.pop_back();
    }
    _surface_names.clear();
    _initialized = false;
  }

  void VK_Material::SetSurfaceLookup(const SurfaceLookup &lookup)
  {
    _surface_lookup = lookup;
  }

  bool VK_Material::Shows(const std::string &surface_name) const
  {
    for (const auto &name : _surface_names)
    {
      if (!name.empty() && name == surface_name) { return true; }
    }
    return false;
  }

  bool VK_Material::ShowsSurfaces() const
  {
    for (const auto &name : _surface_names)
    {
      if (!name.empty()) { return true; }
    }
    return false;
  }

  void VK_Material::ResolveSurfaces(const VK_Texture &fallback)
  {
    for (std::size_t i = 0; i < _textures.size() && i < _surface_names.size(); i++)
    {
      if (_surface_names[i].empty()) { continue; }

      VK_Texture shown;
      if (!_surface_lookup || !_surface_lookup(_surface_names[i], shown)) { shown = fallback; }

      _textures[i] = shown;
    }
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
