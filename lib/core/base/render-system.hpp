#ifndef RENDER_SYSTEM_HPP
#define RENDER_SYSTEM_HPP

#include "core/settings-config.hpp"

namespace core
{
  class RenderSystem
  {
  protected:
    SettingsConfig _settings_config;

    ~RenderSystem() = default;

  public:
    explicit RenderSystem(const SettingsConfig &settings_config)
    {
      _settings_config = settings_config;
    }

    virtual void Initialize() = 0;

    virtual void CleanUp() = 0;

    virtual void RenderFrame() = 0;
  };
} // core

#endif //RENDER_SYSTEM_HPP
