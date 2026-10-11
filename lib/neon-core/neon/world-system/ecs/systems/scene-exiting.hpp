#ifndef SCENE_EXITING_HPP
#define SCENE_EXITING_HPP

#include <neon/world-system/ecs/entity-system.hpp>
#include <neon/world-system/world-system.hpp>

namespace neon
{
  /// Changes the scene when a body is inside the Trigger of an entity that
  /// carries a `SceneExit`: the way out of a level. It asks the world for
  /// the scene the exit names, which is read at the start of the next
  /// frame. The physics registers Trigger; in a world without it the exits
  /// never open, and the system says nothing.
  ///
  /// An application adds it after the physics has taken its steps, so that
  /// a body that entered in this frame is seen:
  ///
  ///     world.AddSystem(std::make_unique<neon::SceneExiting>(&world));
  class SceneExiting final : public EntitySystem
  {
    WorldSystem *_world;
    QueryId _exits = 0;
    ComponentId _trigger = No_Component;

  public:
    explicit SceneExiting(WorldSystem *world);

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //SCENE_EXITING_HPP
