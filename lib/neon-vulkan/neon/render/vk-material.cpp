#include "vk-material.hpp"

#include <utility>

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

    for (const auto &texture_path : _texture_paths)
    {
      VK_Texture texture(texture_path, _file_system_context, _device, _logger);
      if (!texture.Initialize())
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

  void VK_Material::CleanUp()
  {
    _logger->Info("Cleaning up vulkan material");

    while (!_textures.empty())
    {
      _textures.back().CleanUp();
      _textures.pop_back();
    }
    _initialized = false;
  }

  VK_ObjectData VK_Material::GetObjectData(const glm::mat4 &model, const Transform &transform) const
  {
    const auto [red, green, blue, alpha] = _material_info.color;

    VK_ObjectData data;
    data.model = model;
    data.normal_matrix = glm::transpose(glm::inverse(model));
    data.color = {red, green, blue, alpha};
    data.texture_scale = _scale_textures
      ? glm::vec4(GetMaxPositiveComponents(transform.scale), 0.0f, 0.0f)
      : glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);
    data.material = {_material_info.shininess, _material_info.use_textures ? 1.0f : 0.0f, 0.0f, 0.0f};
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
