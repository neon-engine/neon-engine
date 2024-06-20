#include "window-system.hpp"

#include "glfw-window-system.hpp"

namespace core
{
  WindowSystem::WindowSystem(const SettingsConfig &settings_config)
  {
    _settings_config = settings_config;
  }

  void WindowSystem::Initialize()
  {
    Glfw_WindowSystem::InitializeWindow(_settings_config);
  }

  bool WindowSystem::IsRunning() const
  {
    return Glfw_WindowSystem::IsRunning();
  }

  void WindowSystem::CleanUp()
  {
    glfwTerminate();
  }
} // core
