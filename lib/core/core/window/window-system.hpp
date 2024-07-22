#ifndef WINDOW_SYSTEM_HPP
#define WINDOW_SYSTEM_HPP

#include "../application/settings-config.hpp"
#include "window-context.hpp"

namespace core
{
  class WindowSystem : public WindowContext
  {
  protected:
    SettingsConfig _settings_config;

    virtual void ConfigureWindowForRenderer() = 0;

    ~WindowSystem() = default;

  public:
    explicit WindowSystem(const SettingsConfig &settings_config)
    {
      _settings_config = settings_config;
    }

    virtual void Initialize() = 0;

    [[nodiscard]] virtual bool IsRunning() const = 0;

    virtual void CleanUp() = 0;

    virtual void HideCursor() = 0;

    virtual void ShowCursor() = 0;

    virtual void Update() = 0;

  };
} // core

#endif //WINDOW_SYSTEM_HPP
