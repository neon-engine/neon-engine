#include "headless-window-system.hpp"

namespace neon
{
  void Headless_WindowSystem::ConfigureWindowForRenderer() {}

  void Headless_WindowSystem::Initialize()
  {
    _logger->Info(
      "Initializing headless window system, rendering at {}x{}",
      _settings_config.width,
      _settings_config.height);
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
    return {.width = _settings_config.width, .height = _settings_config.height};
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
} // neon
