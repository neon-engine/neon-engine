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
    size_t vertices_size{};
    GLuint nvbo = 0;
    size_t normals_size{};
    GLuint uvbo = 0;
    size_t uvs_size{};
    GLuint ebo = 0;
    size_t indices_size{};
  };

  // ReSharper disable once CppInconsistentNaming
  struct OpenGL_Material
  {
    GLuint shader_program_id;
    size_t texture_ref_size;
    GLuint* texture_references;
  };

  // ReSharper disable once CppInconsistentNaming
  class OpenGL_RenderSystem final : public RenderSystem
  {
    const uint _max_buff_size = 65536;
    std::vector<OpenGL_Geometry> _geometry_references;
    std::vector<OpenGL_Material> _material_references;
    GLint _max_texture_units = 0;

  public:
    explicit OpenGL_RenderSystem(const SettingsConfig &settings_config)
      : RenderSystem(settings_config),
        _geometry_references(_max_buff_size),
        _material_references(_max_buff_size) {}

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

    int InitMaterial(std::string shader_path, std::string texture_paths[]) override;

    void UseMaterial(int material_id) override;

    void DestroyMaterial(int material_id) override;

    static GLuint InitShader(const std::string &shader_path);

    static void DestroyShader(GLuint shader_id);

    static GLuint InitTexture(const std::string &texture_path);

    static void DestroyTexture(GLuint texture_id);

  private:
    static std::string GetFileExtension(const std::string& filename);
  };
} // core

#endif //OPEN_GL_RENDER_SYSTEM_HPP
