#include "node.hpp"

#include <glm/gtc/quaternion.hpp>

namespace core {

  Node::Node(
    const std::string &name,
    const Transform &transform,
    const std::shared_ptr<Logger> &logger)
  {
    _name = name;
    _transform = transform;
    _logger = logger;
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
    _transform.world_coordinates = position * rotation * scale * parent_matrix;
  }

  void Node::AddChild(Node *child)
  {
    _children.push_back(child);
    child->_parent = this;
    _logger->Info("Added node {} to node {}", child->_name, _name);
  }

  void Node::RemoveChild(Node *child)
  {
    if (const auto it = std::ranges::find(_children, child);
      it != _children.end())
    {
      _children.erase(it);
      _logger->Info("Removed node {} from node {}", child->_name, _name);
    }
  }

  const std::vector<Node *> & Node::GetChildren() const
  {
    return _children;
  }

  glm::mat4 Node::GetWorldMatrix()
  {
    return _transform.world_coordinates;
  }

  void Node::Initialize()
  {
    _logger->Info("Initialized {}", _name);
    _initialized = true;
  }

  void Node::Update(const double delta_time)
  {
    CalculateWorldMatrix();
  }

  void Node::CleanUp()
  {
    _logger->Info("Cleaned up {}",_name);
    _initialized = false;
  }
} // core