#include <iostream>
#include <stdexcept>

#include "neon-fps-application.hpp"
#include "neon/core/input/sdl-2-input-system.hpp"
#include "neon/core/render/forward-render-pipeline.hpp"
#include "neon/core/render/opengl-render-system.hpp"
#include "neon/core/window/sdl2-window-system.hpp"

int main()
{
  const auto settings_config = SettingsConfig{
    .width = 1920, .height = 1080, .selected_api = RenderingApi::OpenGl
  };

  core::LoggingSystem logging_system(settings_config);
  logging_system.Initialize();
  core::SDL2_WindowSystem window_system(settings_config, logging_system.CreateLogger("SDL2_WindowSystem"));
  core::SDL2_InputSystem input_system(settings_config, &window_system, logging_system.CreateLogger("SDL2_InputSystem"));
  core::OpenGL_RenderSystem render_system(&window_system, settings_config, logging_system.CreateLogger("OpenGL_RenderSystem"));
  core::Forward_RenderPipeline render_pipeline(
    &render_system, settings_config.max_light_sources, logging_system.CreateLogger("Forward_RenderPipeline"));

  const auto app_logger = logging_system.CreateLogger("NeonFpsApplication");

  NeonFpsApplication app(
    settings_config,
    &window_system,
    &input_system,
    &render_system,
    &render_pipeline,
    &logging_system,
    app_logger);

  try
  {
    app.Run();
  } catch (const std::exception &e)
  {
    app_logger->Critical(e.what());
    app.CleanUp();
  }
}
