#ifndef SOUND_SOURCE_HPP
#define SOUND_SOURCE_HPP

#include <neon/audio/sound-info.hpp>
#include <neon/reflection/type-builder.hpp>

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

  /// What the engine keeps for itself, the id of the sound and what it last
  /// told the audio system, is not described.
  inline void Describe(TypeBuilder<SoundSource> &type)
  {
    type.Named("SoundSource", "Makes an entity a source of sound");

    type.Field("sound", [](SoundSource &source) -> std::string & { return source.sound.path; })
        .Required()
        .Describe("Virtual path of the file");

    type.Field("playing", &SoundSource::playing)
        .Describe("Whether the sound plays. True plays it when the entity joins the world");

    type.Field("looping", [](SoundSource &source) -> bool & { return source.sound.looping; })
        .Describe("Whether it starts over when it has ended");

    type.Field("volume", [](SoundSource &source) -> float & { return source.sound.volume; })
        .AtLeast(0)
        .Describe("1 is the loudness of the file, 0 is silence");

    type.Field("pitch", [](SoundSource &source) -> float & { return source.sound.pitch; })
        .Above(0)
        .Describe("1 is the speed of the file, 2 is twice as fast and an octave higher");

    type.Field("spatial", [](SoundSource &source) -> bool & { return source.sound.spatial; })
        .Describe("Whether the sound has a place in the world");

    type.Field("min_distance", [](SoundSource &source) -> float & { return source.sound.min_distance; })
        .AtLeast(0)
        .Describe("Up to this distance the sound is as loud as it gets");

    type.Field("max_distance", [](SoundSource &source) -> float & { return source.sound.max_distance; })
        .AtLeast(0)
        .Describe("From this distance on it does not get quieter");
  }
} // neon

#endif //SOUND_SOURCE_HPP
