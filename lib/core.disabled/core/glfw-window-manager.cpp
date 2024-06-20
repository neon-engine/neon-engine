#include "glfw-window-manager.hpp"

#include <iostream>

#include "../private/glfw-input-manager.hpp"
#include "../private/opengl-renderer.hpp"


namespace core
{
  Glfw_WindowManager::Glfw_WindowManager(
    const Window_Info &window_info) : WindowManager(window_info), _renderer(_window_info) { }

  Glfw_WindowManager::~Glfw_WindowManager() { CleanUp(); }

  void Glfw_WindowManager::Initialize()
  {
    if (!glfwInit())
    {
      throw std::runtime_error("Failed to initialize GLFW");
    }

    ConfigureWindowForRenderer();

    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    _window = glfwCreateWindow(
      _window_info.width,
      _window_info.height,
      _window_info.title.c_str(),
      nullptr,
      nullptr);

    if (_window == nullptr)
    {
      glfwTerminate();
      throw std::runtime_error("Failed to create GLFW window");
    }

    _created_window = true;

    glfwMakeContextCurrent(_window);

    InitializeInputManager();

    InitializeRenderer();

    HideCursor();
  }

  void Glfw_WindowManager::CleanUp()
  {
    // order here matters

    // first we destroy the renderer
    _renderer.CleanUp();

    // next we close the window
    if (_created_window) { glfwTerminate(); }

    // finally we want to destroy input last, since input is connected to the window
    _input_manager.CleanUp();
  }

  void Glfw_WindowManager::ConfigureWindowForRenderer() const
  {
    switch (_window_info.selected_api)
    {
      case RenderingApi::OpenGl:
      {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        break;
      }
      default:
        throw std::runtime_error("Unsupported type");
    }
  }

  void Glfw_WindowManager::RenderLoop()
  {
    if (!_created_input_manager || !_created_renderer || !_created_window)
    {
      throw std::runtime_error("Window manager was not properly initialized");
    }

    while (!glfwWindowShouldClose(_window))
    {
      // we check for the creation of the input manager before entering the rendering loop
      // ReSharper disable once CppDFANullDereference
      _input_manager.ProcessInput(_window);

      glfwSwapBuffers(_window);
      glfwPollEvents();
    }
  }

  void Glfw_WindowManager::HideCursor()
  {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }

  void Glfw_WindowManager::ShowCursor()
  {
    glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_CAPTURED);
  }

  void Glfw_WindowManager::InitializeInputManager()
  {
    // glfwSetKeyCallback(_window, reinterpret_cast<GLFWkeyfun>(_input_manager.ProcessInputCallback));
    // glfwSetCursorPosCallback(_window, reinterpret_cast<GLFWcursorposfun>(_input_manager.MouseInputCallback));
    // glfwSetScrollCallback(_window, reinterpret_cast<GLFWscrollfun>(_input_manager.ScrollCallback));
  }

  void Glfw_WindowManager::InitializeRenderer()
  {
    _created_renderer = true;
    _renderer.Initialize();
  }

  InputManager Glfw_WindowManager::GetInputManager()
  {
    return WindowManager::GetInputManager();
  }

  Renderer Glfw_WindowManager::GetRenderer()
  {
    return WindowManager::GetRenderer();
  }
} // core
