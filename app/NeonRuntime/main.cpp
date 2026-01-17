#include <neon/input/sdl2-input-system.hpp>
#include <neon/render/forward-render-pipeline.hpp>
#include <neon/render/gl-render-system.hpp>
#include <neon/window/sdl2-window-system.hpp>

#include "neon-fps-application.hpp"

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
  const auto settings_config = SettingsConfig{
    .width = 1920, .height = 1080, .selected_api = RenderingApi::OpenGl
  };

  neon::LoggingSystem logging_system(settings_config);

  logging_system.Initialize();

  neon::SDL2_WindowSystem window_system(settings_config, logging_system.CreateLogger("SDL2_WindowSystem"));
  neon::SDL2_InputSystem input_system(settings_config, &window_system, logging_system.CreateLogger("SDL2_InputSystem"));
  neon::GL_RenderSystem render_system(&window_system, settings_config, logging_system.CreateLogger("OpenGL_RenderSystem"));
  neon::Forward_RenderPipeline render_pipeline(
    &render_system,
    settings_config.max_light_sources,
    logging_system.CreateLogger("Forward_RenderPipeline"));

  neon::SceneManager scene_manager(
    &render_pipeline,
    &input_system,
    &window_system,
    logging_system.CreateLogger("SceneManager"));

  const auto app_logger = logging_system.CreateLogger("NeonFpsApplication");

  NeonFpsApplication app(
    settings_config,
    &window_system,
    &input_system,
    &render_system,
    &render_pipeline,
    &logging_system,
    &scene_manager,
    app_logger);

  try
  {
    app.Run();
  } catch (const std::exception &e)
  {
    app_logger->Critical(e.what());
    app.CleanUp();
    return 1;
  }

  return 0;
}
