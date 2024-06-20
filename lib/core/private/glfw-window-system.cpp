#include "glfw-window-system.hpp"

#include <stdexcept>

namespace core
{
  void Glfw_WindowSystem::ConfigureWindowForRenderer(const SettingsConfig &settings_config) {}

  const GLFWwindow *Glfw_WindowSystem::GetWindow()
  {
    return window;
  }

  void Glfw_WindowSystem::InitializeWindow(const SettingsConfig &settings_config)
  {
    if (!glfwInit())
    {
      throw std::runtime_error("Failed to initialize GLFW");
    }

    ConfigureWindowForRenderer(settings_config);

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(
      settings_config.width,
      settings_config.height,
      settings_config.title.c_str(),
      nullptr,
      nullptr);

    if (window == nullptr)
    {
      glfwTerminate();
      throw std::runtime_error("Failed to create GLFW window");
    }

    glfwMakeContextCurrent(window);

    HideCursor(window);
  }

  bool Glfw_WindowSystem::IsRunning()
  {
    return !glfwWindowShouldClose(window);
  }

  void Glfw_WindowSystem::HideCursor(GLFWwindow *window)
  {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

  void Glfw_WindowSystem::ShowCursor(GLFWwindow *window)
  {
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
  }
} // core
