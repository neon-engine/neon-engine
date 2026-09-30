#ifndef SPECTATOR_HPP
#define SPECTATOR_HPP

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
} // neon

#endif //SPECTATOR_HPP
