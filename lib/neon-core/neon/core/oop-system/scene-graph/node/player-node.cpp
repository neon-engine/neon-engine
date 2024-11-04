#include "player-node.hpp"

namespace core {
  PlayerNode::PlayerNode(
    const std::string &name,
    const Transform &transform,
    InputContext *input_context) : Node(name, transform)
  {
    _input_context = input_context;
  }

  void PlayerNode::Update(const double delta_time)
  {
    const auto input_state = _input_context->GetInputState();

    auto move_direction = glm::vec3{0.0f, 0.0f, 0.0f};

    if (input_state[Action::L_Up])
    {
      move_direction += _transform.Forward() * _move_speed;
    }

    if (input_state[Action::L_Down])
    {
      move_direction += _transform.Forward() * -_move_speed;
    }

    if (input_state[Action::L_Left])
    {
      move_direction += _transform.Right() * -_move_speed;
    }

    if (input_state[Action::L_Right])
    {
      move_direction += _transform.Right() * _move_speed;
    }

    _transform.position += move_direction * static_cast<float>(delta_time);
    CalculateWorldMatrix();
  }
} // core