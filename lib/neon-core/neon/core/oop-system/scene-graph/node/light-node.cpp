#include "light-node.hpp"

namespace core {
  LightNode::LightNode(
    const std::string &name,
    const Transform &transform,
    RenderPipeline *render_pipeline,
    const LightSource &light_source,
    const std::shared_ptr<Logger> &logger)
  : Node(name, transform, logger)
  {
    _render_pipeline = render_pipeline;
    _light_source = light_source;
  }

  void LightNode::Initialize()
  {
    _logger->Info("Initializing light node {}", _name);
    Node::Initialize();
  }

  void LightNode::Update(const double delta_time)
  {
    Node::Update(delta_time);
    _light_source.position = _transform.world_coordinates[3];
    _render_pipeline->EnqueueLightSource(_light_source);
  }

  void LightNode::CleanUp()
  {
    Node::CleanUp();
  }
} // core