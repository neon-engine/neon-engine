#include "audio-playback.hpp"

#include <neon/common/transform.hpp>
#include <neon/world-system/ecs/components/sound-listener.hpp>
#include <neon/world-system/ecs/components/sound-source.hpp>

namespace neon
{
  // Helpers of AudioPlayback, for this file alone.
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

  AudioPlayback::AudioPlayback(AudioContext *audio_context, std::shared_ptr<Logger> logger)
    : _audio_context(audio_context), _logger(std::move(logger))
  {
  }

  void AudioPlayback::Register(EntityStore &store)
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
  }

  void AudioPlayback::Initialize(EntityStore &store)
  {
    _listeners = store.Query<Transform, SoundListener>();

    // a source alone, as music and the sounds of a menu have no place. The
    // place of a spatial one is read when it is needed
    _sources = store.Query<SoundSource>();
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
      auto *sources = block.Column<SoundSource>(0);

      for (std::size_t i = 0; i < block.count; i++)
      {
        const Entity entity = block.entities[i];
        auto &source = sources[i];

        const Transform *transform = source.sound.spatial ? store.Get<Transform>(entity) : nullptr;
        if (source.sound.spatial && transform == nullptr)
        {
          // said once, and not in every frame. It is heard once it has a place
          if (_placeless.insert(entity).second)
          {
            const auto name = store.GetName(entity);
            _logger->Warn(
              "The SoundSource of entity '{}' is spatial, but the entity has no Transform to be heard "
              "from, so it is not heard. Give the entity a Transform, or make the sound not spatial",
              name);
          }
          continue;
        }

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

        if (transform != nullptr)
        {
          const glm::vec3 position = transform->world_coordinates[3];
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
