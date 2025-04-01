#include "spectator-node.hpp"

#include <bits/stl_algo.h>

namespace neon
{
  SpectatorNode::SpectatorNode(
    const std::string &name,
    const Transform &transform,
    InputContext *input_context,
    const std::shared_ptr<Logger> &logger) : Node(name, transform, logger)
  {
    _input_context = input_context;
  }

  void SpectatorNode::Update(const double delta_time)
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

    if (input_state[Action::Mouse])
    {
      const auto x = input_state[Axis::Mouse].x;
      const auto y = input_state[Axis::Mouse].y;

      MouseCallback(x, y);
    }

    _transform.position += move_direction * static_cast<float>(delta_time);
    CalculateWorldMatrix();
  }

  void SpectatorNode::MouseCallback(const double x, const double y)
  {
    _transform.rotation.yaw -= static_cast<float>(x) * 0.1f;
    _transform.rotation.pitch -= static_cast<float>(y) * 0.1f;

    _transform.rotation.pitch = std::clamp(_transform.rotation.pitch, -89.f, 89.f);
  }
} // neon