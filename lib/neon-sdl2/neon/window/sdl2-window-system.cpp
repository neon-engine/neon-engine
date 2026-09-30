#include "sdl2-window-system.hpp"

#include <iostream>
#include <SDL_vulkan.h>

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

    switch (_settings_config.window_mode)
    {
      case WindowMode::Windowed:
      {
        _logger->Info("Window mode: windowed");
        break;
      }
      case WindowMode::Borderless:
      {
        // covers the display at the desktop's resolution, the configured
        // width and height are ignored
        _logger->Info("Window mode: borderless");
        _window_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        break;
      }
      case WindowMode::Fullscreen:
      {
        _logger->Info("Window mode: fullscreen");
        _window_flags |= SDL_WINDOW_FULLSCREEN;
        break;
      }
    }

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
      case RenderingApi::Vulkan:
      {
        // nothing to create here, the renderer asks for a surface later
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
    SDL_DestroyWindow(_window);
    SDL_Quit();
  }

  void SDL2_WindowSystem::Update()
  {
    switch (_settings_config.selected_api)
    {
      case RenderingApi::Vulkan:
      {
        // the renderer presents its own frames
        break;
      }
    }
  }

  void SDL2_WindowSystem::ConfigureWindowForRenderer()
  {
    switch (_settings_config.selected_api)
    {
      case RenderingApi::Vulkan:
      {
        LoadVulkanLibrary();
        _window_flags = SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN;
        break;
      }
      default:
      {
        _logger->Critical("Unsupported rendering API configured");
        throw std::runtime_error("Unsupported rendering API configured");
      }
    }
  }

  void SDL2_WindowSystem::LoadVulkanLibrary() const
  {
    // SDL needs the Vulkan library before it can create a window for it
    if (SDL_Vulkan_LoadLibrary(nullptr) == 0) { return; }

#if defined(__APPLE__)
    // macOS only searches a few fixed folders for libraries, and Homebrew
    // installs the Vulkan loader outside of them
    for (const char *candidate : {"/opt/homebrew/lib/libvulkan.1.dylib", "/usr/local/lib/libvulkan.1.dylib"})
    {
      if (SDL_Vulkan_LoadLibrary(candidate) == 0)
      {
        const std::string path(candidate);
        _logger->Info("Using the Vulkan library at {}", path);
        return;
      }
    }
#endif

    auto error = std::string(SDL_GetError());
    _logger->Critical("The Vulkan library could not be loaded: {}", error);
    throw std::runtime_error("The Vulkan library could not be loaded, is a Vulkan driver installed?");
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

    // a fixed time step replaces the clock
    if (_settings_config.time_step > 0.0) { return _settings_config.time_step; }

    return delta_time * .001;
  }

  void SDL2_WindowSystem::CenterCursor()
  {
    // the window is not always the configured size, so ask for the real one
    int width, height;
    SDL_GetWindowSize(_window, &width, &height);
    SDL_WarpMouseInWindow(_window, width / 2, height / 2);
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

  WindowSize SDL2_WindowSystem::GetDrawableSize()
  {
    WindowSize size{};
    switch (_settings_config.selected_api)
    {
      case RenderingApi::Vulkan:
      {
        SDL_Vulkan_GetDrawableSize(_window, &size.width, &size.height);
        break;
      }
    }
    return size;
  }

  std::vector<std::string> SDL2_WindowSystem::GetVulkanInstanceExtensions()
  {
    unsigned int count = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(_window, &count, nullptr))
    {
      auto error = std::string(SDL_GetError());
      _logger->Error("Could not query the Vulkan extensions of the window: {}", error);
      return {};
    }

    std::vector<const char *> names(count);
    if (!SDL_Vulkan_GetInstanceExtensions(_window, &count, names.data()))
    {
      auto error = std::string(SDL_GetError());
      _logger->Error("Could not query the Vulkan extensions of the window: {}", error);
      return {};
    }

    return {names.begin(), names.end()};
  }

  bool SDL2_WindowSystem::CreateVulkanSurface(void *instance, void *surface)
  {
    if (!SDL_Vulkan_CreateSurface(
      _window,
      static_cast<VkInstance>(instance),
      static_cast<VkSurfaceKHR *>(surface)))
    {
      auto error = std::string(SDL_GetError());
      _logger->Error("Could not create the Vulkan surface of the window: {}", error);
      return false;
    }

    return true;
  }
} // neon
