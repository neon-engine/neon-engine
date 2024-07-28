#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "input-system.hpp"
#include "render-system.hpp"
#include "settings-config.hpp"
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
      WindowSystem *window_system,
      InputSystem *input_system,
      RenderSystem *render_system);

  public:
    virtual ~Application();

    virtual void Run();

    void Initialize() const;

    void CleanUp();
  };
} // core

#endif //APPLICATION_HPP
