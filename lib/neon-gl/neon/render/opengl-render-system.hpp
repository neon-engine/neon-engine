#ifndef OPEN_GL_RENDER_SYSTEM_HPP
#define OPEN_GL_RENDER_SYSTEM_HPP

#include <memory>

#include <neon/render/render-system.hpp>
#include <neon/application/settings-config.hpp>
#include <neon/logging/logger.hpp>

#include "opengl-material.hpp"
#include "opengl-model.hpp"

namespace neon
{
  // ReSharper disable once CppInconsistentNaming
  class OpenGL_RenderSystem final : public RenderSystem
  {
    RenderResolution _render_resolution;
    DataBuffer<OpenGL_Model> _model_refs;
    DataBuffer<OpenGL_Material> _material_refs;

  public:
    explicit OpenGL_RenderSystem(
      WindowContext *window_context,
      const SettingsConfig &settings_config,
      const std::shared_ptr<Logger> &logger)
      : RenderSystem(window_context, settings_config, 4096, logger),
        _render_resolution(_settings_config.width, _settings_config.height),
        _model_refs(4096),
        _material_refs(4096) {}

    void Initialize() override;

    void CleanUp() override;

    void PrepareFrame() override;

    const RenderResolution &GetRenderResolution() override;

    int CreateRenderObject(const RenderInfo &render_info) override;

    void DrawRenderObject(
      int render_object_id,
      const Transform &transform,
      const glm::mat4 &view,
      const glm::mat4 &projection,
      const std::vector<LightSource> &lights) override;

    void DestroyRenderObject(int render_object_id) override;
  };
} // neon

#endif //OPEN_GL_RENDER_SYSTEM_HPP
