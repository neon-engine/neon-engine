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
    RenderResolution _render_resolution;

  public:
    explicit OpenGL_RenderSystem(const SettingsConfig &settings_config)
      : RenderSystem(settings_config, 4096),
        _render_resolution(_settings_config.width, _settings_config.height) {}

    void Initialize() override;

    void CleanUp() override;

    void PrepareFrame() override;

    const RenderResolution &GetRenderResolution() override;

    int CreateRenderObject(
      const std::string &model_path,
      const std::string &shader_path,
      const std::vector<std::string> &texture_paths,
      const Color &color) override;

    int CreateRenderObject(
      const std::vector<float> &vertices,
      const std::vector<float> &normals,
      const std::vector<float> &uvs,
      const std::vector<unsigned int> &indices,
      const std::string &shader_path,
      const std::vector<std::string> &texture_paths,
      const Color &color) override;

    void DrawRenderObject(
      int render_object_id,
      const Transform &transform,
      const glm::mat4 &view,
      const glm::mat4 &projection) override;

    void DestroyRenderObject(int render_object_id) override;
  };
} // core

#endif //OPEN_GL_RENDER_SYSTEM_HPP
