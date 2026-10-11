#include "ui-surface-pointing.hpp"

#include <algorithm>

#include <neon/common/transform.hpp>
#include <neon/world-system/ecs/components/ray-cast.hpp>
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
    _rays = store.Query<RayCast>();
  }

  void UiSurfacePointing::Update(EntityStore &store, double delta_time)
  {
    // the button of the pointer, the left button of the mouse, which is
    // all there is of it while the cursor is hidden and the player looks
    // around, and accept for a controller
    const InputState &input = _input_context->GetInputState();
    const bool is_down = input[Action::Pointer_Primary] || input.IsMouseButtonDown(MouseButton::Left) ||
      input[Action::Ui_Accept];

    std::vector<int> pointed;

    store.Each(_rays, [&](const EntityBlock &block)
    {
      const auto *rays = block.Column<RayCast>(0);

      for (std::size_t i = 0; i < block.count; i++)
      {
        const RayCast &ray = rays[i];
        if (!ray.is_hitting || ray.hit == No_Entity) { continue; }

        auto *view = store.Get<UiSurfaceView>(ray.hit);
        const auto *screen = store.Get<Transform>(ray.hit);
        if (view == nullptr || screen == nullptr || view->surface < 0) { continue; }

        // a screen that does not want to be used from so far
        if (view->reach > 0.0f && ray.distance > view->reach) { continue; }

        // where on the square the ray hit, from its left top corner
        const auto place = glm::vec3(inverse(screen->world_coordinates) * glm::vec4(ray.point, 1.0f));
        _ui_context->SetPointerUv(view->surface, place.x + 0.5f, 0.5f - place.y, is_down);

        // who points: the owner the ray names, or the ray's own entity
        view->pointed_by = ray.owner_entity != No_Entity ? ray.owner_entity : block.entities[i];
        pointed.push_back(view->surface);
      }
    });

    for (const int surface : _pointed)
    {
      if (std::ranges::find(pointed, surface) == pointed.end()) { _ui_context->ClearPointer(surface); }
    }

    _pointed = std::move(pointed);
  }
} // neon
