#include "sdl2-window-system.hpp"

#include <iostream>

namespace neon
{
  void SDL2_WindowSystem::Initialize()
  {
    _logger->Info("Initializing SDL2 window system");
    constexpr int major = SDL_MAJOR_VERSION;
    constexpr int minor = SDL_MINOR_VERSION;
    constexpr int patch = SDL_PATCHLEVEL;
    _logger->Info("SDL version: {}.{}.{}", major, minor, patch);

    if (SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
      auto error = std::string(SDL_GetError());
      _logger->Critical("Failed to initialize SDL2: {}", error);
      throw std::runtime_error("Failed to initialize SDL2");
    }

    ConfigureWindowForRenderer();

    _window = SDL_CreateWindow(
      _settings_config.title.c_str(),
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      _settings_config.width,
      _settings_config.height,
      _window_flags);

    if (_window == nullptr)
    {
      SDL_Quit();
      _logger->Critical("Failed to create SDL2 Window");
      throw std::runtime_error("Failed to create SDL2 Window");
    }

    switch (_settings_config.selected_api)
    {
      case RenderingApi::OpenGl:
      {
        _logger->Info("Creating OpenGL context");
        _context = SDL_GL_CreateContext(_window);

        // disable vsync after creating the context on KDE
        // TODO [issues/1] make vsync configurable
        SDL_GL_SetSwapInterval(0);
        break;
      }
      case RenderingApi::Vulkan:
      {
        _logger->Info("Creating Vulkan context");
        throw std::runtime_error("not yet implemented");
        break;
      }
      default:
      {
        _logger->Critical("Unsupported rendering API configured");
        throw std::runtime_error("Unsupported rendering API configured");
      }
    }
  }

  bool SDL2_WindowSystem::IsRunning() const
  {
    return !_should_close;
  }

  void SDL2_WindowSystem::CleanUp()
  {
    _logger->Info("Cleaning up SDL2 window system");
    if (_context != nullptr) { SDL_GL_DeleteContext(_context); }
    SDL_DestroyWindow(_window);
    SDL_Quit();
  }

  void SDL2_WindowSystem::Update()
  {
    switch (_settings_config.selected_api)
    {
      case RenderingApi::OpenGl:
      {
        SDL_GL_SwapWindow(_window);
        break;
      }
      case RenderingApi::Vulkan:
      {
        throw std::runtime_error("not yet implemented");
        break;
      }
    }
  }

  void SDL2_WindowSystem::ConfigureWindowForRenderer()
  {
    switch (_settings_config.selected_api)
    {
      case RenderingApi::OpenGl:
      {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        _window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
        break;
      }
      case RenderingApi::Vulkan:
      {
        throw std::runtime_error("not yet implemented");
        break;
      }
      default:
      {
        _logger->Critical("Unsupported rendering API configured");
        throw std::runtime_error("Unsupported rendering API configured");
      }
    }
  }

  void SDL2_WindowSystem::SignalToClose()
  {
    _logger->Info("SDL2 signaled to close");
    _should_close = true;
  }

  double SDL2_WindowSystem::GetDeltaTime()
  {
    const auto current_frame = SDL_GetPerformanceCounter();
    const auto delta_time =
      static_cast<double>(current_frame - _last_frame)*1000 / static_cast<double>(SDL_GetPerformanceFrequency());
    _last_frame = current_frame;
    return delta_time * .001;
  }

  void SDL2_WindowSystem::CenterCursor()
  {
    SDL_WarpMouseInWindow(_window, _settings_config.width / 2, _settings_config.height / 2);
  }

  void SDL2_WindowSystem::SetWindowFocus(const bool focus)
  {
    if (focus)
    {
      SDL_SetWindowGrab(_window, SDL_TRUE);
    } else
    {
      SDL_SetWindowGrab(_window, SDL_FALSE);
    }
  }

  void *SDL2_WindowSystem::GetGlProcAddress()
  {
    return reinterpret_cast<void*>(SDL_GL_GetProcAddress);
  }
} // neon
