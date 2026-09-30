#ifndef AUDIO_CONTEXT_HPP
#define AUDIO_CONTEXT_HPP

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
    /// cannot be read or is not a sound, and logs why.
    virtual int CreateSound(const SoundInfo &sound_info) = 0;

    virtual void DestroySound(int sound_id) = 0;

    /// Plays a sound from its start. A sound that is playing starts over.
    virtual void Play(int sound_id) = 0;

    /// Stops a sound. Play() starts it from its start again.
    virtual void Stop(int sound_id) = 0;

    /// Whether a sound is playing. It is not any more once it has ended.
    virtual bool IsPlaying(int sound_id) = 0;

    virtual void SetVolume(int sound_id, float volume) = 0;

    virtual void SetPitch(int sound_id, float pitch) = 0;

    virtual void SetLooping(int sound_id, bool looping) = 0;

    /// Places a sound in the world. `velocity` is in units per second. It
    /// has no effect on a sound that was created without a place.
    virtual void SetPosition(int sound_id, const glm::vec3 &position, const glm::vec3 &velocity) = 0;

    virtual void SetListener(const ListenerInfo &listener_info) = 0;

    /// The loudness of everything. 1 leaves it as it is, 0 is silence.
    virtual void SetMasterVolume(float volume) = 0;

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
