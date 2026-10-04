#include "render-submission.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <neon/common/transform.hpp>
#include <neon/world-system/ecs/components/camera.hpp>
#include <neon/world-system/ecs/components/light.hpp>
#include <neon/world-system/ecs/components/renderable.hpp>
#include <neon/world-system/ecs/components/sky.hpp>

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
    _skies = store.Query<Sky>();
    _renderables = store.Query<Transform, Renderable>();
  }

  void RenderSubmission::Update(EntityStore &store, const double delta_time)
  {
    // The shaders are told how long the world has run. This is asked once
    // for every frame the world takes part in, so the time stands still
    // while the world does, as while a game is paused.
    _time += delta_time;
    _render_pipeline->SetTime(_time, delta_time);

    store.Each(_cameras, [&](const EntityBlock &block)
    {
      const auto *transforms = block.Column<Transform>(0);
      const auto *cameras = block.Column<Camera>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        const auto &transform = transforms[i];
        const auto &camera = cameras[i];

        const glm::vec3 position = transform.world_coordinates[3];
        // A camera that rolls with its entity has what is up turn with it,
        // so that a roll of the entity rolls the view, as it turns what
        // hangs from the camera. Turning around or looking up and down
        // leaves the view level, since up then stays in the plane the camera
        // looks along. Any other camera is level whatever it hangs from.
        const glm::vec3 up = camera.rolls_with_entity
                               ? normalize(glm::mat3(transform.world_coordinates) * camera.up)
                               : camera.up;
        const auto view = lookAt(position, position + transform.Forward(), up);

        CameraInfo info;
        info.target = camera.target;
        info.fov = camera.fov;
        info.view = view;
        info.near = camera.near_plane;
        info.far = camera.far_plane;
        info.effects = camera.effects;
        info.screen_effects = camera.screen_effects;
        info.texture = camera.texture;
        info.width = camera.texture_width;
        info.height = camera.texture_height;

        _render_pipeline->SetCameraInfo(info);
      }
    });

    store.Each(_lights, [&](const EntityBlock &block)
    {
      const auto *transforms = block.Column<Transform>(0);
      auto *lights = block.Column<Light>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &source = lights[i].source;
        source.position = transforms[i].world_coordinates[3];

        // the renderer names a light by the entity it belongs to
        if (source.id.empty()) { source.id = store.GetName(block.entities[i]); }

        _render_pipeline->EnqueueLightSource(source);
      }
    });

    // a scene has one sky: the first that is found is drawn
    bool has_sky = false;
    store.Each(_skies, [&](const EntityBlock &block)
    {
      if (has_sky || block.count == 0) { return; }

      _render_pipeline->SetSky(block.Column<Sky>(0)[0].info);
      has_sky = true;
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
          renderable.mesh_version_drawn = renderable.render_info.mesh_version;
        }

        // the renderer could not create it, and has said why. It knows no
        // render object by that id, so there is nothing to draw
        if (renderable.render_object_id < 0) { continue; }

        // a mesh that was changed since it was last drawn, by a rope that
        // moved, is handed to the renderer again
        const auto &info = renderable.render_info;
        if (info.mesh != nullptr && info.mesh_version != renderable.mesh_version_drawn)
        {
          _render_pipeline->UpdateRenderObjectMesh(renderable.render_object_id, *info.mesh);
          renderable.mesh_version_drawn = info.mesh_version;
        }

        _render_pipeline->EnqueueForRendering(renderable.render_object_id, transforms[i]);
      }
    });
  }
} // neon
