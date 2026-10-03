#ifndef SOUND_SOURCE_HPP
#define SOUND_SOURCE_HPP

#include <optional>

#include <neon/audio/sound-info.hpp>
#include <neon/reflection/type-builder.hpp>

namespace neon
{
  /// A fade that a game asked a SoundSource for.
  struct SoundFade
  {
    enum class Kind
    {
      /// Plays the sound from its start, rising from silence.
      In,
      /// Takes the sound from how loud it is to `volume`.
      To,
      /// Takes the sound to silence and stops it.
      Out
    };

    Kind kind = Kind::To;
    float volume = 1.0f;
    double seconds = 0.0;
  };

  /// Makes an entity a source of sound. With a place, the sound comes from
  /// where the Transform of the entity puts it.
  ///
  /// `playing` is how a game starts and stops the sound. It is set to false
  /// by the engine when a sound that does not loop has ended, and when one
  /// that faded out has stopped.
  struct SoundSource
  {
    SoundInfo sound;

    /// Whether the sound is to play. True plays it as soon as the entity
    /// joins the world.
    bool playing = true;

    /// A fade that is handed to the audio in the next frame, and forgotten
    /// then. Set through the functions below.
    std::optional<SoundFade> fade;

    /// What the audio system knows the sound as. Filled in by the engine.
    /// -1 until then, and -2 when the sound could not be created.
    int sound_id = -1;

    /// What the engine last told the audio system, to tell it changes only.
    bool was_playing = false;
    glm::vec3 last_position{0.0f};
    bool has_last_position = false;

    /// Plays the sound from its start, rising from silence to its volume
    /// over `seconds`.
    void FadeIn(const double seconds)
    {
      playing = true;
      fade = SoundFade{.kind = SoundFade::Kind::In, .seconds = seconds};
    }

    /// Takes the sound from how loud it is to `volume` over `seconds`, on
    /// top of the volume of the sound. It keeps playing at silence. A sound
    /// that does not play is left alone.
    void FadeTo(const float volume, const double seconds)
    {
      fade = SoundFade{.kind = SoundFade::Kind::To, .volume = volume, .seconds = seconds};
    }

    /// Takes the sound to silence over `seconds` and stops it then, which
    /// sets `playing` to false. A sound that does not play is left alone.
    void FadeOut(const double seconds)
    {
      fade = SoundFade{.kind = SoundFade::Kind::Out, .volume = 0.0f, .seconds = seconds};
    }
  };

  /// Fades one source out while another fades in, as from one piece of music
  /// to the next.
  inline void Crossfade(SoundSource &from, SoundSource &to, const double seconds)
  {
    from.FadeOut(seconds);
    to.FadeIn(seconds);
  }

  /// What the engine keeps for itself, the id of the sound and what it last
  /// told the audio system, is not described, and neither is a fade, which
  /// a game asks for while it runs.
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
        .Describe("From this distance on it does not get quieter, which is silence with a linear falloff");

    type.Choice("falloff", [](SoundSource &source) -> SoundFalloff & { return source.sound.falloff; }, {"inverse", "linear"})
        .Describe("How it gets quieter between the two distances: inverse stays heard, linear ends in silence");

    // a project declares groups of its own, so a name is not checked here.
    // The audio says when a sound is of a group that is not there
    type.Field("group", [](SoundSource &source) -> std::string & { return source.sound.group; })
        .Describe("The group whose volume the sound is played at: music, effects, voices, ambience, or one of the game");
  }
} // neon

#endif //SOUND_SOURCE_HPP
