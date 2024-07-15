#include "application.hpp"

namespace core
{
  Application::Application(
    const SettingsConfig &settings_config,
    WindowSystem &window_system,
    InputSystem &input_system,
    RenderSystem &render_system)
  {
    _window_system = &window_system;
    _input_system = &input_system;
    _render_system = &render_system;
  }

  void Application::Initialize() const
  {
    // order here matters
    // the input system and rendering backend are usually dependent on
    // the OS windowing system. Initializing the window first will make it possible
    // to set up both the input systems and render systems

    _window_system->Initialize();
    _input_system->Initialize();
    _render_system->Initialize();
  }

  void Application::CleanUp()
  {
    if (_destroyed) { return; }
    _destroyed = true;
  }

  Application::~Application()
  {
    CleanUp();
  }

  void Application::Run()
  {
    Initialize();

    while (_window_system->IsRunning())
    {
      _input_system->ProcessInput();
    }
  }
} // core
