#ifndef EDITOR_CAMERA_SUBMISSION_HPP
#define EDITOR_CAMERA_SUBMISSION_HPP

#include <neon/render/render-pipeline.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

#include "editor-camera.hpp"

namespace neon
{
  /// Hands the editor camera to the pipeline in place of the scene's
  /// camera, after the world has handed its own, so that the viewport
  /// shows what the editor camera sees. The first time, the editor camera
  /// is put where the scene's camera is, so that the editor opens on what
  /// the game would show. Added to the world after placing, where the
  /// world's own submission runs.
  class EditorCameraSubmission final : public EntitySystem
  {
    EditorCamera *_camera;
    RenderPipeline *_render_pipeline;
    QueryId _cameras = 0;
    bool _started = false;

  public:
    EditorCameraSubmission(EditorCamera *camera, RenderPipeline *render_pipeline);

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //EDITOR_CAMERA_SUBMISSION_HPP
