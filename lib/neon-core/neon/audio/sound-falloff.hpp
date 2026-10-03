#ifndef SOUND_FALLOFF_HPP
#define SOUND_FALLOFF_HPP

namespace neon
{
  /// How a sound with a place gets quieter between its least and its most
  /// distance from the listener.
  enum class SoundFalloff
  {
    /// As sound does in the open: half as loud at twice the distance. From
    /// the most distance on it gets no quieter, and is still heard.
    Inverse = 0,

    /// In a straight line down to silence at the most distance, and silent
    /// from there on.
    Linear
  };
} // neon

#endif //SOUND_FALLOFF_HPP
