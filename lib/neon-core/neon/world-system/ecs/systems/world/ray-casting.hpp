#ifndef RAY_CASTING_HPP
#define RAY_CASTING_HPP

#include <neon/physics/physics-context.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Casts the ray of every entity that carries a `RayCast` and a
  /// Transform, every frame, through the physics, and writes what it hits
  /// onto the component. The ray starts where the entity is and goes where
  /// it looks, as far as its `length`, through the layers of its `mask`,
  /// past the bodies of its owner. Nothing is told; whoever cares reads the
  /// component, as UiSurfacePointing does to place the pointer on a
  /// screen. Without physics, nothing is hit.
  ///
  /// The owner a ray names is found by its path once and kept on the
  /// component, and found again when it is gone.
  ///
  /// An application adds it before what reads the rays:
  ///
  ///     world.AddSystem(std::make_unique<neon::RayCasting>(&physics_system));
  class RayCasting final : public EntitySystem
  {
    PhysicsContext *_physics;
    QueryId _rays = 0;

  public:
    explicit RayCasting(PhysicsContext *physics);

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //RAY_CASTING_HPP
