#include "neon-fps-application.hpp"

NeonFpsApplication::NeonFpsApplication(
  const SettingsConfig &settings_config,
  core::WindowSystem *window_system,
  core::InputSystem *input_system,
  core::RenderSystem *render_system,
  core::RenderPipeline *render_pipeline): Application(
    settings_config,
  window_system,
  input_system,
  render_system,
  render_pipeline) {}
