#ifndef RENDER_SUBMISSION_HPP
#define RENDER_SUBMISSION_HPP

#include <neon/render/render-pipeline.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Hands what is to be drawn to the render pipeline: the camera, the
  /// lights, the sky, and every entity that carries a Renderable.
  ///
  /// The camera of the window is the possessed entity's, see
  /// docs/possession.md: the one on the entity itself or on an entity
  /// below it that draws into the window. When the possessed entity has
  /// none, or nothing is possessed, every camera of the window is handed
  /// over as before, and the pipeline draws with the last. Cameras that
  /// draw into a texture are handed over whatever is possessed.
  class RenderSubmission final : public EntitySystem
  {
    RenderPipeline *_render_pipeline;

    /// Whether the entity is the possessed one or lies below it.
    [[nodiscard]] static bool IsOfThePossessed(EntityStore &store, Entity entity, Entity possessed);

    // the seconds the world has run, which the shaders are told
    double _time = 0.0;
    QueryId _cameras = 0;
    QueryId _lights = 0;
    QueryId _skies = 0;
    QueryId _renderables = 0;
    QueryId _possessed = 0;

  public:
    explicit RenderSubmission(RenderPipeline *render_pipeline);

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //RENDER_SUBMISSION_HPP
