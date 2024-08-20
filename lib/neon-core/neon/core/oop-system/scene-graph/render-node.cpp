#include "render-node.hpp"

namespace core
{
  RenderNode::RenderNode(
    const std::string &name,
    const Transform &transform,
    const RenderInfo &render_info,
    RenderPipeline *render_pipeline)
    : Node(name, transform)
  {
    _render_info = render_info;
    _render_pipeline = render_pipeline;
  }

  void RenderNode::Initialize()
  {
    Node::Initialize();
    std::cout << "Initializing " << _name << std::endl;
    _render_object_id = _render_pipeline->CreateRenderObject(_render_info);
  }

  void RenderNode::Update()
  {
    Node::Update();
    _render_pipeline->EnqueueForRendering(_render_object_id, _world_matrix);
  }

  void RenderNode::CleanUp()
  {
    Node::CleanUp();
    std::cout << "Cleaning up " << _name << std::endl;
    _render_pipeline->DestroyRenderObject(_render_object_id);
  }
} // core
