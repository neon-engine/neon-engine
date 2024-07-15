#include "glfw-window-system.hpp"

#include <iostream>
#include <stdexcept>

namespace core
{
  void Glfw_WindowSystem::ConfigureWindowForRenderer(const SettingsConfig &settings_config) const
  {
    switch (_settings_config.selected_api)
    {
      case RenderingApi::OpenGl:
      {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        break;
      }
      default:
        throw std::runtime_error("Unsupported rendering API configured");
    }
  }

  void Glfw_WindowSystem::Initialize()
  {
    std::cout << "Initializing GLFW window system" << std::endl;
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

    // HideCursor();
  }

  bool Glfw_WindowSystem::IsRunning() const
  {
    return !glfwWindowShouldClose(_window);
  }

  GLFWwindow *Glfw_WindowSystem::GetWindow() const
  {
    return _window;
  }

  void Glfw_WindowSystem::Update()
  {
    // check and call events and swap the buffers
    glfwSwapBuffers(_window);
    glfwPollEvents();
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
    std::cout << "Cleaning up GLFW window system" << std::endl;
    glfwDestroyWindow(_window);
    glfwTerminate();
  }
} // core
