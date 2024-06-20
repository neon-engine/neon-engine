#include "glfw-window-manager.hpp"

#include <iostream>
#include <memory>

#include "glfw-input-manager.hpp"
#include "opengl-renderer.hpp"


namespace core
{
  Glfw_WindowManager::Glfw_WindowManager(const Window_Info &window_info): WindowManager(window_info) {}

  Glfw_WindowManager::~Glfw_WindowManager()
  {
    CleanUp();
  }

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
    if (_created_renderer) { delete _renderer; }

    // next we close the window
    if (_created_window) { glfwTerminate(); }

    // finally we want to destroy input last, since input is connected to the window
    if (_created_input_manager) { delete _input_manager; }
  }

  void Glfw_WindowManager::ConfigureWindowForRenderer() const
  {
    switch (_window_info.supported_api)
    {
      case SupportedApi::OpenGl:
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

  void Glfw_WindowManager::RenderLoop() {}

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
    const auto input_manager = new Glfw_InputManager();
    _input_manager = input_manager;
    _created_input_manager = true;
    glfwSetKeyCallback(_window, reinterpret_cast<GLFWkeyfun>(input_manager->ProcessInputCallback));
    glfwSetCursorPosCallback(_window, reinterpret_cast<GLFWcursorposfun>(input_manager->MouseInputCallback));
    glfwSetScrollCallback(_window, reinterpret_cast<GLFWscrollfun>(input_manager->ScrollCallback));
  }

  void Glfw_WindowManager::InitializeRenderer()
  {
    switch (_window_info.supported_api)
    {
      case SupportedApi::OpenGl:
      {
        _renderer = new OpenGl_Renderer(_window_info);
        break;
      }
      default:
        throw std::runtime_error("unsupported API");
    }
    _created_renderer = true;
    _renderer->Initialize();
  }
} // core
