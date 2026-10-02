#ifndef AUDIO_PLAYBACK_HPP
#define AUDIO_PLAYBACK_HPP

#include <neon/audio/audio-context.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Plays the sounds of the world. It creates a sound for every entity that
  /// carries a SoundSource, starts and stops it as the component says, and
  /// keeps it where the entity is. It hands on the fades that a game asks a
  /// source for. What is heard is heard from the entity that carries a
  /// SoundListener.
  ///
  /// It registers both components, so that a world without it knows neither.
  class AudioPlayback final : public EntitySystem
  {
    AudioContext *_audio_context;
    QueryId _listeners = 0;
    QueryId _sources = 0;

  public:
    /// A sound that could not be created is not tried again.
    // ReSharper disable once CppInconsistentNaming
    static constexpr int failed_sound = -2;

    explicit AudioPlayback(AudioContext *audio_context);

    void Register(EntityStore &store) override;

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //AUDIO_PLAYBACK_HPP
