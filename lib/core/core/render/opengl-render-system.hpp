#ifndef OPEN_GL_RENDER_SYSTEM_HPP
#define OPEN_GL_RENDER_SYSTEM_HPP

#include <GL/gl3w.h>

#include <vector>

#include "open-gl-material.hpp"
#include "open-gl-mesh.hpp"
#include "render-system.hpp"

namespace core
{

  // ReSharper disable once CppInconsistentNaming
  class OpenGL_RenderSystem final : public RenderSystem
  {
    std::vector<OpenGL_Mesh> _mesh_refs;
    std::vector<OpenGL_Material> _material_refs;
    GLint _max_texture_units = 0;

  public:
    explicit OpenGL_RenderSystem(const SettingsConfig &settings_config)
      : RenderSystem(settings_config)
    {
      _max_supported_meshes = 4096;
      _max_supported_materials = 4096;
      _mesh_refs.reserve(_max_supported_meshes);
      _material_refs.reserve(_max_supported_materials);
    }

    void Initialize() override;

    void CleanUp() override;

    void PrepareFrame() override;

    RenderContext *GetContext() override;

    int InitMesh(
      const std::vector<float> &vertices,
      const std::vector<float> &normals,
      const std::vector<float> &tex_coordinates,
      const std::vector<int> &indices) override;

    void DrawMesh(int mesh_id) override;

    void DestroyMesh(int mesh_id) override;

    int InitMaterial(std::string shader_path, std::vector<std::string> texture_paths) override;

    void UseMaterial(int material_id) override;

    void DestroyMaterial(int material_id) override;
  };
} // core

#endif //OPEN_GL_RENDER_SYSTEM_HPP
