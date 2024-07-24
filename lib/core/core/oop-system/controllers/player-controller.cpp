#include "player-controller.hpp"

#include "glm/gtc/matrix_transform.hpp"

core::PlayerController::PlayerController(InputContext *input_context)
  : _input_context(input_context)
{
  _speed = 2.5f;
  _transform.position = glm::vec3(0.0f, 0.0f, 3.0f);
}

void core::PlayerController::Update(const double delta_time, const glm::mat4 &parent_matrix)
{
  const auto input_state = _input_context->GetInputState();

  _move_direction = glm::vec3(0.0f, 0.0f, 0.0f);

  if (input_state[Action::Forward])
  {
    _move_direction += _forward * _speed;
  }

  if (input_state[Action::Back])
  {
    _move_direction += _forward * -_speed;
  }

  if (input_state[Action::Left])
  {
    _move_direction += normalize(cross(_forward, _up)) * -_speed;
  }

  if (input_state[Action::Right])
  {
    _move_direction += normalize(cross(_forward, _up)) * _speed;
  }

  _transform.position += _move_direction * static_cast<float>(delta_time);

  const auto model = translate(parent_matrix, _transform.position);

  _camera.Update(model);
}

const core::Camera & core::PlayerController::GetCamera() const
{
  return _camera;
}
