#ifndef APPLICATION_HPP
#define APPLICATION_HPP
#include "input-system.hpp"
#include "render-system.hpp"
#include "window-system.hpp"

namespace core
{
  class Application
  {
  protected:
    Application(
      WindowSystem &window_system,
      InputSystem &input_system,
      RenderSystem &render_system);

  public:
    virtual ~Application() = default;

    virtual void Run();
  };
} // core

#endif //APPLICATION_HPP
