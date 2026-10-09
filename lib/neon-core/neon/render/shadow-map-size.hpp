#ifndef SHADOW_MAP_SIZE_HPP
#define SHADOW_MAP_SIZE_HPP

#include <array>

namespace neon
{
  /// How many texels the shadow map of the direction light has along each
  /// side, every cascade. More is a sharper shadow at the same distance,
  /// and more to draw and to hold: four bytes a texel a cascade, so 4096
  /// is 64 megabytes with four cascades.
  struct ShadowMapSize
  {
    /// The sizes there are, as a settings menu offers them.
    static constexpr std::array<int, 4> kSizes = {512, 1024, 2048, 4096};

    /// The size of the map unless the settings say otherwise.
    static constexpr int kDefault = 2048;

    /// Whether `size` is one of kSizes.
    [[nodiscard]] static constexpr bool IsSize(const int size)
    {
      for (const int each : kSizes)
      {
        if (each == size) { return true; }
      }
      return false;
    }
  };
} // neon

#endif //SHADOW_MAP_SIZE_HPP
