#ifndef OPEN_GL_RENDER_SYSTEM_HPP
#define OPEN_GL_RENDER_SYSTEM_HPP

#include <memory>

#include "render-system.hpp"
#include "neon/core/application/settings-config.hpp"
#include "neon/core/logging/logger.hpp"

namespace core
{
  // ReSharper disable once CppInconsistentNaming
  class OpenGL_RenderSystem final : public RenderSystem
  {
    RenderResolution _render_resolution;

  public:
    explicit OpenGL_RenderSystem(
      WindowContext *window_context,
      const SettingsConfig &settings_config,
      const std::shared_ptr<Logger> &logger)
      : RenderSystem(window_context, settings_config, 4096, logger),
        _render_resolution(_settings_config.width, _settings_config.height) {}

    void Initialize() override;

    void CleanUp() override;

    void PrepareFrame() override;

    const RenderResolution &GetRenderResolution() override;

    int CreateRenderObject(const RenderInfo &render_info) override;

    void DrawRenderObject(
      int render_object_id,
      const Transform &transform,
      const glm::mat4 &view,
      const glm::mat4 &projection) override;

    void DrawRenderObject(
      int render_object_id,
      const glm::mat4 &to_world,
      const glm::mat4 &view,
      const glm::mat4 &projection) override;

    void DestroyRenderObject(int render_object_id) override;
  };
} // core

#endif //OPEN_GL_RENDER_SYSTEM_HPP
