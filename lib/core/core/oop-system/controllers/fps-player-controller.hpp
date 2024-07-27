#ifndef PLAYER_CONTROLLER_HPP
#define PLAYER_CONTROLLER_HPP

#include "controller.hpp"
#include "input-context.hpp"
#include "camera/camera.hpp"


namespace core
{
  // ReSharper disable once CppInconsistentNaming
  class FPS_PlayerController final : public Controller
  {
    Camera _camera;
    InputContext *_input_context;
    float _last_x = 0.f;
    float _last_y = 0.f;
    bool _first_mouse = true;

  public:
    explicit FPS_PlayerController(InputContext *input_context);

    void Update(double delta_time, const glm::mat4 &parent_matrix) override;

    [[nodiscard]] const Camera &GetCamera() const;
  };
}


#endif //PLAYER_CONTROLLER_HPP
