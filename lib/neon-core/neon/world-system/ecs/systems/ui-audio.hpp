#ifndef UI_AUDIO_HPP
#define UI_AUDIO_HPP

#include <neon/audio/audio-context.hpp>
#include <neon/ui/ui-context.hpp>
#include <neon/world-system/ecs/entity-system.hpp>

namespace neon
{
  /// Lets values of the user interface set what is heard: the volume of a
  /// group of sounds through a UiVolume, and whether the sound of an entity
  /// plays through a UiSoundSwitch. So a settings menu works from its file
  /// and the scene, without code of the game.
  ///
  /// It registers both components. It is added after placing, so that it
  /// goes on while the world is paused, which is when a pause menu shows the
  /// settings, and before AudioPlayback, so that what it asks of a
  /// SoundSource is heard in the same frame.
  ///
  ///     world.AddSystemAfterPlacing(std::make_unique<neon::UiAudio>(&ui_system, &audio_system));
  ///     world.AddSystemAfterPlacing(std::make_unique<neon::AudioPlayback>(&audio_system, logger));
  class UiAudio final : public EntitySystem
  {
    UiContext *_ui_context;
    AudioContext *_audio_context;
    QueryId _volumes = 0;
    QueryId _switches = 0;

  public:
    UiAudio(UiContext *ui_context, AudioContext *audio_context);

    void Register(EntityStore &store) override;

    void Initialize(EntityStore &store) override;

    void Update(EntityStore &store, double delta_time) override;
  };
} // neon

#endif //UI_AUDIO_HPP
