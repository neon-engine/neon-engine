#ifndef VK_EFFECT_KIND_HPP
#define VK_EFFECT_KIND_HPP

namespace neon
{
  /// Which of the two lists of a camera an effect is in, which says what
  /// the picture it reads and writes holds.
  // ReSharper disable once CppInconsistentNaming
  enum class VK_EffectKind
  {
    /// `effects`: the light of the scene, before the tonemapper.
    Light,

    /// `screen_effects`: the colours a screen is given, after the
    /// tonemapper.
    Screen,
  };
} // neon

#endif //VK_EFFECT_KIND_HPP
