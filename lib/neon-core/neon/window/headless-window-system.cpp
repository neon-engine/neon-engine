#include "headless-window-system.hpp"

#include "frame-limit.hpp"

#include <algorithm>
#include <cmath>

namespace neon
{
  void Headless_WindowSystem::ConfigureWindowForRenderer() {}

  void Headless_WindowSystem::Initialize()
  {
    const auto [width, height] = GetDrawableSize();
    _logger->Info("Initializing headless window system, rendering at {}x{}", width, height);
  }

  bool Headless_WindowSystem::IsRunning() const
  {
    return !_should_close;
  }

  void Headless_WindowSystem::CleanUp()
  {
    _logger->Info("Cleaning up headless window system");
  }

  void Headless_WindowSystem::Update() {}

  void Headless_WindowSystem::SignalToClose()
  {
    _logger->Info("Headless window system signaled to close");
    _should_close = true;
  }

  double Headless_WindowSystem::GetDeltaTime()
  {
    return _settings_config.time_step > 0.0 ? _settings_config.time_step : frame_time;
  }

  void Headless_WindowSystem::CenterCursor() {}

  void Headless_WindowSystem::SetWindowFocus(bool) {}

  WindowSize Headless_WindowSystem::GetDrawableSize()
  {
    const double scale = _settings_config.render_scale > 0.0 ? _settings_config.render_scale : 1.0;

    return {
      .width = std::max(1, static_cast<int>(std::lround(_settings_config.width * scale))),
      .height = std::max(1, static_cast<int>(std::lround(_settings_config.height * scale)))
    };
  }

  WindowSize Headless_WindowSystem::GetWindowSize()
  {
    return {.width = _settings_config.width, .height = _settings_config.height};
  }

  void Headless_WindowSystem::SetCursorShape(const CursorShape shape)
  {
    _cursor_shape = shape;
  }

  CursorShape Headless_WindowSystem::GetCursorShape() const
  {
    return _cursor_shape;
  }

  std::vector<std::string> Headless_WindowSystem::GetVulkanInstanceExtensions()
  {
    return {};
  }

  bool Headless_WindowSystem::CreateVulkanSurface(void *, void *)
  {
    // there is no window to draw to, the renderer keeps its frames to itself
    return false;
  }

  bool Headless_WindowSystem::SetWindowMode(const WindowMode mode)
  {
    _window_mode = mode;
    _has_window_mode = true;
    return true;
  }

  WindowMode Headless_WindowSystem::GetWindowMode()
  {
    return _has_window_mode ? _window_mode : _settings_config.window_mode;
  }

  void Headless_WindowSystem::SetFrameLimit(const int frames_per_second)
  {
    _frame_limit = FrameLimit::Of(frames_per_second);
  }

  int Headless_WindowSystem::GetFrameLimit()
  {
    return _frame_limit < 0 ? FrameLimit::Of(_settings_config.max_fps) : _frame_limit;
  }
} // neon
