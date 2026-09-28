#ifndef SDL_2_WINDOW_SYSTEM_HPP
#define SDL_2_WINDOW_SYSTEM_HPP

#include <memory>
#include <SDL.h>
#include <neon/runtime/settings-config.hpp>
#include <neon/logging/logger.hpp>
#include <neon/window/window-system.hpp>

namespace neon
{
  // ReSharper disable once CppInconsistentNaming
  class SDL2_WindowSystem final : public WindowSystem {
    SDL_Window *_window = nullptr;
    int _window_flags = 0;
    bool _should_close = false;
    uint64_t _last_frame;

    void LoadVulkanLibrary() const;

  protected:
    void ConfigureWindowForRenderer() override;

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

    void SignalToClose() override;

    double GetDeltaTime() override;

    void CenterCursor() override;

    void SetWindowFocus(bool focus) override;

    WindowSize GetDrawableSize() override;

    std::vector<std::string> GetVulkanInstanceExtensions() override;

    bool CreateVulkanSurface(void *instance, void *surface) override;
  };
} // neon

#endif //SDL_2_WINDOW_SYSTEM_HPP
