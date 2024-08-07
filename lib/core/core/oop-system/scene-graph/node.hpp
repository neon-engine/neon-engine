#ifndef NODE_HPP
#define NODE_HPP
#include <memory>
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
    std::vector<std::shared_ptr<Node>> _children;

  public:
    Node(const std::string &name, const Transform &transform);

    virtual ~Node();

    void AddChild(const std::shared_ptr<Node> &child);

    void RemoveChild(const std::shared_ptr<Node> &child);

    [[nodiscard]] const std::vector<std::shared_ptr<Node>>& GetChildren() const;

    virtual void Initialize();

    virtual void Update();

    virtual void Render(const glm::mat4 &parent_matrix);

    virtual void CleanUp();
  };
} // core

#endif //NODE_HPP
