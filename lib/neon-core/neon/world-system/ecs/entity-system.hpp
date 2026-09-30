#ifndef ENTITY_SYSTEM_HPP
#define ENTITY_SYSTEM_HPP

#include "entity-store.hpp"

namespace neon
{
  /// Behaviour of the world. A system works on every entity that carries the
  /// components it asks for, once per frame.
  class EntitySystem
  {
  public:
    virtual ~EntitySystem() = default;

    /// Called once, after the components of the engine are registered and
    /// before the scene is populated. The place to create queries.
    virtual void Initialize(EntityStore &store) = 0;

    /// Called once per frame. `delta_time` is in seconds.
    virtual void Update(EntityStore &store, double delta_time) = 0;
  };
} // neon

#endif //ENTITY_SYSTEM_HPP
