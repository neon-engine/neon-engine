#include "open-gl-material.hpp"

#include <iomanip>
#include <iostream>

#include "glm/gtc/type_ptr.hpp"

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
        std::cerr << "Could not initialize texture" << std::endl;
        return false;
      }
      std::string texture_str = "texture";
      texture_str += std::to_string(texture_unit);
      _shader.SetInt(texture_str, texture_unit);
    }

    _shader.Deactivate();
    _initialized = true;
    return true;
  }

  static std::string mat4ToString(const glm::mat4& matrix) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);

    const float* m = glm::value_ptr(matrix);

    for (int row = 0; row < 4; ++row) {
      for (int col = 0; col < 4; ++col) {
        oss << m[col * 4 + row] << " ";
      }
      oss << "\n";
    }

    return oss.str();
  }

  void OpenGL_Material::Use(const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &projection) const
  {
    // std::cout << "model:" << std::endl;
    // std::cout << mat4ToString(model) << std::endl;
    //
    // std::cout << "view:" << std::endl;
    // std::cout << mat4ToString(view) << std::endl;
    //
    // std::cout << "projection" << std::endl;
    // std::cout << mat4ToString(projection) << std::endl;

    for (auto texture_unit = 0; texture_unit < _textures.size(); texture_unit++)
    {
      auto texture = _textures[texture_unit];
      texture.Use(texture_unit);
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
