#include "ray-casting.hpp"

#include <neon/common/transform.hpp>
#include <neon/world-system/ecs/components/ray-cast.hpp>

namespace neon
{
  RayCasting::RayCasting(PhysicsContext *physics)
  {
    _physics = physics;
  }

  void RayCasting::Initialize(EntityStore &store)
  {
    _rays = store.Query<Transform, RayCast>();
  }

  void RayCasting::Update(EntityStore &store, double delta_time)
  {
    store.Each(_rays, [&](const EntityBlock &block)
    {
      const auto *transforms = block.Column<Transform>(0);
      auto *rays = block.Column<RayCast>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        RayCast &ray = rays[i];

        // the owner, by its path, found once and again when it is gone
        if (ray.owner.empty()) { ray.owner_entity = No_Entity; }
        else if (ray.owner_entity == No_Entity || !store.IsAlive(ray.owner_entity))
        {
          ray.owner_entity = store.FindEntity(ray.owner);
        }

        ray.is_hitting = false;
        ray.hit = No_Entity;
        if (!ray.enabled || _physics == nullptr) { continue; }

        Ray cast;
        cast.origin = transforms[i].world_coordinates[3];
        cast.direction = transforms[i].Forward();
        if (ray.length > 0.0f) { cast.distance = ray.length; }

        // past the bodies of whoever the ray acts for, which the ray may
        // start inside of
        QueryFilter filter;
        filter.mask = ray.mask;
        filter.triggers = ray.triggers;
        filter.ignore = ray.owner_entity != No_Entity ? ray.owner_entity : block.entities[i];

        RayHit hit;
        if (!_physics->CastRay(cast, filter, hit)) { continue; }

        ray.is_hitting = true;
        ray.hit = hit.entity;
        ray.point = hit.point;
        ray.normal = hit.normal;
        ray.distance = hit.distance;
      }
    });
  }
} // neon
