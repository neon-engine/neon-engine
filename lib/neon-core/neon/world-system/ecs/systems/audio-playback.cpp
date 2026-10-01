#include "audio-playback.hpp"

#include <neon/common/transform.hpp>
#include <neon/world-system/ecs/components/sound-listener.hpp>
#include <neon/world-system/ecs/components/sound-source.hpp>

namespace neon
{
  namespace
  {
    /// How fast something moves, from where it was a frame ago.
    glm::vec3 Velocity(
      const glm::vec3 &position,
      glm::vec3 &last_position,
      bool &has_last_position,
      const double delta_time)
    {
      auto velocity = glm::vec3{0.0f};
      if (has_last_position && delta_time > 0.0)
      {
        velocity = (position - last_position) / static_cast<float>(delta_time);
      }

      last_position = position;
      has_last_position = true;
      return velocity;
    }
  }

  AudioPlayback::AudioPlayback(AudioContext *audio_context)
  {
    _audio_context = audio_context;
  }

  void AudioPlayback::Initialize(EntityStore &store)
  {
    store.Register<SoundListener>("SoundListener");

    // the audio system holds a sound for every source, which is released
    // when the entity stops being one
    store.Register<SoundSource>("SoundSource", [this](Entity, SoundSource &source)
    {
      if (source.sound_id < 0) { return; }

      _audio_context->DestroySound(source.sound_id);
      source.sound_id = -1;
      source.was_playing = false;
    });

    _listeners = store.Query<Transform, SoundListener>();
    _sources = store.Query<Transform, SoundSource>();
  }

  void AudioPlayback::Update(EntityStore &store, const double delta_time)
  {
    store.Each(_listeners, [&](const EntityBlock &block)
    {
      const auto *transforms = block.Column<Transform>(0);
      auto *listeners = block.Column<SoundListener>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        const auto &transform = transforms[i];
        auto &listener = listeners[i];

        const glm::vec3 position = transform.world_coordinates[3];

        _audio_context->SetListener({
          .position = position,
          .forward = transform.Forward(),
          .up = transform.Up(),
          .velocity = Velocity(position, listener.last_position, listener.has_last_position, delta_time)
        });
        _audio_context->SetMasterVolume(listener.volume);
      }
    });

    store.Each(_sources, [&](const EntityBlock &block)
    {
      const auto *transforms = block.Column<Transform>(0);
      auto *sources = block.Column<SoundSource>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &source = sources[i];

        if (source.sound_id == failed_sound)
        {
          source.fade.reset();
          continue;
        }

        if (source.sound_id < 0)
        {
          source.sound_id = _audio_context->CreateSound(source.sound);
          if (source.sound_id < 0)
          {
            // the audio system has said why. Asking again every frame would
            // read the file and say it again every frame
            source.sound_id = failed_sound;
            source.playing = false;
            source.fade.reset();
            continue;
          }
        }

        const auto id = source.sound_id;

        _audio_context->SetVolume(id, source.sound.volume);
        _audio_context->SetPitch(id, source.sound.pitch);
        _audio_context->SetLooping(id, source.sound.looping);

        if (source.sound.spatial)
        {
          const glm::vec3 position = transforms[i].world_coordinates[3];
          _audio_context->SetPosition(
            id,
            position,
            Velocity(position, source.last_position, source.has_last_position, delta_time));
        }

        // a fade in plays the sound as Play() would, from silence
        if (source.fade.has_value() && source.fade->kind == SoundFade::Kind::In && source.playing)
        {
          _audio_context->FadeIn(id, source.fade->seconds);
          source.fade.reset();
          source.was_playing = true;
        }

        if (source.playing && !source.was_playing)
        {
          _audio_context->Play(id);
        } else if (!source.playing && source.was_playing)
        {
          _audio_context->Stop(id);
        } else if (source.playing && !_audio_context->IsPlaying(id))
        {
          // it has ended, or faded out
          source.playing = false;
        }

        // after the sound was played, which starts it at the volume it has
        if (source.fade.has_value() && source.playing)
        {
          if (source.fade->kind == SoundFade::Kind::Out)
          {
            _audio_context->FadeOut(id, source.fade->seconds);
          } else
          {
            _audio_context->FadeTo(id, source.fade->volume, source.fade->seconds);
          }
        }
        source.fade.reset();

        source.was_playing = source.playing;
      }
    });

    _audio_context->Advance(delta_time);
  }
} // neon
