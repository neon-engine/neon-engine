#ifndef NODE_HPP
#define NODE_HPP

#include <memory>
#include <vector>

#include <neon/common/transform.hpp>

#include "neon/logging/logger.hpp"


namespace neon
{
  // ReSharper disable once CppClassCanBeFinal
  class Node
  {
    bool _initialized = false;
  protected:
    std::string _name;
    Transform _transform;
    Node *_parent = nullptr;
    std::vector<Node *> _children;
    std::shared_ptr<Logger> _logger;

    void CalculateWorldMatrix();

  public:
    Node(const std::string &name, const Transform &transform, const std::shared_ptr<Logger> &logger);

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
} // neon

#endif //NODE_HPP
