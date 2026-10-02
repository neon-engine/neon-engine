#ifndef PERSISTENT_HPP
#define PERSISTENT_HPP

#include <neon/reflection/type-builder.hpp>

namespace neon
{
  /// Marks an entity that stays when the world changes scene: the player,
  /// what the game keeps between levels. Everything else of the old scene is
  /// destroyed before the new one is read. Its children stay with it. It
  /// holds nothing; being there is what it says.
  struct Persistent
  {
    // Flecs and the store need a component to have a size, and a reflection
    // type at least one field, so that a scene writes `Persistent: {}`
    bool keep = true;
  };

  inline void Describe(TypeBuilder<Persistent> &type)
  {
    type.Named("Persistent", "Keeps the entity when the world changes scene");

    type.Field("keep", &Persistent::keep)
        .Describe("Whether it stays. Written for completeness; the component being there is what counts");
  }
} // neon

#endif //PERSISTENT_HPP
