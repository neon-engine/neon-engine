#ifndef INPUT_SYSTEM_HPP
#define INPUT_SYSTEM_HPP
#include "settings-config.hpp"

namespace core
{
  class InputSystem final {
    SettingsConfig _settings_config;
  public:
    explicit InputSystem(const SettingsConfig &settings_config);
    void Initialize();
    void ProcessInput();
    void CleanUp();
  };
} // core

#endif //INPUT_SYSTEM_HPP
