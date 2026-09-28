#include <neon/filesystem/sdl2-file-system.hpp>
#include <neon/input/sdl2-input-system.hpp>
#include <neon/render/forward-render-pipeline.hpp>
#include <neon/render/gl-render-system.hpp>
#include <neon/window/sdl2-window-system.hpp>

#include "neon-fps-application.hpp"

// SDL2 provides the real platform entry point (WinMain on Windows) through
// SDL2main and renames main to SDL_main behind this include. It requires the
// full argc/argv signature.
#include <SDL_main.h>

int main(int /*argc*/, char * /*argv*/[])
{
  const auto settings_config = SettingsConfig{
    .width = 1920,
    .height = 1080,
    .selected_api = RenderingApi::OpenGl,
    .window_mode = WindowMode::Borderless
  };

  neon::LoggingSystem logging_system(settings_config);

  logging_system.Initialize();

  // everything that loads files depends on the file system, so it comes up
  // before the other systems are created
  neon::SDL2_FileSystem file_system(settings_config, logging_system.CreateLogger("SDL2_FileSystem"));
  file_system.Initialize();

  neon::SDL2_WindowSystem window_system(settings_config, logging_system.CreateLogger("SDL2_WindowSystem"));
  neon::SDL2_InputSystem input_system(settings_config, &window_system, logging_system.CreateLogger("SDL2_InputSystem"));
  neon::GL_RenderSystem render_system(
    &window_system,
    &file_system,
    settings_config,
    logging_system.CreateLogger("OpenGL_RenderSystem"));
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
  }

  // CleanUp is safe to call more than once. Doing it here guarantees the
  // systems shut down before the file system they depend on.
  app.CleanUp();
  file_system.CleanUp();

  return 0;
}
