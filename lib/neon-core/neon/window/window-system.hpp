#ifndef WINDOW_SYSTEM_HPP
#define WINDOW_SYSTEM_HPP

#include "window-context.hpp"
#include "neon/application/settings-config.hpp"
#include "neon/logging/logger.hpp"

namespace neon
{
  class WindowSystem : public WindowContext
  {
  protected:
    SettingsConfig _settings_config;
    std::shared_ptr<Logger> _logger;

    virtual void ConfigureWindowForRenderer() = 0;

    ~WindowSystem() = default;

  public:
    explicit WindowSystem(const SettingsConfig &settings_config, const std::shared_ptr<Logger> &logger)
    {
      _settings_config = settings_config;
      _logger = logger;
    }

    virtual void Initialize() = 0;

    [[nodiscard]] virtual bool IsRunning() const = 0;

    virtual void CleanUp() = 0;

    virtual void Update() = 0;

  };
} // neon

#endif //WINDOW_SYSTEM_HPP
