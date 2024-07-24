#ifndef INPUT_SYSTEM_HPP
#define INPUT_SYSTEM_HPP

#include "input-context.hpp"
#include "../application/settings-config.hpp"

namespace core
{
  class InputSystem : public InputContext
  {
  protected:
    SettingsConfig _settings_config;

    ~InputSystem() = default;

  public:
    explicit InputSystem(const SettingsConfig &settings_config)
    {
      _settings_config = settings_config;
    }

    virtual void Initialize() = 0;

    virtual void ProcessInput() = 0;

    virtual void CleanUp() = 0;
  };
} // core

#endif //INPUT_SYSTEM_HPP
