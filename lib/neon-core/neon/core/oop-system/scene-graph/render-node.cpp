#include "render-node.hpp"

#include <glm/gtc/quaternion.hpp>

namespace core
{
  RenderNode::RenderNode(
    const std::string &name,
    const Transform &transform,
    const RenderInfo &render_info,
    RenderContext *render_context)
    : Node(name, transform)
  {
    _render_info = render_info;
    _render_context = render_context;
  }

  void RenderNode::Initialize()
  {
    std::cout << "Initializing " << _name << std::endl;
    _render_object_id = _render_context->CreateRenderObject(_render_info);
  }

  void RenderNode::Update()
  {
  }

  void RenderNode::Render(const glm::mat4 &parent_matrix)
  {
    const auto position = translate(glm::mat4{1.0f}, _transform.position);
    const auto rotation = mat4_cast(_transform.rotation.GetQuaternion());
    const auto scale = glm::scale(glm::mat4{1.0f}, _transform.scale);
    const auto to_world = position * rotation * scale * parent_matrix;
    _render_context->EnqueueForRendering(_render_object_id, to_world);
  }

  void RenderNode::CleanUp()
  {
    std::cout << "Cleaning up " << _name << std::endl;
    _render_context->DestroyRenderObject(_render_object_id);
  }
} // core
