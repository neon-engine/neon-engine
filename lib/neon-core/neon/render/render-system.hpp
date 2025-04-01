#ifndef RENDER_SYSTEM_HPP
#define RENDER_SYSTEM_HPP

#include "render-context.hpp"
#include "neon/application/settings-config.hpp"
#include "neon/logging/logger.hpp"
#include "neon/window/window-context.hpp"

namespace neon
{
  class RenderSystem : public RenderContext
  {
    int _mesh_index = 0;
    int _material_index = 0;

  protected:
    WindowContext *_window_context;
    SettingsConfig _settings_config;
    std::shared_ptr<Logger> _logger;

  public:
    explicit RenderSystem(
      WindowContext *window_context,
      const SettingsConfig &settings_config,
      const int max_render_objects,
      const std::shared_ptr<Logger> &logger)
      : RenderContext(max_render_objects, logger)
    {
      _window_context = window_context;
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
} // neon

#endif //RENDER_SYSTEM_HPP
