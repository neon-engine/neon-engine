#include "neon-fps-application.hpp"

NeonFpsApplication::NeonFpsApplication(
  const SettingsConfig &settings_config,
  core::WindowSystem *window_system,
  core::InputSystem *input_system,
  core::RenderSystem *render_system,
  core::RenderPipeline *render_pipeline,
  core::LoggingSystem *logging_system,
  const std::shared_ptr<core::Logger> &logger): Application(
    settings_config,
    window_system,
    input_system,
    render_system,
    render_pipeline,
    logging_system,
    logger) {}
