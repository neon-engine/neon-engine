#ifndef NEON_FPS_APPLICATION_HPP
#define NEON_FPS_APPLICATION_HPP

#include "neon/application/application.hpp"
#include "neon/application/settings-config.hpp"


class NeonFpsApplication final : public neon::Application {
public:
  NeonFpsApplication(
    const SettingsConfig &settings_config,
    neon::WindowSystem *window_system,
    neon::InputSystem *input_system,
    neon::RenderSystem *render_system,
    neon::RenderPipeline *render_pipeline,
    neon::LoggingSystem *logging_system,
    neon::WorldSystem *world_system,
    const std::shared_ptr<neon::Logger> &logger);
};



#endif //NEON_FPS_APPLICATION_HPP
