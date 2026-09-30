#ifndef RUNTIME_HPP
#define RUNTIME_HPP

#include "settings-config.hpp"
#include "neon/input/input-system.hpp"
#include "neon/logging/logging-system.hpp"
#include "neon/world-system/world-system.hpp"
#include "neon/render/render-pipeline.hpp"
#include "neon/render/render-system.hpp"
#include "neon/ui/ui-system.hpp"
#include "neon/window/window-system.hpp"

namespace neon
{
  class Runtime
  {
    bool _destroyed = false;
    bool _failed = false;

  protected:
    SettingsConfig _settings_config;
    WindowSystem *_window_system;
    InputSystem *_input_system;
    RenderSystem *_render_system;
    RenderPipeline *_render_pipeline;
    LoggingSystem *_logging_system;
    WorldSystem* _world_system;
    UiSystem *_ui_system = nullptr;
    std::shared_ptr<Logger> _logger;

    Runtime(
      const SettingsConfig &settings_config,
      WindowSystem *window_system,
      InputSystem *input_system,
      RenderSystem *render_system,
      RenderPipeline *render_pipeline,
      LoggingSystem *logging_system,
      WorldSystem *world_system,
      const std::shared_ptr<Logger> &logger);

  public:
    virtual ~Runtime();

    /// Gives the runtime a user interface, which is then updated before
    /// the world and drawn on top of it. Call it before Initialize(). A
    /// runtime without one draws the world alone.
    void SetUiSystem(UiSystem *ui_system);

    virtual void Run();

    void Initialize() const;

    void CleanUp();

    /// Whether something the run was asked to do did not happen, such as a
    /// screenshot that could not be written. An application turns this into
    /// its exit code.
    [[nodiscard]] bool HasFailed() const;
  };
} // neon

#endif //RUNTIME_HPP
