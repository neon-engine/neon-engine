#include "node.hpp"

namespace core {
  Node::Node(const std::string &name)
  {
    _name = name;
  }

  Node::~Node() = default;

  void Node::AddChild(Node *child)
  {
    _children.push_back(child);
    child->_parent = this;
  }

  void Node::RemoveChild(Node *child)
  {
    if (const auto it = std::ranges::find(_children, child);
      it != _children.end())
    {
      _children.erase(it);
    }
  }

  void Node::Initialize() {}

  void Node::Update() {}

  void Node::Render() {}

  void Node::CleanUp() {}
} // core