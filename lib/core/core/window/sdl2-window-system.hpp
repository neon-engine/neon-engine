#ifndef SDL_2_WINDOW_SYSTEM_HPP
#define SDL_2_WINDOW_SYSTEM_HPP

// ReSharper disable once CppUnusedIncludeDirective
#include <GL/gl3w.h>

#include <SDL.h>

#include "sdl-2-window-context.hpp"
#include "window-system.hpp"

namespace core
{
  // ReSharper disable once CppInconsistentNaming
  class SDL2_WindowSystem final : public WindowSystem, public SDL2_WindowContext{
    SDL_Window *_window = nullptr;
    int _window_flags = 0;
    SDL_GLContext _context = nullptr;
    bool _should_close = false;
  public:
    explicit SDL2_WindowSystem(const SettingsConfig &settings_config)
      : WindowSystem(settings_config) {}

    void Initialize() override;

    [[nodiscard]] bool IsRunning() const override;

    void CleanUp() override;

    void HideCursor() override;

    void ShowCursor() override;

    void Update() override;

  protected:
    void ConfigureWindowForRenderer() override;

  public:
    void SignalToClose() override;
  };
} // core

#endif //SDL_2_WINDOW_SYSTEM_HPP
