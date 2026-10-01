#ifndef UI_SOUND_SWITCH_HPP
#define UI_SOUND_SWITCH_HPP

#include <optional>
#include <string>

#include <neon/reflection/type-builder.hpp>

namespace neon
{
  /// Lets a value of the user interface choose whether the sound of an
  /// entity plays. It fades in while the value is `equals`, and fades out
  /// while it is anything else. With one entity for every piece of music,
  /// each with its own word, a choice in a menu fades from one to another.
  ///
  /// It goes with a SoundSource on the same entity.
  struct UiSoundSwitch
  {
    /// The name of the value, as files write it in `{track}`.
    std::string value;

    /// What the value is while the sound plays.
    std::string equals;

    /// How long fading in and out takes, in seconds.
    float fade = 1.0f;

    /// Whether the value was `equals` in the last frame, which nothing knows
    /// before the first. Kept by the engine.
    std::optional<bool> was_chosen;
  };

  /// What the engine keeps for itself is not described.
  inline void Describe(TypeBuilder<UiSoundSwitch> &type)
  {
    type.Named("UiSoundSwitch", "Lets a value of the user interface choose whether the sound of the entity plays");

    type.Field("value", &UiSoundSwitch::value)
        .Required()
        .Describe("The name of the value, as the user interface writes it in braces");

    type.Field("equals", &UiSoundSwitch::equals)
        .Required()
        .Describe("What the value is while the sound plays");

    type.Field("fade", &UiSoundSwitch::fade)
        .AtLeast(0)
        .Describe("How long fading in and out takes, in seconds");
  }
} // neon

#endif //UI_SOUND_SWITCH_HPP
