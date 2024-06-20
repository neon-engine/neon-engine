#ifndef NEON_FPS_APPLICATION_HPP
#define NEON_FPS_APPLICATION_HPP
#include <core/application.hpp>


class NeonFpsApplication final : public core::Application {
public:
  NeonFpsApplication(
    const SettingsConfig &settings_config,
    core::WindowSystem &window_system,
    core::InputSystem &input_system,
    core::RenderSystem &render_system);
};



#endif //NEON_FPS_APPLICATION_HPP
