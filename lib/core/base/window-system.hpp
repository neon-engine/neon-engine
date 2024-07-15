#ifndef WINDOW_SYSTEM_HPP
#define WINDOW_SYSTEM_HPP

#include "core/settings-config.hpp"

namespace core
{
  class WindowSystem
  {
  protected:
    SettingsConfig _settings_config;

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
