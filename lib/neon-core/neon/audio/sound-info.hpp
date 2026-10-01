#ifndef SOUND_INFO_HPP
#define SOUND_INFO_HPP

#include <string>

#include <glm/glm.hpp>

namespace neon
{
  /// The groups every audio system has. Each has a volume of its own, which
  /// is what a settings menu changes. A game adds more with
  /// AudioContext::AddGroup().
  namespace sound_group
  {
    constexpr auto music = "music";
    constexpr auto effects = "effects";
    constexpr auto voices = "voices";
  }

  /// What a sound is created from.
  struct SoundInfo
  {
    /// Virtual path of the file, such as `assets://sounds/step.wav`.
    std::string path;

    /// Whether the sound starts over when it has ended.
    bool looping = false;

    /// 1 is the loudness of the file. 0 is silence.
    float volume = 1.0f;

    /// 1 is the speed of the file. 2 is twice as fast and an octave higher.
    float pitch = 1.0f;

    /// Whether the sound has a place in the world. It is then heard from
    /// where the listener is: quieter from further away, and from the side
    /// it is on. A sound without a place is heard the same everywhere, which
    /// is what music and the sounds of a menu want.
    bool spatial = false;

    /// Up to this distance from the listener, a sound with a place is as
    /// loud as it gets.
    float min_distance = 1.0f;

    /// From this distance on, it does not get any quieter.
    float max_distance = 100.0f;

    /// The group the sound belongs to, whose volume it is played at on top
    /// of its own.
    std::string group = sound_group::effects;
  };

  /// Where the world is heard from.
  struct ListenerInfo
  {
    glm::vec3 position{0.0f};
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};

    /// Units per second. It shifts the pitch of what moves towards the
    /// listener or away from it.
    glm::vec3 velocity{0.0f};
  };
} // neon

#endif //SOUND_INFO_HPP
