#ifndef OPEN_GL_RENDER_SYSTEM_HPP
#define OPEN_GL_RENDER_SYSTEM_HPP

#include <vector>

#include "open-gl-material.hpp"
#include "open-gl-mesh.hpp"
#include "render-system.hpp"

namespace core
{

  // ReSharper disable once CppInconsistentNaming
  class OpenGL_RenderSystem final : public RenderSystem
  {
  public:
    explicit OpenGL_RenderSystem(const SettingsConfig &settings_config)
      : RenderSystem(settings_config, 4096)
    { }

    void Initialize() override;

    void CleanUp() override;

    void PrepareFrame() override;

    // RenderContext *GetContext() override;

    int InitMesh(std::string model_path, glm::mat4 &normalized_matrix) override;

    int InitMesh(
      const std::vector<float> &vertices,
      const std::vector<float> &normals,
      const std::vector<float> &tex_coordinates,
      const std::vector<unsigned int> &indices,
      glm::mat4 &normalized_matrix) override;

    void DrawMesh(int mesh_id) override;

    void DestroyMesh(int mesh_id) override;

    int InitMaterial(std::string shader_path, std::vector<std::string> texture_paths) override;

    void UseMaterial(
      int material_id,
      const glm::mat4 &model,
      const glm::mat4 &view,
      const glm::mat4 &projection) override;

    void DestroyMaterial(int material_id) override;

    RenderResolution GetRenderResolution() override;

    int CreateRenderObject(std::string model_path,
      std::string shader_path,
      std::vector<std::string> texture_paths) override;

    int CreateRenderObject(const std::vector<float> &vertices,
      const std::vector<float> &normals,
      const std::vector<float> &tex_coordinates,
      const std::vector<unsigned int> &indices,
      std::string shader_path,
      std::vector<std::string> texture_paths) override;

    void UseRenderObject() override;

    void DestroyRenderObject() override;
  };
} // core

#endif //OPEN_GL_RENDER_SYSTEM_HPP
