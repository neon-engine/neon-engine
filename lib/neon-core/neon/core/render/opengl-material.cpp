#include "opengl-material.hpp"

namespace core
{
  OpenGL_Material::OpenGL_Material() = default;

  OpenGL_Material::OpenGL_Material(
    const std::string &shader_path,
    const std::vector<std::string> &texture_paths,
    const Color &color,
    const std::shared_ptr<Logger> &logger)
  {
    _shader = OpenGL_Shader(shader_path, logger);
    _textures = std::vector<OpenGL_Texture>();
    _color = color;
    _logger = logger;

    for (auto &texture_path : texture_paths)
    {
      const auto texture = OpenGL_Texture(texture_path, _logger);
      _textures.push_back(texture);
    }
  }

  bool OpenGL_Material::Initialize()
  {
    if (_initialized)
    {
      _logger->Error("Material is already initialized");
      return true;
    }

    if (!_shader.Initialize())
    {
      _logger->Error("Could not initialize shader");
      return false;
    }

    for (auto &texture : _textures)
    {
      if (!texture.Initialize())
      {
        _logger->Error("Could not initialize texture");
        return false;
      }
    }

    _initialized = true;
    return true;
  }

  void OpenGL_Material::Use(
    const glm::mat4 &model,
    const glm::mat4 &view,
    const glm::mat4 &projection,
    const std::vector<LightSource> &lights) const
  {
    _shader.Activate();
    for (auto texture_unit = 0; texture_unit < _textures.size(); texture_unit++)
    {
      auto texture = _textures[texture_unit];
      std::string texture_str = "texture";
      texture_str += std::to_string(texture_unit + 1);
      _shader.SetInt(texture_str, texture_unit);
      texture.Use(texture_unit);
    }

    _shader.SetMat4("model", model);
    _shader.SetMat4("view", view);
    _shader.SetMat4("projection", projection);
    _shader.SetVec3("color", _color.r, _color.g, _color.b);
  }
  void OpenGL_Material::CleanUp()
  {
    _logger->Info("Cleaning up opengl material");
    if (!_initialized) { return; }

    _shader.CleanUp();
    for (auto &texture : _textures)
    {
      texture.CleanUp();
    }
    _initialized = false;
  }
} // core
