#ifndef PLAYER_NODE_HPP
#define PLAYER_NODE_HPP
#include "node.hpp"
#include "neon/core/input/input-context.hpp"

namespace core
{
  class PlayerNode final : public Node
  {
    float _move_speed = 2.5f;
    float _look_speed = 0.1f;
    glm::vec3 _up = Transform::Up();
    glm::vec3 _forward = Transform::Forward();
    InputContext *_input_context;
  public:
    PlayerNode(
      const std::string &name,
      const Transform &transform,
      InputContext *input_context);

    void Update() override;
  };
} // core

#endif //PLAYER_NODE_HPP
