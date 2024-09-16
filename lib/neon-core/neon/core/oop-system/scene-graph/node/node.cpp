#include "node.hpp"

#include <iostream>

#include "glm/gtc/quaternion.hpp"

namespace core {

  Node::Node(
    const std::string &name,
    const Transform &transform)
  {
    _name = name;
    _transform = transform;
  }

  Node::~Node() = default;

  const std::string & Node::GetName()
  {
    return _name;
  }

  void Node::CalculateWorldMatrix()
  {
    const auto position = translate(glm::mat4{1.0f}, _transform.position);
    const auto rotation = mat4_cast(_transform.rotation.GetQuaternion());
    const auto scale = glm::scale(glm::mat4{1.0f}, _transform.scale);
    const auto parent_matrix = _parent == nullptr ? glm::mat4(1.0f) : _parent->GetWorldMatrix();
    _world_matrix = position * rotation * scale * parent_matrix;
  }

  void Node::AddChild(Node *child)
  {
    _children.push_back(child);
    child->_parent = this;
    std::cout << "Added node " << child->_name << " to node " << _name << std::endl;
  }

  void Node::RemoveChild(Node *child)
  {
    if (const auto it = std::ranges::find(_children, child);
      it != _children.end())
    {
      _children.erase(it);
      std::cout << "Removed node " << child->_name << " from node " << _name << std::endl;
    }
  }

  const std::vector<Node *> & Node::GetChildren() const
  {
    return _children;
  }

  glm::mat4 Node::GetWorldMatrix()
  {
    return _world_matrix;
  }

  void Node::Initialize() {}

  void Node::Update(const double delta_time)
  {
    CalculateWorldMatrix();
  }

  void Node::CleanUp() {}
} // core