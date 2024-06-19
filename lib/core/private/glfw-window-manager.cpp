#include "glfw-window-manager.hpp"

#include <iostream>


namespace core
{
  Glfw_WindowManager::Glfw_WindowManager(const Window_Info &window_info): WindowManager(window_info)
  {
    _window_info = window_info;
    if (!glfwInit())
    {
      throw std::runtime_error("Failed to initialize GLFW");
    }

    ConfigureForOpenGl();

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _window = glfwCreateWindow(
      window_info.width,
      window_info.height,
      window_info.title.c_str(),
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

  void Glfw_WindowManager::ConfigureForOpenGl()
  {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_FOCUSED, GL_TRUE);
  }

  void Glfw_WindowManager::RenderLoop() {}
  void Glfw_WindowManager::HideCursor() { glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); }
  void Glfw_WindowManager::ShowCursor() { glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);}
} // core
