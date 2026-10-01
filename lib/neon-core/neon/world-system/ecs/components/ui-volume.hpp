#ifndef UI_VOLUME_HPP
#define UI_VOLUME_HPP

#include <optional>
#include <string>

#include <neon/reflection/type-builder.hpp>

namespace neon
{
  /// Makes a value of the user interface the volume of a group of sounds,
  /// such as a slider of a settings menu that sets how loud the music is.
  /// The value is a number from 0 to `full`, and `full` is the volume of 1.
  struct UiVolume
  {
    /// The name of the value, as files write it in `{music}`.
    std::string value;

    /// The group of sounds: music, effects, voices, or one of the game.
    std::string group;

    /// What the value is at the full volume of the group. 100 for a slider
    /// that goes from 0 to 100.
    float full = 100.0f;

    /// The volume that was last handed to the audio, so that it is handed
    /// over again only when it changes. Kept by the engine.
    std::optional<float> last_volume;
  };

  /// What the engine keeps for itself is not described.
  inline void Describe(TypeBuilder<UiVolume> &type)
  {
    type.Named("UiVolume", "Makes a value of the user interface the volume of a group of sounds");

    type.Field("value", &UiVolume::value)
        .Required()
        .Describe("The name of the value, as the user interface writes it in braces");

    type.Field("group", &UiVolume::group)
        .Required()
        .Describe("The group of sounds: music, effects, voices, or one of the game");

    type.Field("full", &UiVolume::full)
        .Above(0)
        .Describe("What the value is at the full volume of the group");
  }
} // neon

#endif //UI_VOLUME_HPP
