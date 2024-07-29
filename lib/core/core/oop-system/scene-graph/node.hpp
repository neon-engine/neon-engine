#ifndef NODE_HPP
#define NODE_HPP
#include <vector>

#include "data-types.hpp"

namespace core
{
  class Node
  {
    Transform _transform;
    Node* _parent;
    std::vector<Node *> _children;

  public:
    Node();

    Node(Node* parent, std::vector<Node> children);

    virtual void Initialize();

    virtual void CleanUp();

    virtual void Update();
  };
} // core

#endif //NODE_HPP
