#ifndef TARGET_QUALITY_HPP
#define TARGET_QUALITY_HPP

#include <algorithm>
#include <array>

namespace neon
{
  /// The quality of render targets a game sets: the scale of the size a
  /// camera's target is made at, and the most levels of smaller copies a
  /// target has, which are made again in every frame it is drawn into.
  struct TargetQuality
  {
    /// The scales there are, as a settings menu offers them.
    static constexpr std::array<double, 3> kScales = {1.0, 0.5, 0.25};

    /// No side of a target that is scaled goes below this many pixels.
    static constexpr int kFloor = 16;

    /// The most levels a setting or a camera may ask for.
    static constexpr int kMost_Mipmaps = 16;

    /// Whether `scale` is one of kScales.
    [[nodiscard]] static constexpr bool IsScale(const double scale)
    {
      return std::ranges::find(kScales, scale) != kScales.end();
    }

    /// Whether `mipmaps` is a number of levels to ask for: 0 for as many as
    /// the size allows, or 1 (none) to kMost_Mipmaps.
    [[nodiscard]] static constexpr bool IsMipmaps(const int mipmaps)
    {
      return mipmaps >= 0 && mipmaps <= kMost_Mipmaps;
    }

    /// A side of `size` pixels at `scale`, never below kFloor unless it was
    /// smaller already.
    [[nodiscard]] static constexpr int SizeAt(const int size, const double scale)
    {
      const auto scaled = static_cast<int>(static_cast<double>(size) * scale);
      return std::max(scaled, std::min(size, kFloor));
    }

    /// The levels a target of `levels` that its size allows has, where one
    /// asks for at most `asked` and another for at most `capped`, either 0
    /// for no limit.
    [[nodiscard]] static constexpr int LevelsOf(const int levels, const int asked, const int capped)
    {
      int most = levels;
      if (asked > 0) { most = std::min(most, asked); }
      if (capped > 0) { most = std::min(most, capped); }
      return std::max(most, 1);
    }
  };
} // neon

#endif //TARGET_QUALITY_HPP
