#include "node.hpp"

namespace core {
  Node::Node(
    const std::string &name,
    const Transform &transform)
  {
    _name = name;
    _transform = transform;
  }

  Node::~Node() = default;

  void Node::AddChild(const std::shared_ptr<Node> &child)
  {
    _children.push_back(child);
    child->_parent = this;
    std::cout << "Added node " << child->_name << " to node " << _name << std::endl;
  }

  void Node::RemoveChild(const std::shared_ptr<Node> &child)
  {
    if (const auto it = std::ranges::find(_children, child);
      it != _children.end())
    {
      _children.erase(it);
      std::cout << "Removed node " << child->_name << " from node " << _name << std::endl;
    }
  }

  const std::vector<std::shared_ptr<Node>> & Node::GetChildren() const
  {
    return _children;
  }

  void Node::Initialize() {}

  void Node::Update() {}

  void Node::Render(const glm::mat4 &parent_matrix) {}

  void Node::CleanUp() {}
} // core