#include "render-node.hpp"

namespace neon
{
  RenderNode::RenderNode(
    const std::string &name,
    const Transform &transform,
    const RenderInfo &render_info,
    RenderPipeline *render_pipeline,
    const std::shared_ptr<Logger> &logger)
    : Node(name, transform, logger)
  {
    _render_info = render_info;
    _render_pipeline = render_pipeline;
  }

  void RenderNode::Initialize()
  {
    _logger->Info("Initializing {}", _name);
    _render_object_id = _render_pipeline->CreateRenderObject(_render_info);
    Node::Initialize();
  }

  void RenderNode::Update(const double delta_time)
  {
    Node::Update(delta_time);
    _render_pipeline->EnqueueForRendering(_render_object_id, _transform);
  }

  void RenderNode::CleanUp()
  {
    _render_pipeline->DestroyRenderObject(_render_object_id);
    Node::CleanUp();
  }
} // neon
