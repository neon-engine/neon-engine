#include <iostream>
#include <stdexcept>

#include "neon-fps-application.hpp"

int main()
{
  // auto window_info = Window_Info{
  //   .width = 800, .height = 600, .selected_api = RenderingApi::OpenGl
  // };
  NeonFpsApplication app{};

  try
  {
    app.Run();
  } catch (const std::exception &e)
  {
    std::cerr << e.what() << std::endl;
  }
}
