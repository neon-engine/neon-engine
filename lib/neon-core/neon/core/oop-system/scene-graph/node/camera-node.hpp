#ifndef CAMERA_NODE_HPP
#define CAMERA_NODE_HPP

#include "node.hpp"
#include "neon/core/render/data-types.hpp"
#include "neon/core/render/render-pipeline.hpp"


namespace core {

class CameraNode final : public Node {
  RenderPipeline *_render_pipeline;
  glm::vec3 _camera_up = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 _look_direction = glm::vec3(0.0f, 0.0f, -1.0f);
  glm::mat4 _view = glm::mat4(1.0f);
  float _fov = 45.0f;
  RenderTarget _render_target = RenderTarget::Window;
  float _near = 0.1f;
  float _far = 1000.f;
public:
  CameraNode(
    const std::string &name,
    const Transform &transform,
    RenderPipeline *render_pipeline);

  void Update(double delta_time) override;
};

} // core

#endif //CAMERA_NODE_HPP
