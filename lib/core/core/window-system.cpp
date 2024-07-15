#include "window-system.hpp"

#include <iostream>

#include "glfw-window-system.hpp"

namespace core
{
  WindowSystem::WindowSystem(const SettingsConfig &settings_config)
  {
    _settings_config = settings_config;
  }

  void WindowSystem::Initialize()
  {
    std::cout << "initializing window" << std::endl;
    // Glfw_WindowSystem::InitializeWindow(_settings_config);
  }

  bool WindowSystem::IsRunning() const
  {
    // return Glfw_WindowSystem::IsRunning();
    return false;
  }

  void WindowSystem::CleanUp()
  {
    glfwTerminate();
  }
} // core
