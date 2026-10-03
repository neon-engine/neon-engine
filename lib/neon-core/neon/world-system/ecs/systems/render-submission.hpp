#ifndef RENDER_SUBMISSION_HPP
#define RENDER_SUBMISSION_HPP

#include <neon/render/render-pipeline.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Hands what is to be drawn to the render pipeline: the camera, the
  /// lights, the sky, and every entity that carries a Renderable.
  class RenderSubmission final : public EntitySystem
  {
    RenderPipeline *_render_pipeline;
    QueryId _cameras = 0;
    QueryId _lights = 0;
    QueryId _skies = 0;
    QueryId _renderables = 0;

  public:
    explicit RenderSubmission(RenderPipeline *render_pipeline);

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //RENDER_SUBMISSION_HPP
