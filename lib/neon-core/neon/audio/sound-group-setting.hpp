#ifndef SOUND_GROUP_SETTING_HPP
#define SOUND_GROUP_SETTING_HPP

#include <string>
#include <vector>

#include "sound-info.hpp"

namespace neon
{
  /// A group of sounds as the settings know it: its name, the volume it
  /// starts at, and whether it is held still while the game is paused.
  /// The groups of the engine are there from the start, see
  /// built_in_sound_groups(); a project declares more in its settings
  /// file, under `audio.groups`, and the volumes are what a player changes.
  struct SoundGroupSetting
  {
    std::string name;

    /// 1 is the loudness of the sounds, 0 is silence.
    float volume = 1.0f;

    /// Whether the sounds of the group stop where they are while the game
    /// is paused, and go on from there when it goes on. Effects, voices,
    /// and ambience do; music plays on through a menu.
    bool pauses = true;
  };

  /// The groups every audio system has, in the order they are listed in.
  inline std::vector<SoundGroupSetting> built_in_sound_groups()
  {
    return {
      {.name = sound_group::music, .pauses = false},
      {.name = sound_group::effects},
      {.name = sound_group::voices},
      {.name = sound_group::ambience}
    };
  }
} // neon

#endif //SOUND_GROUP_SETTING_HPP
