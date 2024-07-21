#include "open-gl-material.hpp"

#include <iostream>

namespace core
{
  OpenGL_Material::OpenGL_Material() = default;

  OpenGL_Material::OpenGL_Material(const std::string &shader_path, const std::vector<std::string> &texture_paths)
  {
    _shader = OpenGL_Shader(shader_path);
    _textures = std::vector<OpenGL_Texture>();

    for (auto &texture_path : texture_paths)
    {
      const auto texture = OpenGL_Texture(texture_path);
      _textures.push_back(texture);
    }
  }

  bool OpenGL_Material::Initialize()
  {
    if (_initialized)
    {
      std::cerr << "Material is already initialized" << std::endl;
      return true;
    }

    if (!_shader.Initialize())
    {
      std::cerr << "Could not initialize shader" << std::endl;
      return false;
    }

    _shader.Activate();

    for (auto texture_unit = 0; texture_unit < _textures.size(); texture_unit++)
    {
      if (auto texture = _textures[texture_unit]; !texture.Initialize())
      {
        throw std::runtime_error("Could not initialize texture");
      }
      std::string texture_str = "texture";
      texture_str += std::to_string(texture_unit);
      _shader.SetInt(texture_str, texture_unit);
    }

    _shader.Deactivate();
    _initialized = true;
    return true;
  }

  void OpenGL_Material::Use(const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &projection) const
  {
    for (auto texture_unit = 0; texture_unit < _textures.size(); texture_unit++)
    {
      _textures[texture_unit].Use(texture_unit);
    }

    _shader.Activate();

    _shader.SetMat4("model", model);
    _shader.SetMat4("view", view);
    _shader.SetMat4("projection", projection);
  }
  void OpenGL_Material::CleanUp()
  {
    std::cout << "Cleaning up opengl material" << std::endl;
    if (!_initialized) { return; }

    _shader.CleanUp();
    for (auto &texture : _textures)
    {
      texture.CleanUp();
    }
    _initialized = false;
  }
} // core
