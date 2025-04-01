#ifndef OPEN_GL_MATERIAL_HPP
#define OPEN_GL_MATERIAL_HPP

#include <vector>
#include <neon/core/common/color.hpp>

#include "light-source.hpp"
#include "material-info.hpp"
#include "opengl-shader.hpp"
#include "opengl-texture.hpp"
#include "neon/core/common/transform.hpp"
#include "neon/core/logging/logger.hpp"


namespace neon
{
  // ReSharper disable once CppInconsistentNaming
  class OpenGL_Material
  {
    bool _initialized = false;
    OpenGL_Shader _shader{};
    std::vector<OpenGL_Texture> _textures{};
    MaterialInfo _material_info;
    bool _scale_textures = false;
    std::shared_ptr<Logger> _logger;

    void SetDirectionLight(const LightSource &light) const;

    void SetPointLight(const LightSource &light, size_t index) const;

    void SetSpotLight(const LightSource &light, size_t index) const;

    static glm::vec2 GetMaxPositiveComponents(const glm::vec3 &vector);

  public:
    OpenGL_Material();

    OpenGL_Material(
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

#endif //OPEN_GL_MATERIAL_HPP
