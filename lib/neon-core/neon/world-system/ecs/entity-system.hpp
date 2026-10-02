#ifndef ENTITY_SYSTEM_HPP
#define ENTITY_SYSTEM_HPP

#include "entity-store.hpp"

namespace neon
{
  /// Behaviour of the world. A system works on every entity that carries the
  /// components it asks for.
  ///
  /// Update is called once per frame, with the time the frame took. It is for
  /// what belongs to a frame: reading the input, moving a camera, what is
  /// shown.
  ///
  /// FixedUpdate is called once per step of the world, with the time of a
  /// step, which is always the same. It is for what the game is decided by:
  /// forces, velocities, timers, anything that is moved by the physics or
  /// collides. How often it is called in a frame depends on how long the
  /// frame took, and can be never. What it does must not depend on the frame
  /// rate, so a game behaves the same at 30 and at 300 frames per second.
  class EntitySystem
  {
  public:
    virtual ~EntitySystem() = default;

    /// Called once, before any system is initialized. The place to register
    /// the components the system brings, so that every system finds every
    /// component when it makes its queries, whichever is initialized first.
    /// Does nothing unless a system says otherwise.
    virtual void Register(EntityStore &store) {}

    /// Called once, after every component is registered and before the
    /// scene is populated. The place to create queries.
    virtual void Initialize(EntityStore &store) = 0;

    /// Called once per frame. `delta_time` is in seconds.
    virtual void Update(EntityStore &store, double delta_time) = 0;

    /// Called once per step of the world, before the frame is updated.
    /// `fixed_delta_time` is the length of a step in seconds. Does nothing
    /// unless a system says otherwise.
    virtual void FixedUpdate(EntityStore &store, double fixed_delta_time) {}

    /// Called once per frame, after every entity was placed in the world and
    /// before the frame is drawn. `blend` says how far the frame lies between
    /// the last two steps, from 0 to 1. It is for placing what is drawn
    /// between the two, so that it moves smoothly whatever the frame rate is.
    /// What is changed here is `world_coordinates` of a Transform, which is
    /// what is drawn, and nothing a game decides by. Does nothing unless a
    /// system says otherwise.
    virtual void Interpolate(EntityStore &store, double blend) {}
  };
} // neon

#endif //ENTITY_SYSTEM_HPP
