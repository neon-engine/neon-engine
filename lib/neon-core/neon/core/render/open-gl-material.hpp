#ifndef OPEN_GL_MATERIAL_HPP
#define OPEN_GL_MATERIAL_HPP

#include <vector>
#include <neon/core/common/color.hpp>

#include "open-gl-shader.hpp"
#include "open-gl-texture.hpp"


namespace core
{
  // ReSharper disable once CppInconsistentNaming
  class OpenGL_Material
  {
    bool _initialized = false;
    OpenGL_Shader _shader{};
    std::vector<OpenGL_Texture> _textures{};
    Color _color;
  public:
    OpenGL_Material();

    OpenGL_Material(
      const std::string &shader_path,
      const std::vector<std::string> &texture_paths,
      const Color &color);

    bool Initialize();

    void Use(const glm::mat4 &model, const glm::mat4 &view, const glm::mat4 &projection) const;

    void CleanUp();
  };
} // core

#endif //OPEN_GL_MATERIAL_HPP
