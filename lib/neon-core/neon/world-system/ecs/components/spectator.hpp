#ifndef SPECTATOR_HPP
#define SPECTATOR_HPP

#include <neon/reflection/type-builder.hpp>

namespace neon
{
  /// Lets the input move an entity freely through the world.
  struct Spectator
  {
    /// Units per second.
    float move_speed = 2.5f;

    /// Degrees per unit the mouse moved.
    float look_speed = 0.1f;
  };

  inline void Describe(TypeBuilder<Spectator> &type)
  {
    type.Named("Spectator", "Lets the input move an entity freely through the world");

    type.Field("move_speed", &Spectator::move_speed)
        .Describe("Units per second");

    type.Field("look_speed", &Spectator::look_speed)
        .Describe("Degrees per unit the mouse moved");
  }
} // neon

#endif //SPECTATOR_HPP
