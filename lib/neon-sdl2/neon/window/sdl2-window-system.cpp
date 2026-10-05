#include "sdl2-window-system.hpp"

#include <neon/window/frame-limit.hpp>
#include <algorithm>
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

    SetHints();

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
        break;
      }
      case WindowMode::Fullscreen:
      {
        _logger->Info("Window mode: fullscreen");
        break;
      }
    }

    _window_flags |= WindowFlagsOf(_settings_config);

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

    _frame_limit = FrameLimit::Of(_settings_config.max_fps);
    if (_frame_limit > 0)
    {
      const int limit = _frame_limit;
      _logger->Info("Frame limit: {} frames a second", limit);
    }
    _window_mode = _settings_config.window_mode;
    _wanted_width = _settings_config.width;
    _wanted_height = _settings_config.height;

    FollowMetrics();
    _metrics_revision = 0;

    const int point_width = _metrics.point_width;
    const int point_height = _metrics.point_height;
    const int pixel_width = _metrics.pixel_width;
    const int pixel_height = _metrics.pixel_height;
    const double density = _metrics.Density();
    _logger->Info(
      "The window has {}x{} points and {}x{} pixels, which is a density of {}",
      point_width, point_height, pixel_width, pixel_height, density);
  }

  void SDL2_WindowSystem::SetHints()
  {
    // Windows draws a process that says nothing at the density of 1 and
    // stretches the picture. With this, SDL declares the process aware of
    // the density of every monitor, and counts windows and the pointer in
    // points, as macOS and Wayland do. A window then has the same size to
    // the eye on every display, and all of the pixels of each.
    SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "1");

    // a text that is put together may be longer than fits an event
    SDL_SetHint(SDL_HINT_IME_SUPPORT_EXTENDED_TEXT, "1");

    // the candidates of an input method are shown by the system, next to
    // the caret the engine reports
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
  }

  int SDL2_WindowSystem::WindowFlagsOf(const SettingsConfig &settings_config)
  {
    // all of the pixels of a display of high density, on macOS and on
    // Wayland. On X11 a point is a pixel, and the flag does nothing
    int flags = SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;

    switch (settings_config.window_mode)
    {
      case WindowMode::Borderless:
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
        break;
      case WindowMode::Fullscreen:
        flags |= SDL_WINDOW_FULLSCREEN;
        break;
      default:
        // the renderer follows a window that is given another size
        flags |= SDL_WINDOW_RESIZABLE;
        break;
    }

    return flags;
  }

  WindowMetrics SDL2_WindowSystem::MetricsOf(SDL_Window *window, const int pixel_width, const int pixel_height)
  {
    WindowMetrics metrics;
    if (window == nullptr) { return metrics; }

    SDL_GetWindowSize(window, &metrics.point_width, &metrics.point_height);
    metrics.pixel_width = pixel_width;
    metrics.pixel_height = pixel_height;
    return metrics;
  }

  int SDL2_WindowSystem::SystemCursorOf(const CursorShape shape)
  {
    switch (shape)
    {
      case CursorShape::Pointer:
      case CursorShape::Grab:
      case CursorShape::Grabbing: return SDL_SYSTEM_CURSOR_HAND;
      case CursorShape::Text: return SDL_SYSTEM_CURSOR_IBEAM;
      case CursorShape::Wait: return SDL_SYSTEM_CURSOR_WAIT;
      case CursorShape::Progress: return SDL_SYSTEM_CURSOR_WAITARROW;
      case CursorShape::Crosshair: return SDL_SYSTEM_CURSOR_CROSSHAIR;
      case CursorShape::Move: return SDL_SYSTEM_CURSOR_SIZEALL;
      case CursorShape::NotAllowed: return SDL_SYSTEM_CURSOR_NO;
      case CursorShape::EwResize: return SDL_SYSTEM_CURSOR_SIZEWE;
      case CursorShape::NsResize: return SDL_SYSTEM_CURSOR_SIZENS;
      case CursorShape::NeswResize: return SDL_SYSTEM_CURSOR_SIZENESW;
      case CursorShape::NwseResize: return SDL_SYSTEM_CURSOR_SIZENWSE;
      case CursorShape::None: return -1;
      default: return SDL_SYSTEM_CURSOR_ARROW;
    }
  }

  bool SDL2_WindowSystem::FitDisplayMode() const
  {
    const int display = SDL_GetWindowDisplayIndex(_window);
    SDL_DisplayMode wanted{};
    wanted.w = _wanted_width;
    wanted.h = _wanted_height;

    SDL_DisplayMode nearest{};
    if (display < 0 || SDL_GetClosestDisplayMode(display, &wanted, &nearest) == nullptr) { return false; }
    return SDL_SetWindowDisplayMode(_window, &nearest) == 0;
  }

  bool SDL2_WindowSystem::SetWindowMode(const WindowMode mode)
  {
    if (_window == nullptr) { return false; }

    bool changed = false;
    switch (mode)
    {
      case WindowMode::Windowed:
      {
        changed = SDL_SetWindowFullscreen(_window, 0) == 0;
        if (changed)
        {
          SDL_SetWindowBordered(_window, SDL_TRUE);
          SDL_SetWindowResizable(_window, SDL_TRUE);
          SDL_SetWindowSize(_window, _wanted_width, _wanted_height);
          SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        }
        break;
      }
      case WindowMode::Borderless:
      {
        // covers the display at the size the desktop has
        changed = SDL_SetWindowFullscreen(_window, SDL_WINDOW_FULLSCREEN_DESKTOP) == 0;
        break;
      }
      case WindowMode::Fullscreen:
      {
        // the size is set before the display is taken over, and the display
        // is taken over anew when it is taken over already
        if (_window_mode == WindowMode::Fullscreen) { SDL_SetWindowFullscreen(_window, 0); }
        changed = FitDisplayMode() && SDL_SetWindowFullscreen(_window, SDL_WINDOW_FULLSCREEN) == 0;
        break;
      }
    }

    const char *name = mode == WindowMode::Windowed ? "windowed" : mode == WindowMode::Borderless ? "borderless" : "fullscreen";
    if (!changed)
    {
      const std::string error = SDL_GetError();
      _logger->Error("The window cannot be shown {}: {}", name, error);
      return false;
    }

    _window_mode = mode;
    _logger->Info("Window mode: {}", name);

    // the renderer and the user interface follow the size it has now
    FollowMetrics();
    return true;
  }

  WindowMode SDL2_WindowSystem::GetWindowMode()
  {
    return _window_mode;
  }

  bool SDL2_WindowSystem::SetWindowSize(const int width, const int height)
  {
    if (_window == nullptr || width <= 0 || height <= 0) { return false; }

    _wanted_width = width;
    _wanted_height = height;

    switch (_window_mode)
    {
      case WindowMode::Windowed:
        SDL_SetWindowSize(_window, width, height);
        SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        break;
      case WindowMode::Borderless:
        // it covers its display, and has the size when it is a window again
        break;
      case WindowMode::Fullscreen:
        return SetWindowMode(WindowMode::Fullscreen);
    }

    _logger->Info("Window size: {}x{} points", width, height);
    FollowMetrics();
    return true;
  }

  std::vector<WindowSize> SDL2_WindowSystem::GetDisplaySizes()
  {
    std::vector<WindowSize> sizes;
    if (_window == nullptr) { return sizes; }

    // the modes of a display come largest first, and one size comes once
    // for every rate the display shows it at
    const int display = SDL_GetWindowDisplayIndex(_window);
    const int count = display < 0 ? 0 : SDL_GetNumDisplayModes(display);
    for (int index = 0; index < count; index++)
    {
      SDL_DisplayMode mode{};
      if (SDL_GetDisplayMode(display, index, &mode) != 0) { continue; }

      const bool is_known = std::ranges::any_of(sizes, [&mode](const WindowSize &size)
      {
        return size.width == mode.w && size.height == mode.h;
      });
      if (!is_known) { sizes.push_back({.width = mode.w, .height = mode.h}); }
    }
    return sizes;
  }

  void SDL2_WindowSystem::FollowMetrics()
  {
    if (_window == nullptr) { return; }

    const auto [pixel_width, pixel_height] = GetDrawableSize();
    const WindowMetrics metrics = MetricsOf(_window, pixel_width, pixel_height);

    if (metrics == _metrics) { return; }

    _metrics = metrics;
    _metrics_revision++;
  }

  WindowSize SDL2_WindowSystem::GetWindowSize()
  {
    return {.width = _metrics.point_width, .height = _metrics.point_height};
  }

  WindowMetrics SDL2_WindowSystem::GetMetrics()
  {
    return _metrics;
  }

  std::uint64_t SDL2_WindowSystem::GetMetricsRevision()
  {
    return _metrics_revision;
  }

  void SDL2_WindowSystem::SetCursorShape(const CursorShape shape)
  {
    if (shape == _cursor_shape) { return; }
    _cursor_shape = shape;

    const int system_cursor = SystemCursorOf(shape);
    if (system_cursor < 0)
    {
      SDL_ShowCursor(SDL_DISABLE);
      return;
    }

    auto &cursor = _cursors[static_cast<std::size_t>(system_cursor)];
    if (cursor == nullptr) { cursor = SDL_CreateSystemCursor(static_cast<SDL_SystemCursor>(system_cursor)); }

    // a platform without the shape keeps the cursor it shows
    if (cursor != nullptr) { SDL_SetCursor(cursor); }
    SDL_ShowCursor(SDL_ENABLE);
  }

  bool SDL2_WindowSystem::IsRunning() const
  {
    return !_should_close;
  }

  void SDL2_WindowSystem::CleanUp()
  {
    _logger->Info("Cleaning up SDL2 window system");

    for (auto &cursor : _cursors)
    {
      if (cursor != nullptr) { SDL_FreeCursor(cursor); }
      cursor = nullptr;
    }

    SDL_DestroyWindow(_window);
    SDL_Quit();
  }

  void SDL2_WindowSystem::Update()
  {
    // a window that was resized, or moved to a display of another density
    FollowMetrics();

    MeasureFrame();

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
        _window_flags = SDL_WINDOW_VULKAN;
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

  void SDL2_WindowSystem::SetFrameLimit(const int frames_per_second)
  {
    const int limit = FrameLimit::Of(frames_per_second);
    if (limit == _frame_limit) { return; }

    _frame_limit = limit;
    if (limit > 0) { _logger->Info("Frame limit: {} frames a second", limit); }
    else { _logger->Info("Frame limit: none"); }
  }

  int SDL2_WindowSystem::GetFrameLimit()
  {
    return _frame_limit;
  }

  void SDL2_WindowSystem::MeasureFrame()
  {
    // Under a limit a frame that was done early waits until it is due:
    // asleep while there is more than a couple of milliseconds left, since
    // a sleep ends late, and looking at the clock for the last of it. A
    // frame is due one frame's time after the one before was, not after it
    // began, so that one that ran late is made up for by the next and the
    // limit is what a second holds. No more than one frame is made up for.
    if (_frame_limit > 0 && _settings_config.time_step <= 0.0)
    {
      const auto frequency = static_cast<double>(SDL_GetPerformanceFrequency());
      const auto frame = static_cast<Uint64>(FrameLimit::SecondsOf(_frame_limit) * frequency);
      const Uint64 now = SDL_GetPerformanceCounter();
      _frame_due = std::max(_frame_due + frame, now > frame ? now - frame : 0);

      for (Uint64 at = now; at < _frame_due; at = SDL_GetPerformanceCounter())
      {
        if (static_cast<double>(_frame_due - at) / frequency > 0.002) { SDL_Delay(1); }
      }
    }

    const auto current_frame = SDL_GetPerformanceCounter();
    const auto delta_time =
      static_cast<double>(current_frame - _last_frame)*1000 / static_cast<double>(SDL_GetPerformanceFrequency());
    _last_frame = current_frame;

    _delta_time = delta_time * .001;
  }

  double SDL2_WindowSystem::GetDeltaTime()
  {
    // a fixed time step replaces the clock
    if (_settings_config.time_step > 0.0) { return _settings_config.time_step; }

    // The time is measured once in a frame, where the frame ends. The
    // world and the user interface both ask, and are told the same.
    return _delta_time;
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
