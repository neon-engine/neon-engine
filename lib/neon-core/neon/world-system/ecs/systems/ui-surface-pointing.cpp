#include "ui-surface-pointing.hpp"

#include <limits>

#include <neon/world-system/ecs/components/camera.hpp>
#include <neon/world-system/ecs/components/ui-surface-view.hpp>

namespace neon
{
  UiSurfacePointing::UiSurfacePointing(UiContext *ui_context, InputContext *input_context)
  {
    _ui_context = ui_context;
    _input_context = input_context;
  }

  void UiSurfacePointing::Initialize(EntityStore &store)
  {
    _cameras = store.Query<Transform, Camera>();
    _surfaces = store.Query<Transform, UiSurfaceView>();
    _player = store.FindComponent("Player");
  }

  bool UiSurfacePointing::Hit(
    const Transform &transform,
    const glm::vec3 &origin,
    const glm::vec3 &direction,
    float &u,
    float &v,
    float &distance)
  {
    // in the space of the square, where it lies from -0.5 to 0.5 at z = 0
    const glm::mat4 to_local = inverse(transform.world_coordinates);
    const auto local_origin = glm::vec3(to_local * glm::vec4(origin, 1.0f));
    const auto local_direction = glm::vec3(to_local * glm::vec4(direction, 0.0f));

    // towards the face, which looks along +Z, and not away from it or
    // along it
    if (local_direction.z >= -1e-6f) { return false; }

    const float along = -local_origin.z / local_direction.z;
    if (along <= 0.0f) { return false; }

    const glm::vec3 place = local_origin + along * local_direction;
    if (place.x < -0.5f || place.x > 0.5f || place.y < -0.5f || place.y > 0.5f) { return false; }

    // the top of the surface is the top of the square
    u = place.x + 0.5f;
    v = 0.5f - place.y;
    distance = along * length(direction);
    return true;
  }

  void UiSurfacePointing::Update(EntityStore &store, double delta_time)
  {
    // the camera that draws the window is where the player looks from
    bool has_camera = false;
    Entity camera = No_Entity;
    glm::vec3 origin{0.0f};
    glm::vec3 direction{0.0f, 0.0f, -1.0f};

    store.Each(_cameras, [&](const EntityBlock &block)
    {
      const auto *transforms = block.Column<Transform>(0);
      const auto *cameras = block.Column<Camera>(1);

      for (std::size_t i = 0; i < block.count && !has_camera; i++)
      {
        if (cameras[i].target != RenderTarget::Window) { continue; }

        has_camera = true;
        camera = block.entities[i];
        origin = transforms[i].world_coordinates[3];
        direction = transforms[i].Forward();
      }
    });

    int nearest = No_Ui_Surface;
    Entity nearest_screen = No_Entity;
    float nearest_distance = std::numeric_limits<float>::max();
    float nearest_u = 0.0f;
    float nearest_v = 0.0f;

    if (has_camera)
    {
      store.Each(_surfaces, [&](const EntityBlock &block)
      {
        const auto *transforms = block.Column<Transform>(0);
        const auto *views = block.Column<UiSurfaceView>(1);

        for (std::size_t i = 0; i < block.count; i++)
        {
          const UiSurfaceView &view = views[i];
          if (view.surface < 0) { continue; }

          float u = 0.0f;
          float v = 0.0f;
          float distance = 0.0f;
          if (!Hit(transforms[i], origin, direction, u, v, distance)) { continue; }
          if (view.reach > 0.0f && distance > view.reach) { continue; }
          if (distance >= nearest_distance) { continue; }

          nearest = view.surface;
          nearest_screen = block.entities[i];
          nearest_distance = distance;
          nearest_u = u;
          nearest_v = v;
        }
      });
    }

    if (nearest != No_Ui_Surface)
    {
      // the button of the pointer, the left button of the mouse, which is
      // all there is of it while the cursor is hidden and the player looks
      // around, and accept for a controller
      const InputState &input = _input_context->GetInputState();
      const bool is_down = input[Action::Pointer_Primary] || input.IsMouseButtonDown(MouseButton::Left) ||
        input[Action::Ui_Accept];
      _ui_context->SetPointerUv(nearest, nearest_u, nearest_v, is_down);

      // who points: the player the camera belongs to, which is the nearest
      // entity from the camera up that carries a Player, or the camera
      Entity pointing = camera;
      if (_player != No_Component)
      {
        for (Entity above = camera; above != No_Entity; above = store.GetParent(above))
        {
          if (!store.HasComponent(above, _player)) { continue; }

          pointing = above;
          break;
        }
      }

      if (auto *view = store.Get<UiSurfaceView>(nearest_screen); view != nullptr) { view->pointed_by = pointing; }
    }

    if (_pointed != No_Ui_Surface && _pointed != nearest) { _ui_context->ClearPointer(_pointed); }

    // said when it changes, for a HUD that shows that something can be used
    if ((nearest != No_Ui_Surface) != (_pointed != No_Ui_Surface))
    {
      _ui_context->SetFlag("pointing", nearest != No_Ui_Surface);
    }

    _pointed = nearest;
  }
} // neon
