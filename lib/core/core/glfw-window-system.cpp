#include "glfw-window-system.hpp"

#include <stdexcept>

namespace core
{
  void Glfw_WindowSystem::ConfigureWindowForRenderer(const SettingsConfig &settings_config)
  {
    _settings_config = settings_config;
  }

  void Glfw_WindowSystem::Initialize()
  {
    if (!glfwInit())
    {
      throw std::runtime_error("Failed to initialize GLFW");
    }

    ConfigureWindowForRenderer(_settings_config);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _window = glfwCreateWindow(
      _settings_config.width,
      _settings_config.height,
      _settings_config.title.c_str(),
      nullptr,
      nullptr);

    if (_window == nullptr)
    {
      glfwTerminate();
      throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(_window);

    HideCursor();
  }
  bool Glfw_WindowSystem::IsRunning() const
  {
    return !glfwWindowShouldClose(_window);
  }

  const GLFWwindow *Glfw_WindowSystem::GetWindow() const
  {
    return _window;
  }

  void Glfw_WindowSystem::HideCursor()
  {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

  void Glfw_WindowSystem::ShowCursor()
  {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
  }

  void Glfw_WindowSystem::CleanUp()
  {
    glfwDestroyWindow(_window);
    glfwTerminate();
  }
} // core
