#ifndef APPLICATION_HPP
#define APPLICATION_HPP
#include "input-manager.hpp"
#include "renderer.hpp"
#include "window-manager.hpp"

namespace core
{
  class Application
  {
  protected:
    WindowManager &_window_manager;
    InputManager &_input_manager;
    Renderer &_renderer;

    ~Application() = default;

  public:
    explicit Application(
      WindowManager &window_manager,
      InputManager &input_manager,
      Renderer &renderer) : _window_manager(window_manager),
                            _input_manager(input_manager),
                            _renderer(renderer) {}

    virtual void Run() = 0;
  };
} // core

#endif //APPLICATION_HPP
