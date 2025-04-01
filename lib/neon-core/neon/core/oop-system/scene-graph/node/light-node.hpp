#ifndef LIGHT_NODE_HPP
#define LIGHT_NODE_HPP
#include "node.hpp"
#include "neon/core/render/render-pipeline.hpp"

namespace neon
{

class LightNode final : public Node {
  RenderPipeline *_render_pipeline;
  LightSource _light_source;
public:
  LightNode(
    const std::string &name,
    const Transform &transform,
    RenderPipeline *render_pipeline,
    const LightSource &light_source,
    const std::shared_ptr<Logger> &logger);

  void Initialize() override;

  void Update(double delta_time) override;

  void CleanUp() override;
};

} // neon

#endif //LIGHT_NODE_HPP
