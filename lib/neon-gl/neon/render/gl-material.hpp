#ifndef GL_MATERIAL_HPP
#define GL_MATERIAL_HPP
#include <vector>
#include <neon/common/transform.hpp>
#include <neon/logging/logger.hpp>
#include <neon/render/light-source.hpp>
#include <neon/render/material-info.hpp>
#include "gl-shader.hpp"
#include "gl-texture.hpp"



namespace neon
{
  // ReSharper disable once CppInconsistentNaming
  class GL_Material
  {
    bool _initialized = false;
    GL_Shader _shader{};
    std::vector<GL_Texture> _textures{};
    MaterialInfo _material_info;
    bool _scale_textures = false;
    std::shared_ptr<Logger> _logger;

    void SetDirectionLight(const LightSource &light) const;

    void SetPointLight(const LightSource &light, size_t index) const;

    void SetSpotLight(const LightSource &light, size_t index) const;

    static glm::vec2 GetMaxPositiveComponents(const glm::vec3 &vector);

  public:
    GL_Material();

    GL_Material(
      const std::string &shader_path,
      const std::vector<std::string> &texture_paths,
      const MaterialInfo &material_info,
      bool scale_textures,
      const std::shared_ptr<Logger> &logger);

    bool Initialize();

    void Use(
      const glm::mat4 &model,
      const glm::mat4 &view,
      const glm::mat4 &projection,
      const Transform &transform,
      const std::vector<LightSource> &lights) const;

    void CleanUp();
  };
} // neon

#endif //GL_MATERIAL_HPP
