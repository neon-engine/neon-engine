#ifndef RENDER_SYSTEM_HPP
#define RENDER_SYSTEM_HPP
#include "settings-config.hpp"

namespace core {

class RenderSystem final {
public:
  RenderSystem(const SettingsConfig& settings_config);
  void Initialize();
  void CleanUp();
};

} // core

#endif //RENDER_SYSTEM_HPP
