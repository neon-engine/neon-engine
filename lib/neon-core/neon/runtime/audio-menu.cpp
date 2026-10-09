#include "audio-menu.hpp"

#include <algorithm>
#include <cmath>

namespace neon
{
  // Helpers of AudioMenu: from a value of the menu to a volume, and back.
  namespace
  {
    /// The volume a value stands for: 1 at the full value, and 0 below 0.
    float VolumeOf(const double value)
    {
      return static_cast<float>(std::max(value / AudioMenu::kFull, 0.0));
    }

    /// The value a volume is shown as, to a hundredth, so that a volume
    /// such as 0.6 is not shown as 60.0000023.
    double ValueOf(const float volume)
    {
      return std::round(static_cast<double>(volume) * AudioMenu::kFull * 100.0) / 100.0;
    }
  }

  AudioMenu::AudioMenu(
    UiContext *ui,
    AudioContext *audio,
    const std::vector<SoundGroupSetting> &groups,
    PlayerSettings *player,
    const std::shared_ptr<Logger> &logger)
  {
    _ui = ui;
    _audio = audio;
    _player = player;
    _logger = logger;

    for (const auto &group : groups) { _volumes.push_back({.group = group.name}); }
  }

  void AudioMenu::Open()
  {
    for (auto &volume : _volumes)
    {
      volume.opened = _audio->GetGroupVolume(volume.group);
      volume.shown = ValueOf(volume.opened);
      volume.applied = volume.shown;
      _ui->SetNumber(volume.group, volume.shown);
    }
    _is_open = true;
  }

  void AudioMenu::Update()
  {
    if (!_is_open) { return; }

    for (auto &volume : _volumes)
    {
      double value = 0.0;
      if (!_ui->GetNumber(volume.group, value) || value == volume.applied) { continue; }

      const float level = VolumeOf(value);
      _audio->SetGroupVolume(volume.group, level);
      _logger->Info("The menu set the volume of {} to {}", volume.group, level);
      volume.applied = value;
    }
  }

  void AudioMenu::Close(const bool keep)
  {
    if (!_is_open) { return; }

    Update();
    _is_open = false;

    for (const auto &volume : _volumes)
    {
      if (volume.applied == volume.shown) { continue; }

      if (!keep)
      {
        _audio->SetGroupVolume(volume.group, volume.opened);
      } else if (_player != nullptr)
      {
        // as a number from 0 to 1, as the settings write a volume
        _player->Set("audio", "volumes", volume.group, DataValue::Number(VolumeOf(volume.applied)));
      }
    }

    if (keep && _player != nullptr) { (void) _player->Write(); }
  }

  std::vector<std::string> AudioMenu::GetValueNames() const
  {
    std::vector<std::string> names;
    for (const auto &volume : _volumes) { names.push_back(volume.group); }
    return names;
  }
} // neon
