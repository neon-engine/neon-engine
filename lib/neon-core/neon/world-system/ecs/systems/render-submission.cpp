#include "render-submission.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <neon/common/transform.hpp>
#include <neon/world-system/ecs/components/camera.hpp>
#include <neon/world-system/ecs/components/light.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>

namespace neon
{
  RenderSubmission::RenderSubmission(RenderPipeline *render_pipeline)
  {
    _render_pipeline = render_pipeline;
  }

  void RenderSubmission::Initialize(EntityStore &store)
  {
    _cameras = store.Query<Transform, Camera>();
    _lights = store.Query<Transform, Light>();
    _renderables = store.Query<Transform, Renderable>();
  }

  void RenderSubmission::Update(EntityStore &store, const double delta_time)
  {
    store.Each(_cameras, [&](const EntityBlock &block)
    {
      const auto *transforms = block.Column<Transform>(0);
      const auto *cameras = block.Column<Camera>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        const auto &transform = transforms[i];
        const auto &camera = cameras[i];

        const glm::vec3 position = transform.world_coordinates[3];
        const auto view = lookAt(position, position + transform.Forward(), camera.up);

        _render_pipeline->SetCameraInfo({
          camera.target,
          camera.fov,
          view,
          camera.near_plane,
          camera.far_plane
        });
      }
    });

    store.Each(_lights, [&](const EntityBlock &block)
    {
      const auto *transforms = block.Column<Transform>(0);
      auto *lights = block.Column<Light>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        lights[i].source.position = transforms[i].world_coordinates[3];
        _render_pipeline->EnqueueLightSource(lights[i].source);
      }
    });

    store.Each(_renderables, [&](const EntityBlock &block)
    {
      const auto *transforms = block.Column<Transform>(0);
      auto *renderables = block.Column<Renderable>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &renderable = renderables[i];

        // an entity is made known to the renderer the first time it is drawn,
        // so that entities which join the world later need no step of their own
        if (renderable.render_object_id < 0)
        {
          renderable.render_object_id = _render_pipeline->CreateRenderObject(renderable.render_info);
        }

        _render_pipeline->EnqueueForRendering(renderable.render_object_id, transforms[i]);
      }
    });
  }
} // neon
