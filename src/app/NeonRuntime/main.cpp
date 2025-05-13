#include <neon/input/sdl2-input-system.hpp>
#include <neon/render/forward-render-pipeline.hpp>
#include <neon/render/gl-render-system.hpp>
#include <neon/render/vk-render-system.hpp>
#include <neon/window/sdl2-window-system.hpp>

#include "neon-runtime-app.hpp"

int main()
{
  const auto settings_config = SettingsConfig{
    .width = 1920, .height = 1080, .selected_api = RenderingApi::Vulkan
  };

  neon::LoggingSystem logging_system(settings_config);

  logging_system.Initialize();

  neon::SDL2_WindowSystem window_system(settings_config, logging_system.CreateLogger("SDL2_WindowSystem"));
  neon::SDL2_InputSystem input_system(settings_config, &window_system, logging_system.CreateLogger("SDL2_InputSystem"));
  neon::Vk_RenderSystem render_system(&window_system, settings_config, logging_system.CreateLogger("Vk_RenderSystem"));
  neon::Forward_RenderPipeline render_pipeline(
    &render_system,
    settings_config.max_light_sources,
    logging_system.CreateLogger("Forward_RenderPipeline"));

  neon::SceneManager scene_manager(
    &render_pipeline,
    &input_system,
    &window_system,
    logging_system.CreateLogger("SceneManager"));

  const auto app_logger = logging_system.CreateLogger("NeonRuntimeApp");

  NeonRuntimeApp app(
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
  }
}
