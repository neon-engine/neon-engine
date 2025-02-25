#ifndef PLAYER_NODE_HPP
#define PLAYER_NODE_HPP
#include "node.hpp"
#include "neon/core/input/input-context.hpp"

namespace core
{
  class SpectatorNode final : public Node
  {
    float _move_speed = 2.5f;
    float _look_speed = 0.1f;
    InputContext *_input_context;

  public:
    SpectatorNode(
      const std::string &name,
      const Transform &transform,
      InputContext *input_context);

    void Update(double delta_time) override;

    void MouseCallback(const double &x, const double &y);
  };
} // core

#endif //PLAYER_NODE_HPP
