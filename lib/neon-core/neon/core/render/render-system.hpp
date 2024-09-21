#ifndef RENDER_SYSTEM_HPP
#define RENDER_SYSTEM_HPP

#include "open-gl-material.hpp"
#include "open-gl-mesh.hpp"
#include "render-context.hpp"
#include "neon/core/application/settings-config.hpp"
#include "neon/core/logging/logger.hpp"

namespace core
{
  class RenderSystem : public RenderContext
  {
    int _mesh_index = 0;
    int _material_index = 0;
  protected:

    DataBuffer<OpenGL_Mesh> _mesh_refs;
    DataBuffer<OpenGL_Material> _material_refs;
    SettingsConfig _settings_config;
    std::shared_ptr<Logger> _logger;

  public:
    explicit RenderSystem(
      const SettingsConfig &settings_config,
      const int max_render_objects,
      const std::shared_ptr<Logger> &logger)
    : RenderContext(max_render_objects), _mesh_refs(max_render_objects), _material_refs(max_render_objects)
    {
      _settings_config = settings_config;
      _logger = logger;
    }

  protected:
    ~RenderSystem() = default;

  public:
    virtual void Initialize() = 0;

    virtual void CleanUp() = 0;

    virtual void PrepareFrame() = 0;
  };
} // core

#endif //RENDER_SYSTEM_HPP
