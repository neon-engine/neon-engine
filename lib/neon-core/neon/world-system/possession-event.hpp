#ifndef POSSESSION_EVENT_HPP
#define POSSESSION_EVENT_HPP

#include <neon/world-system/ecs/entity.hpp>

namespace neon
{
  /// An entity was possessed or released, see docs/possession.md. What
  /// the hooks of a script and an extension are told.
  struct PossessionEvent
  {
    Entity entity = No_Entity;

    /// Whether the entity was possessed; false when it was released.
    bool possessed = false;
  };
} // neon

#endif //POSSESSION_EVENT_HPP
