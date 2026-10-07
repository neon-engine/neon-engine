#ifndef TEXTURE_SCALE_HPP
#define TEXTURE_SCALE_HPP

#include <algorithm>
#include <array>

namespace neon
{
  /// The size textures read from files are kept at: a quality a game sets,
  /// so that a computer with little memory on its graphics card can still
  /// play. Every step halves both sides, which is one level of the smaller
  /// copies a texture has anyway: at 1/4, the two largest levels are left
  /// out, and the rest are the same.
  ///
  /// Images of the user interface, fonts, render targets, and images made
  /// while the game runs are kept at their size.
  struct TextureScale
  {
    /// The scales there are, as a settings menu offers them.
    static constexpr std::array<double, 4> kScales = {1.0, 0.5, 0.25, 0.125};

    /// No side of a texture is halved below this many pixels, so that a
    /// small texture keeps its detail.
    static constexpr int kFloor = 32;

    /// Whether `scale` is one of kScales.
    [[nodiscard]] static constexpr bool IsScale(const double scale)
    {
      return std::ranges::find(kScales, scale) != kScales.end();
    }

    /// How many times an image of `width` by `height` is halved at
    /// `scale`: once for every halving the scale asks for, while its
    /// smaller side stays at kFloor or above.
    [[nodiscard]] static constexpr int HalvingsOf(const int width, const int height, const double scale)
    {
      int asked = 0;
      for (double left = scale; left < 1.0 && asked < 16; left *= 2.0) { asked++; }

      int halvings = 0;
      int smaller = std::min(width, height);
      while (halvings < asked && smaller / 2 >= kFloor)
      {
        smaller /= 2;
        halvings++;
      }
      return halvings;
    }
  };
} // neon

#endif //TEXTURE_SCALE_HPP
