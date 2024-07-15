#ifndef OPEN_GL_RENDER_SYSTEM_HPP
#define OPEN_GL_RENDER_SYSTEM_HPP
#include "render-system.hpp"

namespace core
{
  class OpenGl_RenderSystem final : public RenderSystem {
  public:
    explicit OpenGl_RenderSystem(const SettingsConfig &settings_config)
      : RenderSystem(settings_config) {}

    void Initialize() override;

    void CleanUp() override;
  };
} // core

#endif //OPEN_GL_RENDER_SYSTEM_HPP
