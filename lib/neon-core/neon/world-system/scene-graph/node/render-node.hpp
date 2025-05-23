#ifndef RENDER_NODE_HPP
#define RENDER_NODE_HPP

#include "node.hpp"
#include "neon/render/render-pipeline.hpp"

namespace neon
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
      RenderPipeline *render_pipeline,
      const std::shared_ptr<Logger> &logger);

    void Initialize() override;

    void Update(double delta_time) override;

    void CleanUp() override;
  };
} // neon

#endif //RENDER_NODE_HPP
