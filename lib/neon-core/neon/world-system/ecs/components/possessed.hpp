#ifndef POSSESSED_HPP
#define POSSESSED_HPP

#include <neon/reflection/type-builder.hpp>

namespace neon
{
  /// Marks the one entity that is possessed: the one the input drives and
  /// the view follows, see docs/possession.md. The world sets it, on
  /// exactly one entity at a time, when something possesses the entity, and
  /// takes it away when it is released; no file holds it. A system that
  /// drives an entity by the input queries for it next to the component it
  /// drives, as FirstPersonControllerSystem and SpectatorMovement do, so
  /// that an entity that is not possessed is left alone.
  struct Possessed
  {
    // Flecs and the store need a component to have a size
    bool driven = true;
  };

  /// Not read from a file, so it has no format; described for the scripts,
  /// which reach it by its name.
  inline void Describe(TypeBuilder<Possessed> &type)
  {
    type.Named("Possessed", "Marks the one entity that is possessed. Set by the world");

    type.Field("driven", &Possessed::driven)
        .Describe("Written for completeness; the component being there is what counts");
  }
} // neon

#endif //POSSESSED_HPP
