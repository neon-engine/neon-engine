#include "player-node.hpp"

namespace core {
  PlayerNode::PlayerNode(
    const std::string &name,
    const Transform &transform) : Node(name, transform) {}

  void PlayerNode::Update()
  {
    Node::Update();
  }
} // core