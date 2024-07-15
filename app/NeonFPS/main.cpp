#include <iostream>
#include <stdexcept>
#include "core/glfw-input-system.hpp"
#include "core/settings-config.hpp"
#include "neon-fps-application.hpp"
#include "core/glfw-window-system.hpp"
#include "core/opengl-render-system.hpp"

int main()
{
  const auto settings_config = SettingsConfig{
    .width = 800, .height = 600, .selected_api = RenderingApi::OpenGl
  };

  core::Glfw_WindowSystem window_system(settings_config);
  core::Glfw_InputSystem input_system(settings_config, window_system.GetWindow());
  core::OpenGl_RenderSystem render_system(settings_config);

  NeonFpsApplication app(settings_config, window_system, input_system, render_system);

  try
  {
    app.Run();
  } catch (const std::exception &e)
  {
    std::cerr << e.what() << std::endl;
  }
}
