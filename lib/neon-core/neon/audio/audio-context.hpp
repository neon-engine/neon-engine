#ifndef AUDIO_CONTEXT_HPP
#define AUDIO_CONTEXT_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "sound-info.hpp"

namespace neon
{
  /// What the rest of the engine sees of audio.
  ///
  /// A sound is created from a file and known by an id from then on. It
  /// plays when it is told to, and as often as it is told to. Two sounds of
  /// the same file are two sounds, which can play at the same time.
  ///
  /// A function that is handed an id it does not know does nothing.
  class AudioContext
  {
  protected:
    ~AudioContext() = default;

  public:
    /// Creates a sound. It does not play yet. Returns -1 when the file
    /// cannot be read or is not a sound, and logs why. A sound of a group
    /// that is not there is reported and put among the effects.
    virtual int CreateSound(const SoundInfo &sound_info) = 0;

    virtual void DestroySound(int sound_id) = 0;

    /// Keeps the bytes of a sound file under a name: WAV, FLAC, MP3, or Ogg
    /// Vorbis, as a file would hold them. A sound whose path is
    /// `sound://<name>` is created from them, see SoundMemory. It is how an
    /// extension plays what it reads out of an archive of its own.
    ///
    /// A name that is set again names the new bytes from then on; sounds
    /// that were created keep what they were created from. Returns false
    /// for no name or no bytes. Whether the bytes are a sound is found when
    /// one is created from them.
    virtual bool SetSound(const std::string &name, std::vector<std::uint8_t> bytes) = 0;

    /// Plays a sound from its start. A sound that is playing starts over.
    virtual void Play(int sound_id) = 0;

    /// Stops a sound. Play() starts it from its start again.
    virtual void Stop(int sound_id) = 0;

    /// Whether a sound is playing. It is not any more once it has ended.
    virtual bool IsPlaying(int sound_id) = 0;

    /// Plays a sound from its start, rising from silence to its volume over
    /// `seconds`.
    virtual void FadeIn(int sound_id, double seconds) = 0;

    /// Fades a sound from how loud it is now to `volume` over `seconds`. The
    /// fade is on top of the volume of the sound, which SetVolume() sets and
    /// which it leaves as it is: 1 is the volume of the sound, 0 is silence.
    /// A sound that fades to silence keeps playing. Play() and FadeIn() start
    /// a sound at 1 again, and a fade puts off a fade out that was under way.
    virtual void FadeTo(int sound_id, float volume, double seconds) = 0;

    /// Fades a sound out over `seconds`, and stops it then. It is playing
    /// until then.
    virtual void FadeOut(int sound_id, double seconds) = 0;

    /// Fades one sound out while another fades in, as from one piece of music
    /// to the next. The first is stopped once it is silent.
    void Crossfade(const int from_sound_id, const int to_sound_id, const double seconds)
    {
      FadeOut(from_sound_id, seconds);
      FadeIn(to_sound_id, seconds);
    }

    virtual void SetVolume(int sound_id, float volume) = 0;

    virtual void SetPitch(int sound_id, float pitch) = 0;

    virtual void SetLooping(int sound_id, bool looping) = 0;

    /// Places a sound in the world. `velocity` is in units per second. It
    /// has no effect on a sound that was created without a place.
    virtual void SetPosition(int sound_id, const glm::vec3 &position, const glm::vec3 &velocity) = 0;

    virtual void SetListener(const ListenerInfo &listener_info) = 0;

    /// The loudness of everything. 1 leaves it as it is, 0 is silence. It is
    /// on top of the volumes of the groups.
    virtual void SetMasterVolume(float volume) = 0;

    /// Adds a group of sounds, at a volume of 1, which is held still while
    /// the game is paused, as the effects are. Music, effects, voices, and
    /// ambience are there from the start, with the groups the settings
    /// declare. Adding a group that is there does nothing.
    virtual void AddGroup(const std::string &group) = 0;

    /// Stops every sound of a group. The sounds of the other groups go on;
    /// stopping the music leaves the ambience as it is, and so does
    /// stopping the effects. A group that is not there is reported.
    virtual void StopGroup(const std::string &group) = 0;

    /// Holds the sounds of the groups that pause with the game where they
    /// are: the effects, the voices, the ambience, and the groups of a
    /// game. The music plays on. With false they go on from where they
    /// were. A sound that is held counts as playing, so that nothing takes
    /// it for ended.
    virtual void SetPaused(bool paused) = 0;

    /// The loudness of every sound of a group. 1 leaves them as they are, 0
    /// is silence. A group that is not there is reported and left alone.
    virtual void SetGroupVolume(const std::string &group, float volume) = 0;

    /// The volume of a group, or 0 for a group that is not there.
    virtual float GetGroupVolume(const std::string &group) = 0;

    /// Tells the audio how much time the application advanced by, in
    /// seconds. Called once per frame. Audio that goes to a sound card keeps
    /// its own time and has no use for it. Audio that goes nowhere mixes as
    /// much as it is told here.
    virtual void Advance(double delta_time) = 0;

    /// How loud what was last put out is, from 0 for silence to 1 for as
    /// loud as it gets. For meters, and for checking that something is heard
    /// without hearing it.
    virtual float GetOutputLevel() = 0;
  };
} // neon

#endif //AUDIO_CONTEXT_HPP
