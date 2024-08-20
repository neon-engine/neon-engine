#ifndef PLAYER_NODE_HPP
#define PLAYER_NODE_HPP
#include "node.hpp"

namespace core
{
  class PlayerNode final : public Node
  {
  public:
    PlayerNode(const std::string &name, const Transform &transform);

    void Update() override;
  };
} // core

#endif //PLAYER_NODE_HPP
