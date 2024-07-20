#ifndef OPEN_GL_RENDER_SYSTEM_HPP
#define OPEN_GL_RENDER_SYSTEM_HPP

#include <GL/gl3w.h>

#include <vector>
#include "render-system.hpp"

namespace core
{
  // ReSharper disable once CppInconsistentNaming
  struct OpenGL_Geometry
  {
    GLuint vao = 0;
    GLuint vbo = 0;
    size_t vertices_size;
    GLuint nvbo = 0;
    size_t normals_size;
    GLuint uvbo = 0;
    size_t uvs_size;
    GLuint ebo = 0;
    size_t indices_size;
  };

  // ReSharper disable once CppInconsistentNaming
  class OpenGL_RenderSystem final : public RenderSystem
  {
    const uint _max_buff_size = 65536;
    std::vector<OpenGL_Geometry> _geometry_references;
    std::vector<GLuint> _shader_references;
    std::vector<GLuint> _texture_references;

  public:
    explicit OpenGL_RenderSystem(const SettingsConfig &settings_config)
      : RenderSystem(settings_config),
        _geometry_references(_max_buff_size),
        _shader_references(_max_buff_size),
        _texture_references(_max_buff_size) {}

    void Initialize() override;

    void CleanUp() override;

    void PrepareFrame() override;

    RenderContext *GetContext() override;

    int InitGeometry(
      const std::vector<float> &vertices,
      const std::vector<float> &normals,
      const std::vector<float> &tex_coordinates,
      const std::vector<int> &indices) override;

    void DrawGeometry(int geometry_id) override;

    void DestroyGeometry(int geometry_id) override;

    int InitShader(std::string shader_path) override;

    void UseShader(int shader_id) override;

    void DestroyShader(int shader_id) override;

    int InitTexture(std::string texture_path) override;

    void UseTexture(int texture_id) override;

    void DestroyTexture(int texture_id) override;
  };
} // core

#endif //OPEN_GL_RENDER_SYSTEM_HPP
