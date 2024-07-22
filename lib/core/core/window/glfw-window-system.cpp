#include "glfw-window-system.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

namespace core
{

  void GLFW_WindowSystem::Initialize()
  {
    std::cout << "Initializing GLFW window system" << std::endl;
    glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);

    if (!glfwInit())
    {
      throw std::runtime_error("Failed to initialize GLFW");
    }

    ConfigureWindowForRenderer();

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

  bool GLFW_WindowSystem::IsRunning() const
  {
    return !glfwWindowShouldClose(_window);
  }

  GLFWwindow *GLFW_WindowSystem::GetWindow() const
  {
    return _window;
  }

  void GLFW_WindowSystem::Update()
  {
    // check and call events and swap the buffers
    glfwSwapBuffers(_window);
    glfwPollEvents();
  }

  void GLFW_WindowSystem::HideCursor()
  {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

  void GLFW_WindowSystem::ShowCursor()
  {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
  }

  void GLFW_WindowSystem::CleanUp()
  {
    std::cout << "Cleaning up GLFW window system" << std::endl;
    glfwDestroyWindow(_window);
    glfwTerminate();
  }

  void GLFW_WindowSystem::ConfigureWindowForRenderer()
  {
    switch (_settings_config.selected_api)
    {
      case RenderingApi::OpenGl:
      {
        std::cout << "Configuring GLFW window system for OpenGL" << std::endl;
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        break;
      }
      default:
        throw std::runtime_error("Unsupported rendering API configured");
    }
  }

  void GLFW_WindowSystem::SignalToClose()
  {
    throw std::logic_error("GLFW_WindowSystem::SignalToClose() not implemented");
  }
  double GLFW_WindowSystem::GetDeltaTime()
  {
    throw std::logic_error("GLFW_WindowSystem::GetDeltaTime() not implemented");
  }
} // core
