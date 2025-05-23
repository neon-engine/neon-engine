#include "camera-node.hpp"

namespace neon
{
  CameraNode::CameraNode(
    const std::string &name,
    const Transform &transform,
    RenderPipeline *render_pipeline,
    const std::shared_ptr<Logger> &logger) : Node(name, transform, logger)
  {
    _render_pipeline = render_pipeline;
  }

  void CameraNode::Update(const double delta_time)
  {
    Node::Update(delta_time);
    const glm::vec3 position = _transform.world_coordinates[3];
    _view = lookAt(position, position + _transform.Forward(), _camera_up);
    _render_pipeline->SetCameraInfo({_render_target, _fov, _view, _near, _far});
  }
} // neon