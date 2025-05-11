#include "neon-runtime-app.hpp"

NeonRuntimeApp::NeonRuntimeApp(
  const SettingsConfig &settings_config,
  neon::WindowSystem *window_system,
  neon::InputSystem *input_system,
  neon::RenderSystem *render_system,
  neon::RenderPipeline *render_pipeline,
  neon::LoggingSystem *logging_system,
  neon::WorldSystem *world_system,
  const std::shared_ptr<neon::Logger> &logger): Application(
    settings_config,
    window_system,
    input_system,
    render_system,
    render_pipeline,
    logging_system,
    world_system,
    logger) {}
