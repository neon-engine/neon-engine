#ifndef INPUT_SYSTEM_HPP
#define INPUT_SYSTEM_HPP

#include <memory>

#include "input-context.hpp"
#include "neon/core/application/settings-config.hpp"
#include "neon/core/logging/logger.hpp"

namespace core
{
  class InputSystem : public InputContext
  {
  protected:
    SettingsConfig _settings_config;
    std::shared_ptr<Logger> _logger;

    ~InputSystem() = default;

  public:
    explicit InputSystem(const SettingsConfig &settings_config, const std::shared_ptr<Logger> &logger)
    {
      _settings_config = settings_config;
      _logger = logger;
    }

    virtual void Initialize() = 0;

    virtual void ProcessInput() = 0;

    virtual void CleanUp() = 0;
  };
} // core

#endif //INPUT_SYSTEM_HPP
