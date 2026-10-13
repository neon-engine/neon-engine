#ifndef POSSESSABLE_HPP
#define POSSESSABLE_HPP

#include <neon/reflection/type-builder.hpp>

namespace neon
{
  /// Marks an entity that can be driven: a character, a vehicle, a camera.
  /// Whoever drives, the player in a game or the editor in play mode,
  /// possesses one such entity at a time, see docs/possession.md. The
  /// world then marks the one that is driven with Possessed, the input
  /// reaches it alone, and the view follows it. It holds nothing; being
  /// there is what it says.
  struct Possessable
  {
    // Flecs and the store need a component to have a size, and a reflection
    // type at least one field, so that a scene writes `Possessable: Default`
    bool drivable = true;
  };

  inline void Describe(TypeBuilder<Possessable> &type)
  {
    type.Named("Possessable", "Marks an entity that can be possessed: driven by the input and seen from");

    type.Field("drivable", &Possessable::drivable)
        .Describe("Whether it can be driven. Written for completeness; the component being there is what counts");
  }
} // neon

#endif //POSSESSABLE_HPP
