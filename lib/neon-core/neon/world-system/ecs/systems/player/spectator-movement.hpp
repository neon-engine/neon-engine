#ifndef SPECTATOR_MOVEMENT_HPP
#define SPECTATOR_MOVEMENT_HPP

#include <neon/input/input-context.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Moves and turns the entity that carries a Spectator and is possessed
  /// by the input, see docs/possession.md: the action `move` of the input
  /// map moves it, `look` turns it. A spectator that is not possessed
  /// stays where it is.
  class SpectatorMovement final : public EntitySystem
  {
    InputContext *_input_context;
    QueryId _query = 0;

  public:
    explicit SpectatorMovement(InputContext *input_context);

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //SPECTATOR_MOVEMENT_HPP
