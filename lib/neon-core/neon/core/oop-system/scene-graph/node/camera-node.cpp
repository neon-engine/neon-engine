#include "camera-node.hpp"

namespace core {
  CameraNode::CameraNode(
    const std::string &name,
    const Transform &transform,
    RenderPipeline *render_pipeline) : Node(name, transform)
  {
    _render_pipeline = render_pipeline;
  }

  void CameraNode::Update(const double delta_time)
  {
    Node::Update(delta_time);
    const glm::vec3 position = _world_matrix[3];
    _view = lookAt(position, position + _look_direction, _camera_up);
    _render_pipeline->SetCameraInfo({_render_target, _fov, _view, _near, _far});
  }
} // core