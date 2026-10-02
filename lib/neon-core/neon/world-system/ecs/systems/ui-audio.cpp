#include "ui-audio.hpp"

#include <algorithm>

#include <neon/world-system/ecs/components/sound-source.hpp>
#include <neon/world-system/ecs/components/ui-sound-switch.hpp>
#include <neon/world-system/ecs/components/ui-volume.hpp>

namespace neon
{
  UiAudio::UiAudio(UiContext *ui_context, AudioContext *audio_context)
  {
    _ui_context = ui_context;
    _audio_context = audio_context;
  }

  void UiAudio::Register(EntityStore &store)
  {
    store.Register<UiVolume>("UiVolume");
    store.Register<UiSoundSwitch>("UiSoundSwitch");
  }

  void UiAudio::Initialize(EntityStore &store)
  {
    _volumes = store.Query<UiVolume>();
    _switches = store.Query<UiSoundSwitch, SoundSource>();
  }

  void UiAudio::Update(EntityStore &store, double)
  {
    store.Each(_volumes, [&](const EntityBlock &block)
    {
      auto *volumes = block.Column<UiVolume>(0);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &volume = volumes[i];

        double number = 0.0;
        if (!_ui_context->GetNumber(volume.value, number) || volume.full <= 0.0f) { continue; }

        const float level = std::max(static_cast<float>(number) / volume.full, 0.0f);
        if (volume.last_volume == level) { continue; }

        // handed over only when it changes, so that a group that is not
        // there is reported once and not in every frame
        _audio_context->SetGroupVolume(volume.group, level);
        volume.last_volume = level;
      }
    });

    store.Each(_switches, [&](const EntityBlock &block)
    {
      auto *switches = block.Column<UiSoundSwitch>(0);
      auto *sources = block.Column<SoundSource>(1);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &choice = switches[i];
        auto &source = sources[i];

        bool is_set = false;
        const bool chosen = _ui_context->GetValue(choice.value, &is_set) == choice.equals && is_set;

        if (!choice.was_chosen.has_value())
        {
          // At first a sound that is not chosen is silent from the start,
          // and one that is fades in, as it would after a choice.
          if (chosen) { source.FadeIn(choice.fade); } else { source.playing = false; }
        } else if (chosen && !*choice.was_chosen)
        {
          source.FadeIn(choice.fade);
        } else if (!chosen && *choice.was_chosen)
        {
          source.FadeOut(choice.fade);
        }

        choice.was_chosen = chosen;
      }
    });
  }
} // neon
