#include "opengl-material.hpp"

#include <sstream>

namespace core
{
  OpenGL_Material::OpenGL_Material() = default;

  OpenGL_Material::OpenGL_Material(
    const std::string &shader_path,
    const std::vector<std::string> &texture_paths,
    const MaterialInfo &material_info,
    const bool scale_textures,
    const std::shared_ptr<Logger> &logger)
  {
    _shader = OpenGL_Shader(shader_path, logger);
    _textures = std::vector<OpenGL_Texture>();
    _material_info = material_info;
    _scale_textures = scale_textures;
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
    const Transform &transform,
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

    const auto color = _material_info.color;

    _shader.SetMat4("model", model);
    _shader.SetMat4("view", view);
    _shader.SetMat4("projection", projection);
    _shader.SetVec3("color", color.r, color.g, color.b);
    _shader.SetFloat("material.shininess", _material_info.shininess);

    if (!_scale_textures)
    {
      _shader.SetVec2("texture_scale", glm::vec2{1.0f});
    } else
    {
      _shader.SetVec2("texture_scale", GetMaxPositiveComponents(transform.scale));
    }

    int point_light_count = 0;
    int spot_light_count = 0;

    for (const auto &light : lights)
    {
      switch (light.light_type)
      {
        case LightType::Direction:
        {
          SetDirectionLight(light);
          break;
        }
        case LightType::Point:
        {
          SetPointLight(light, point_light_count++);
          break;
        }
        case LightType::SpotLight:
        {
          SetSpotLight(light, spot_light_count++);
          break;
        }
        default:
        {
          int type = static_cast<int>(light.light_type);
          _logger->Error("Unsupported light type {} in material with value {}", type);
        }
      }
    }

    _shader.SetInt("num_point_lights", point_light_count);
    _shader.SetInt("num_spot_lights", spot_light_count);
  }

  void OpenGL_Material::SetDirectionLight(const LightSource &light) const
  {
    _shader.SetVec3("direction_light.direction", light.direction);
    _shader.SetVec3("direction_light.ambient", light.ambient);
    _shader.SetVec3("direction_light.diffuse", light.diffuse);
    _shader.SetVec3("dirLight.specular", light.specular);
  }

  void OpenGL_Material::SetPointLight(const LightSource &light, size_t index) const
  {
    const std::string position = std::format("point_lights[{}].position", index);

    const std::string constant = std::format("point_lights[{}].constant", index);
    const std::string linear = std::format("point_lights[{}].linear", index);
    const std::string quadratic = std::format("point_lights[{}].quadratic", index);

    const std::string ambient = std::format("point_lights[{}].ambient", index);
    const std::string diffuse = std::format("point_lights[{}].diffuse", index);
    const std::string specular = std::format("point_lights[{}].specular", index);

    _shader.SetVec3(position, light.position);

    _shader.SetFloat(constant, light.constant);
    _shader.SetFloat(linear, light.linear);
    _shader.SetFloat(quadratic, light.quadratic);

    _shader.SetVec3(ambient, light.ambient);
    _shader.SetVec3(diffuse, light.diffuse);
    _shader.SetVec3(specular, light.specular);
  }

  void OpenGL_Material::SetSpotLight(const LightSource &light, size_t index) const
  {
    const std::string position = std::format("spot_lights[{}].position", index);
    const std::string direction = std::format("spot_lights[{}].direction", index);
    const std::string cutoff = std::format("spot_lights[{}].cutoff", index);
    const std::string outer_cutoff = std::format("spot_lights[{}].outer_cutoff", index);

    const std::string constant = std::format("spot_lights[{}].constant", index);
    const std::string linear = std::format("spot_lights[{}].linear", index);
    const std::string quadratic = std::format("spot_lights[{}].quadratic", index);

    const std::string ambient = std::format("spot_lights[{}].ambient", index);
    const std::string diffuse = std::format("spot_lights[{}].diffuse", index);
    const std::string specular = std::format("spot_lights[{}].specular", index);

    _shader.SetVec3(position, light.position);
    _shader.SetVec3(direction, light.direction);
    _shader.SetFloat(cutoff, light.cutoff);
    _shader.SetFloat(outer_cutoff, light.outer_cutoff);

    _shader.SetFloat(constant, light.constant);
    _shader.SetFloat(linear, light.linear);
    _shader.SetFloat(quadratic, light.quadratic);

    _shader.SetVec3(ambient, light.ambient);
    _shader.SetVec3(diffuse, light.diffuse);
    _shader.SetVec3(specular, light.specular);
  }

  glm::vec2 OpenGL_Material::GetMaxPositiveComponents(const glm::vec3 &vector)
  {
    float a = (vector.x > 0) ? vector.x : 0.0f;
    float b = (vector.y > 0) ? vector.y : 0.0f;
    float c = (vector.z > 0) ? vector.z : 0.0f;

    if (b > a) { std::swap(a, b); }
    if (c > a) { std::swap(a, c); }
    if (c > b) { std::swap(b, c); }

    return glm::vec2(a, b);
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
