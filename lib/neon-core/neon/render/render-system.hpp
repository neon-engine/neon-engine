#ifndef RENDER_SYSTEM_HPP
#define RENDER_SYSTEM_HPP

#include <string>

#include "render-context.hpp"
#include "neon/runtime/settings-config.hpp"
#include "neon/filesystem/file-system-context.hpp"
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
    FileSystemContext *_file_system_context;
    SettingsConfig _settings_config;
    std::shared_ptr<Logger> _logger;

  public:
    explicit RenderSystem(
      WindowContext *window_context,
      FileSystemContext *file_system_context,
      const SettingsConfig &settings_config,
      const int max_render_objects,
      const std::shared_ptr<Logger> &logger)
      : RenderContext(max_render_objects, logger)
    {
      _window_context = window_context;
      _file_system_context = file_system_context;
      _settings_config = settings_config;
      _logger = logger;
    }

  protected:
    ~RenderSystem() = default;

  public:
    virtual void Initialize() = 0;

    virtual void CleanUp() = 0;

    /// Called once before anything is drawn in a frame.
    virtual void PrepareFrame() = 0;

    /// Called once after everything in a frame has been drawn. A backend that
    /// collects its drawing and submits it at the end does so here.
    virtual void FinishFrame() = 0;

    /// Saves the last finished frame as a PNG image at a virtual path, such as
    /// `user://screenshots/frame.png`. Returns false if it could not be saved.
    virtual bool CaptureFrame(const std::string &path) = 0;
  };
} // neon

#endif //RENDER_SYSTEM_HPP
