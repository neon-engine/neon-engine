#ifndef NODE_HPP
#define NODE_HPP
#include <vector>

#include "data-types.hpp"
#include "render-context.hpp"

namespace core
{
  // ReSharper disable once CppClassCanBeFinal
  class Node
  {
  protected:
    std::string _name;
    Transform _transform;
    glm::mat4 _world_matrix{1.0f};
    Node *_parent = nullptr;
    std::vector<Node *> _children;

  public:
    Node(const std::string &name, const Transform &transform);

    virtual ~Node();

    void AddChild(Node *child);

    void RemoveChild(Node *child);

    virtual void Initialize();

    virtual void Update();

    virtual void Render();

    virtual void CleanUp();
  };
} // core

#endif //NODE_HPP
