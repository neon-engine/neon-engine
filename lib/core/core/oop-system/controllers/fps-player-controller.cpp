#include "fps-player-controller.hpp"

#include <algorithm>

#include "glm/gtc/matrix_transform.hpp"

core::FPS_PlayerController::FPS_PlayerController(InputContext *input_context)
  : _input_context(input_context)
{
  _move_speed = 2.5f;
  _look_speed = .1f;
  _transform.position = glm::vec3(0.0f, 0.0f, 3.0f);
}

void core::FPS_PlayerController::Update(const double delta_time, const glm::mat4 &parent_matrix)
{
  const auto input_state = _input_context->GetInputState();

  _move_direction = glm::vec3(0.0f, 0.0f, 0.0f);

  if (input_state[Action::L_Up])
  {
    _move_direction += _forward * _move_speed;
  }

  if (input_state[Action::L_Down])
  {
    _move_direction += _forward * -_move_speed;
  }

  if (input_state[Action::L_Left])
  {
    _move_direction += normalize(cross(_forward, _up)) * -_move_speed;
  }

  if (input_state[Action::L_Right])
  {
    _move_direction += normalize(cross(_forward, _up)) * _move_speed;
  }

  if (input_state[Action::Mouse])
  {
    const auto mouse_movement = input_state[Axis::Mouse];
    const auto x_pos = mouse_movement.x;
    const auto y_pos = mouse_movement.y;

    if (_first_mouse)
    {
      _first_mouse = false;
      _last_x = static_cast<float>(x_pos);
      _last_y = static_cast<float>(y_pos);
    }

    auto x_offset = static_cast<float>(x_pos) - _last_x;
    auto y_offset = _last_y - static_cast<float>(y_pos);
    _last_x = x_pos;
    _last_y = y_pos;

    x_offset *= _look_speed;
    y_offset *= _look_speed;

    _yaw += x_offset;
    _pitch += y_offset;

    _pitch = std::clamp(_pitch, -89.0f, 89.0f);

    glm::vec3 direction;
    direction.x = glm::cos(glm::radians(_yaw)) * glm::cos(glm::radians(_pitch));
    direction.y = glm::sin(glm::radians(_pitch));
    direction.z = glm::sin(glm::radians(_yaw)) * glm::cos(glm::radians(_pitch));
    _camera.SetLookDirection(glm::normalize(direction));
  }

  _transform.position += _move_direction * static_cast<float>(delta_time);

  const auto model =
    translate(parent_matrix, _transform.position);

  _camera.Update(model);
}

const core::Camera & core::FPS_PlayerController::GetCamera() const
{
  return _camera;
}
