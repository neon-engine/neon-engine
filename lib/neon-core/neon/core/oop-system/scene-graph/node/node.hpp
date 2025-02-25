#ifndef NODE_HPP
#define NODE_HPP

#include <memory>
#include <vector>

#include <neon/core/common/transform.hpp>


namespace core
{
  // ReSharper disable once CppClassCanBeFinal
  class Node
  {
    bool _initialized;
  protected:
    std::string _name;
    Transform _transform;
    glm::mat4 _world_matrix{1.0f};
    Node *_parent = nullptr;
    std::vector<Node *> _children;

    void CalculateWorldMatrix();

  public:
    Node(const std::string &name, const Transform &transform);

    virtual ~Node();

    [[nodiscard]] const std::string& GetName();

    void AddChild(Node *child);

    void RemoveChild(Node *child);

    [[nodiscard]] const std::vector<Node *>& GetChildren() const;

    virtual glm::mat4 GetWorldMatrix();

    virtual void Initialize();

    virtual void Update(double delta_time);

    virtual void CleanUp();
  };
} // core

#endif //NODE_HPP
