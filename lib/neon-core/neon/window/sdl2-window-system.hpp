#ifndef SDL_2_WINDOW_SYSTEM_HPP
#define SDL_2_WINDOW_SYSTEM_HPP

// gl3w must be included first
// ReSharper disable once CppUnusedIncludeDirective
#include <memory>
#include <glad/gl.h>

#include <SDL.h>

#include "window-system.hpp"
#include "neon/application/settings-config.hpp"
#include "neon/logging/logger.hpp"

namespace neon
{
  // ReSharper disable once CppInconsistentNaming
  class SDL2_WindowSystem final : public WindowSystem {
    SDL_Window *_window = nullptr;
    int _window_flags = 0;
    SDL_GLContext _context = nullptr;
    bool _should_close = false;
    uint64_t _last_frame;

  public:
    explicit SDL2_WindowSystem(const SettingsConfig &settings_config, const std::shared_ptr<Logger> &logger)
      : WindowSystem(settings_config, logger)
    {
      _last_frame = SDL_GetPerformanceCounter();
    }

    void Initialize() override;

    [[nodiscard]] bool IsRunning() const override;

    void CleanUp() override;

    void Update() override;

  protected:
    void ConfigureWindowForRenderer() override;

  public:
    void SignalToClose() override;

    double GetDeltaTime() override;

    void CenterCursor() override;

    void SetWindowFocus(bool focus) override;

    void* GetGlProcAddress() override;
  };
} // neon

#endif //SDL_2_WINDOW_SYSTEM_HPP
