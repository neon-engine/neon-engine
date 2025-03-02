#ifndef OPEN_GL_MATERIAL_HPP
#define OPEN_GL_MATERIAL_HPP

#include <vector>
#include <neon/core/common/color.hpp>

#include "light-source.hpp"
#include "opengl-shader.hpp"
#include "opengl-texture.hpp"
#include "neon/core/logging/logger.hpp"


namespace core
{
  // ReSharper disable once CppInconsistentNaming
  class OpenGL_Material
  {
    bool _initialized = false;
    OpenGL_Shader _shader{};
    std::vector<OpenGL_Texture> _textures{};
    Color _color;
    std::shared_ptr<Logger> _logger;

  public:
    OpenGL_Material();

    OpenGL_Material(
      const std::string &shader_path,
      const std::vector<std::string> &texture_paths,
      const Color &color,
      const std::shared_ptr<Logger> &logger);

    bool Initialize();

    void Use(
      const glm::mat4 &model,
      const glm::mat4 &view,
      const glm::mat4 &projection,
      const std::vector<LightSource> &lights) const;

    void CleanUp();
  };
} // core

#endif //OPEN_GL_MATERIAL_HPP
