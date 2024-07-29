#include "render-node.hpp"

namespace core
{
  RenderNode::RenderNode(
    const std::string &name,
    const Transform &transform,
    const RenderInfo &render_info)
    : Node(name, transform)
  {
    _render_info = render_info;
  }

  void RenderNode::Initialize()
  {
  }

  void RenderNode::Update()
  {
  }

  void RenderNode::Render()
  {
  }

  void RenderNode::CleanUp()
  {
  }
} // core
