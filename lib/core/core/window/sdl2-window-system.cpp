#include "sdl2-window-system.hpp"

#include <iostream>
#include <SDL.h>
#include <sstream>

namespace core
{
  void SDL2_WindowSystem::Initialize()
  {
    std::cout << "Initializing SDL2 window system" << std::endl;
    if (SDL_InitSubSystem(SDL_INIT_VIDEO))
    {
      std::stringstream ss;
      ss << "Failed to initialize SDL2: " << SDL_GetError();
      throw std::runtime_error(ss.str());
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
      throw std::runtime_error("Failed to create SDL2 Window");
    }

    switch (_settings_config.selected_api)
    {
      case RenderingApi::OpenGl:
      {
        _context = SDL_GL_CreateContext(_window);
        break;
      }
      default:
        throw std::runtime_error("Unsupported rendering API configured");
    }
  }

  bool SDL2_WindowSystem::IsRunning() const
  {
    return !_should_close;
  }

  void SDL2_WindowSystem::CleanUp()
  {
    std::cout << "Cleaning up SDL2 window system" << std::endl;
    if (_context != nullptr) { SDL_GL_DeleteContext(_context); }
    SDL_DestroyWindow(_window);
    SDL_Quit();
  }

  void SDL2_WindowSystem::HideCursor() {}
  void SDL2_WindowSystem::ShowCursor() {}
  void SDL2_WindowSystem::Update()
  {
    switch (_settings_config.selected_api)
    {
      case RenderingApi::OpenGl:
      {
        SDL_GL_SwapWindow(_window);
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
        _window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
        break;
      }
      default:
        throw std::runtime_error("Unsupported rendering API configured");
    }
  }

  void SDL2_WindowSystem::SignalToClose()
  {
    std::cout << "SDL2 signaled to close" << std::endl;
    _should_close = true;
  }

  double SDL2_WindowSystem::GetDeltaTime()
  {
    const auto current_frame = SDL_GetPerformanceCounter();
    const auto delta_time = (static_cast<double>(current_frame - _last_frame)*1000 / static_cast<double>(SDL_GetPerformanceFrequency()));
    _last_frame = current_frame;
    return delta_time * .001;
  }
} // core
