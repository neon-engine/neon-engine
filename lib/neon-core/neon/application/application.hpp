#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "settings-config.hpp"
#include "neon/input/input-system.hpp"
#include "neon/logging/logging-system.hpp"
#include "neon/oop-system/scene-graph/scene-manager.hpp"
#include "neon/render/render-pipeline.hpp"
#include "neon/render/render-system.hpp"
#include "neon/window/window-system.hpp"

namespace neon
{
  class Application
  {
    bool _destroyed = false;

  protected:
    WindowSystem *_window_system;
    InputSystem *_input_system;
    RenderSystem *_render_system;
    RenderPipeline *_render_pipeline;
    LoggingSystem *_logging_system;
    std::shared_ptr<Logger> _logger;

    SceneManager _scene_manager;

    Application(
      const SettingsConfig &settings_config,
      WindowSystem *window_system,
      InputSystem *input_system,
      RenderSystem *render_system,
      RenderPipeline *render_pipeline,
      LoggingSystem *logging_system,
      const std::shared_ptr<Logger> &logger);

  public:
    virtual ~Application();

    virtual void Run();

    void Initialize() const;

    void CleanUp();
  };
} // neon

#endif //APPLICATION_HPP
