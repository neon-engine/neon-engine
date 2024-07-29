#ifndef RENDER_NODE_HPP
#define RENDER_NODE_HPP
#include "node.hpp"

namespace core
{
  class RenderNode final : public Node
  {
    RenderInfo _render_info;

  public:
    explicit RenderNode(
      const std::string &name,
      const Transform &transform,
      const RenderInfo &render_info);
  };
} // core

#endif //RENDER_NODE_HPP
