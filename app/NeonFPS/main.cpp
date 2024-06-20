#include <iostream>
#include <stdexcept>
#include <core/application.hpp>

#include "neon-fps-application.hpp"
#include "core/glfw-window-manager.hpp"
#include "core/input-manager.hpp"

int main()
{
  auto window_info = Window_Info{
    .width = 800, .height = 600, .selected_api = RenderingApi::OpenGl
  };
  core::Glfw_WindowManager window_manager(window_info);
  core::InputManager input_manager = window_manager.GetInputManager();
  core::Renderer renderer = window_manager.GetRenderer();
  NeonFpsApplication app(
    window_manager,
    input_manager,
    renderer);

  try
  {
    app.Run();
  } catch (const std::exception &e)
  {
    std::cerr << e.what() << std::endl;
  }
}
