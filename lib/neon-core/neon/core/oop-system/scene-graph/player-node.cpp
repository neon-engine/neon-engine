#include "player-node.hpp"

namespace core {
  PlayerNode::PlayerNode(
    const std::string &name,
    const Transform &transform,
    InputContext *input_context) : Node(name, transform)
  {
    _input_context = input_context;
  }

  void PlayerNode::Update()
  {
    const auto input_state = _input_context->GetInputState();

    auto move_direction = glm::vec3{0.0f, 0.0f, 0.0f};

    if (input_state[Action::L_Up])
    {
      move_direction += _forward * _move_speed;
    }

    if (input_state[Action::L_Down])
    {
      move_direction += _forward * -_move_speed;
    }

    if (input_state[Action::L_Left])
    {
      move_direction += normalize(cross(_forward, _up)) * -_move_speed;
    }

    if (input_state[Action::L_Right])
    {
      move_direction += normalize(cross(_forward, _up)) * _move_speed;
    }

    _transform.position += move_direction * static_cast<float>(1.0f);
    CalculateWorldMatrix();
  }
} // core