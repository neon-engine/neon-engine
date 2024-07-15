#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "input-system.hpp"
#include "render-system.hpp"
#include "window-system.hpp"

namespace core
{
  class Application
  {
    bool _destroyed = false;

  protected:
    WindowSystem *_window_system;
    InputSystem *_input_system;
    RenderSystem *_render_system;

    Application(
      const SettingsConfig &settings_config,
      WindowSystem &window_system,
      InputSystem &input_system,
      RenderSystem &render_system);

    void Initialize() const;

    void CleanUp();

  public:
    virtual ~Application();

    virtual void Run();
  };
} // core

#endif //APPLICATION_HPP
