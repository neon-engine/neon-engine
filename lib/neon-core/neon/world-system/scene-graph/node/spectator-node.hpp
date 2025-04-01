#ifndef PLAYER_NODE_HPP
#define PLAYER_NODE_HPP
#include "node.hpp"
#include "neon/input/input-context.hpp"

namespace neon
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
      InputContext *input_context,
      const std::shared_ptr<Logger> &logger);

    void Update(double delta_time) override;

    void MouseCallback(double x, double y);
  };
} // neon

#endif //PLAYER_NODE_HPP
