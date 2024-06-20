#include <iostream>
#include <stdexcept>
#include <core/settings-config.hpp>
#include "neon-fps-application.hpp"

int main()
{
  auto settings_config = SettingsConfig{
    .width = 800, .height = 600, .selected_api = RenderingApi::OpenGl
  };
  core::WindowSystem window_system(settings_config);
  core::InputSystem input_system(settings_config);
  core::RenderSystem render_system(settings_config);

  NeonFpsApplication app(settings_config, window_system, input_system, render_system);

  try
  {
    // app.Run();
    std::cout << "Hello World!" << std::endl;
  } catch (const std::exception &e)
  {
    std::cerr << e.what() << std::endl;
  }
}
