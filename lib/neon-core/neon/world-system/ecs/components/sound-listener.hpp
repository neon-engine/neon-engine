#ifndef SOUND_LISTENER_HPP
#define SOUND_LISTENER_HPP

#include <glm/glm.hpp>
#include <neon/reflection/type-builder.hpp>

namespace neon
{
  /// Makes an entity the point the world is heard from, which is where its
  /// Transform puts it. It usually sits on the entity that carries the
  /// camera.
  struct SoundListener
  {
    /// The loudness of everything that is heard. 1 leaves it as it is.
    float volume = 1.0f;

    /// Kept by the engine, to work out how fast the listener moves.
    glm::vec3 last_position{0.0f};
    bool has_last_position = false;
  };

  inline void Describe(TypeBuilder<SoundListener> &type)
  {
    type.Named("SoundListener", "Makes an entity the point the world is heard from");

    type.Field("volume", &SoundListener::volume)
        .AtLeast(0)
        .Describe("The loudness of everything that is heard");
  }
} // neon

#endif //SOUND_LISTENER_HPP
