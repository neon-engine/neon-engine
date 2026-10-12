#include "editor-camera-submission.hpp"

#include <neon/common/transform.hpp>
#include <neon/world-system/ecs/components/camera.hpp>

namespace neon
{
  EditorCameraSubmission::EditorCameraSubmission(EditorCamera *camera, RenderPipeline *render_pipeline)
  {
    _camera = camera;
    _render_pipeline = render_pipeline;
  }

  void EditorCameraSubmission::Initialize(EntityStore &store)
  {
    _cameras = store.Query<Transform, Camera>();
  }

  void EditorCameraSubmission::Update(EntityStore &store, double delta_time)
  {
    // where the scene's camera of the window is, once: the first one that
    // draws into the window, as the world takes it
    if (!_started)
    {
      store.Each(_cameras, [&](const EntityBlock &block)
      {
        if (_started) { return; }
        const auto *transforms = block.Column<Transform>(0);
        const auto *cameras = block.Column<Camera>(1);

        for (std::size_t i = 0; i < block.count && !_started; i++)
        {
          if (cameras[i].target != RenderTarget::Window) { continue; }
          const glm::vec3 position = transforms[i].world_coordinates[3];
          _camera->StartAt(position, Rotation::FromQuaternion(glm::quat_cast(glm::mat3(transforms[i].world_coordinates))));
          _started = true;
        }
      });
      // a scene without a camera leaves the editor camera where it was
      _started = true;
    }

    _render_pipeline->SetCameraInfo(_camera->GetCameraInfo());
  }
} // neon
