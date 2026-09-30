#ifndef SOUND_SOURCE_HPP
#define SOUND_SOURCE_HPP

#include <neon/audio/sound-info.hpp>

namespace neon
{
  /// Makes an entity a source of sound. With a place, the sound comes from
  /// where the Transform of the entity puts it.
  ///
  /// `playing` is how a game starts and stops the sound. It is set to false
  /// by the engine when a sound that does not loop has ended.
  struct SoundSource
  {
    SoundInfo sound;

    /// Whether the sound is to play. True plays it as soon as the entity
    /// joins the world.
    bool playing = true;

    /// What the audio system knows the sound as. Filled in by the engine.
    /// -1 until then, and -2 when the sound could not be created.
    int sound_id = -1;

    /// What the engine last told the audio system, to tell it changes only.
    bool was_playing = false;
    glm::vec3 last_position{0.0f};
    bool has_last_position = false;
  };
} // neon

#endif //SOUND_SOURCE_HPP
