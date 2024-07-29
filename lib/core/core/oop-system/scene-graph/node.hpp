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
    Node *_parent = nullptr;
    std::vector<Node *> _children;

  public:
    explicit Node(const std::string &name);

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
