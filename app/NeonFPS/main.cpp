#include <iostream>
#include <stdexcept>

#include "neon-fps-application.hpp"
#include "opengl-render-system.hpp"
#include "sdl-2-input-system.hpp"
#include "sdl2-window-system.hpp"
#include "settings-config.hpp"

int main()
{
  const auto settings_config = SettingsConfig{
    .width = 1280, .height = 720, .selected_api = RenderingApi::OpenGl
  };

  core::SDL2_WindowSystem window_system(settings_config);
  core::SDL2_InputSystem input_system(settings_config, &window_system);
  core::OpenGL_RenderSystem render_system(settings_config);

  NeonFpsApplication app(settings_config, &window_system, &input_system, &render_system);

  try
  {
    app.Run();
  } catch (const std::exception &e)
  {
    std::cerr << e.what() << std::endl;
  }
}
