#ifndef NEON_RUNTIME_HPP
#define NEON_RUNTIME_HPP

#include "neon/runtime/runtime.hpp"
#include "neon/runtime/settings-config.hpp"


class NeonRuntime final : public neon::Runtime {
public:
  NeonRuntime(
    const SettingsConfig &settings_config,
    neon::WindowSystem *window_system,
    neon::InputSystem *input_system,
    neon::RenderSystem *render_system,
    neon::RenderPipeline *render_pipeline,
    neon::LoggingSystem *logging_system,
    neon::WorldSystem *world_system,
    const std::shared_ptr<neon::Logger> &logger);
};



#endif //NEON_RUNTIME_HPP
