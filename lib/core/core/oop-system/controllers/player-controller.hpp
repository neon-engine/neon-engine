#ifndef PLAYER_CONTROLLER_HPP
#define PLAYER_CONTROLLER_HPP

#include "controller.hpp"
#include "input-context.hpp"
#include "camera/camera.hpp"


namespace core
{
  class PlayerController final : public Controller
  {
    Camera _camera;
    InputContext *_input_context;

  public:
    explicit PlayerController(InputContext *input_context);

    void Update(double delta_time, const glm::mat4 &parent_matrix) override;

    const Camera &GetCamera() const;
  };
}


#endif //PLAYER_CONTROLLER_HPP
