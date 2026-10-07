#ifndef ANISOTROPY_HPP
#define ANISOTROPY_HPP

#include <array>

namespace neon
{
  /// How many samples a texture is read with where its surface is seen
  /// from the side: anisotropic filtering. More keeps a floor or a road
  /// sharp into the distance, at the cost of reading the texture more.
  /// 1 is none: a surface seen from the side blurs.
  ///
  /// A renderer holds it to what the graphics card allows.
  struct Anisotropy
  {
    /// The levels there are, as a settings menu offers them.
    static constexpr std::array<int, 5> kLevels = {1, 2, 4, 8, 16};

    /// What a texture is read with unless the settings say otherwise.
    static constexpr int kDefault = 8;

    /// Whether `level` is one of kLevels.
    [[nodiscard]] static constexpr bool IsLevel(const int level)
    {
      for (const int each : kLevels)
      {
        if (each == level) { return true; }
      }
      return false;
    }
  };
} // neon

#endif //ANISOTROPY_HPP
