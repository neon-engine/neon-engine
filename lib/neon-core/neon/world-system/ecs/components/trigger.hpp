#ifndef TRIGGER_HPP
#define TRIGGER_HPP

#include <cstdint>

#include <neon/physics/physics-types.hpp>

namespace neon
{
  /// Makes an entity an area that reports what enters and leaves it.
  /// Nothing is stopped or pushed by it. Its shape is what the Collider of
  /// the entity says, together with those of the entities below it.
  ///
  /// It is where its Transform puts it, and follows when that is moved.
  struct Trigger
  {
    /// The layers the trigger is in, which only queries look at.
    std::uint32_t layers = 1;

    /// The layers the trigger looks for. A body is reported when it is in
    /// one of them.
    std::uint32_t mask = 1;

    /// How many bodies are inside. Kept by the engine.
    std::uint32_t inside = 0;

    /// What the physics knows the entity as. Filled in by the engine.
    BodyId body = No_Body;

    /// Set by the engine when the trigger could not be created. It is not
    /// tried again.
    bool failed = false;
  };
} // neon

#endif //TRIGGER_HPP
