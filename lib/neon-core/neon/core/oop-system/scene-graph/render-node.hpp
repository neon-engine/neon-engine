#ifndef RENDER_NODE_HPP
#define RENDER_NODE_HPP

#include "node.hpp"
#include "neon/core/render/render-context.hpp"

namespace core
{
  class RenderNode final : public Node
  {
    RenderInfo _render_info;
    RenderContext *_render_context;
    int _render_object_id = -1;

  public:
    explicit RenderNode(
      const std::string &name,
      const Transform &transform,
      const RenderInfo &render_info,
      RenderContext *render_context);

    void Initialize() override;

    void Update() override;

    void Render(const glm::mat4 &parent_matrix) override;

    void CleanUp() override;
  };
} // core

#endif //RENDER_NODE_HPP
