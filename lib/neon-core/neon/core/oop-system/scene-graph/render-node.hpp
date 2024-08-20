#ifndef RENDER_NODE_HPP
#define RENDER_NODE_HPP

#include "node.hpp"
#include "neon/core/render/render-pipeline.hpp"

namespace core
{
  class RenderNode final : public Node
  {
    RenderInfo _render_info;
    RenderPipeline *_render_pipeline;
    int _render_object_id = -1;

  public:
    explicit RenderNode(
      const std::string &name,
      const Transform &transform,
      const RenderInfo &render_info,
      RenderPipeline *render_pipeline);

    void Initialize() override;

    void Update() override;

    void CleanUp() override;
  };
} // core

#endif //RENDER_NODE_HPP
