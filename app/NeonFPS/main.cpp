#include <iostream>
#include <stdexcept>
#include <core/application.hpp>
#include <core/impl/gl-renderer.hpp>

int main()
{
  core::GL_Renderer rend{};
  core::Application app{};

  try
  {
    app.Run();
    rend;
  } catch (const std::exception &e)
  {
    std::cerr << e.what() << std::endl;
  }
}
