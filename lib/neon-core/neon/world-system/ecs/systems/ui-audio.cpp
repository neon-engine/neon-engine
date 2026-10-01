#include "ui-audio.hpp"

#include <algorithm>
#include <cstdlib>

#include <neon/world-system/ecs/components/sound-source.hpp>
#include <neon/world-system/ecs/components/ui-sound-switch.hpp>
#include <neon/world-system/ecs/components/ui-volume.hpp>

namespace neon
{
  namespace
  {
    /// The number a value holds, or nothing when it holds none.
    std::optional<double> NumberOf(const std::string &text)
    {
      if (text.empty()) { return std::nullopt; }

      char *end = nullptr;
      const double number = std::strtod(text.c_str(), &end);
      if (end != text.c_str() + text.size()) { return std::nullopt; }
      return number;
    }
  }

  UiAudio::UiAudio(UiContext *ui_context, AudioContext *audio_context)
  {
    _ui_context = ui_context;
    _audio_context = audio_context;
  }

  void UiAudio::Initialize(EntityStore &store)
  {
    store.Register<UiVolume>("UiVolume");
    store.Register<UiSoundSwitch>("UiSoundSwitch");

    _volumes = store.Query<UiVolume>();

    // SoundSource is registered by AudioPlayback, which is initialized after
    // this system, so the query that needs it is made in the first frame
  }

  void UiAudio::Update(EntityStore &store, double)
  {
    store.Each(_volumes, [&](const EntityBlock &block)
    {
      auto *volumes = block.Column<UiVolume>(0);

      for (std::size_t i = 0; i < block.count; i++)
      {
        auto &volume = volumes[i];

        bool is_set = false;
        const auto number = NumberOf(_ui_context->GetValue(volume.value, &is_set));
        if (!is_set || !number || volume.full <= 0.0f) { continue; }

        const float level = std::max(static_cast<float>(*number) / volume.full, 0.0f);
        if (volume.last_volume == level) { continue; }

        // handed over only when it changes, so that a group that is not
        // there is reported once and not in every frame
        _audio_context->SetGroupVolume(volume.group, level);
        volume.last_volume = level;
      }
    });

    if (_switches == 0) { _switches = store.Query<UiSoundSwitch, SoundSource>(); }

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
