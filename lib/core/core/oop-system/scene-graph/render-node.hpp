#ifndef RENDER_NODE_HPP
#define RENDER_NODE_HPP
#include "node.hpp"

namespace core
{
  class RenderNode final : public Node
  {
    int _render_object_id;

  public:
    explicit RenderNode(
      const std::string &name,
      const int render_object_id)
      : Node(name),
        _render_object_id(render_object_id) {}
  };
} // core

#endif //RENDER_NODE_HPP
