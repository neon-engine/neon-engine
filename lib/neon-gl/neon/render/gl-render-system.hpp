#ifndef GL_RENDER_SYSTEM_HPP
#define GL_RENDER_SYSTEM_HPP

#include <memory>
#include <optional>

#include <neon/render/render-system.hpp>
#include <neon/application/settings-config.hpp>
#include <neon/logging/logger.hpp>

#include "gl-material.hpp"
#include "gl-model.hpp"

namespace neon
{
  // ReSharper disable once CppInconsistentNaming
  class GL_RenderSystem final : public RenderSystem
  {
    // set in Initialize(), once the window exists and its real size is known
    std::optional<RenderResolution> _render_resolution;
    DataBuffer<GL_Model> _model_refs;
    DataBuffer<GL_Material> _material_refs;

  public:
    explicit GL_RenderSystem(
      WindowContext *window_context,
      FileSystemContext *file_system_context,
      const SettingsConfig &settings_config,
      const std::shared_ptr<Logger> &logger)
      : RenderSystem(window_context, file_system_context, settings_config, 4096, logger),
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

#endif //GL_RENDER_SYSTEM_HPP
