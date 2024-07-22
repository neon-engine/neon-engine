#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "settings-config.hpp"
#include "input/input-system.hpp"
#include "render/render-system.hpp"
#include "window/window-system.hpp"

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
      WindowSystem *window_system,
      InputSystem *input_system,
      RenderSystem *render_system);

    void Initialize() const;

    void CleanUp();

  public:
    virtual ~Application();

    virtual void Run();
  };
} // core

#endif //APPLICATION_HPP
