#ifndef WINDOW_SYSTEM_HPP
#define WINDOW_SYSTEM_HPP
#include "settings-config.hpp"

namespace core {

class WindowSystem final {
public:
  WindowSystem(const SettingsConfig &settings_config);
  void Initialize();
  void CleanUp();
};

} // core

#endif //WINDOW_SYSTEM_HPP
