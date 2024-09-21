#ifndef NEON_FPS_APPLICATION_HPP
#define NEON_FPS_APPLICATION_HPP

#include "neon/core/application/application.hpp"
#include "neon/core/application/settings-config.hpp"


class NeonFpsApplication final : public core::Application {
public:
  NeonFpsApplication(
    const SettingsConfig &settings_config,
    core::WindowSystem *window_system,
    core::InputSystem *input_system,
    core::RenderSystem *render_system,
    core::RenderPipeline *render_pipeline,
    core::LoggingSystem *logging_system,
    const std::shared_ptr<core::Logger> &logger);
};



#endif //NEON_FPS_APPLICATION_HPP
