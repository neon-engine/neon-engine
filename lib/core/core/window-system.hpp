#ifndef WINDOW_SYSTEM_HPP
#define WINDOW_SYSTEM_HPP

#include <GLFW/glfw3.h>

#include "settings-config.hpp"

namespace core {

class WindowSystem final {
  SettingsConfig _settings_config;
public:
  explicit WindowSystem(const SettingsConfig &settings_config);
  void Initialize();
  [[nodiscard]] bool IsRunning() const;
  void CleanUp();
};

} // core

#endif //WINDOW_SYSTEM_HPP
