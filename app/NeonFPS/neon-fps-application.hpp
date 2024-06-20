#ifndef NEON_FPS_APPLICATION_HPP
#define NEON_FPS_APPLICATION_HPP
#include "core/application.hpp"
#include "core/input-manager.hpp"
#include "core/renderer.hpp"


class NeonFpsApplication final : public core::Application {
public:
  explicit NeonFpsApplication(
    core::WindowManager &window_manager,
    core::InputManager &input_manager,
    core::Renderer &renderer)
    : Application(window_manager, input_manager, renderer) {}

  void Run() override;
};



#endif //NEON_FPS_APPLICATION_HPP
