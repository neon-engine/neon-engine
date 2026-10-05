#ifndef FRAME_LIMIT_HPP
#define FRAME_LIMIT_HPP

#include <algorithm>

namespace neon
{
  /// The most frames a second an application draws: a number from 30 to
  /// 300, or none, which lets it draw as many as it can. It holds whether a
  /// frame waits for the screen or not, and while the world is paused, when
  /// a frame costs next to nothing and the frame rate would run away.
  ///
  /// Kept apart from the window so that the numbers are checked without one.
  struct FrameLimit
  {
    /// No limit: as many frames as can be drawn.
    static constexpr int kUnlimited = 0;

    static constexpr int kLeast = 30;
    static constexpr int kMost = 300;

    /// The limit that holds for a number that was asked for: none for 0
    /// and below, and else the number, held between the least and the most.
    [[nodiscard]] static constexpr int Of(const int asked)
    {
      return asked <= 0 ? kUnlimited : std::clamp(asked, kLeast, kMost);
    }

    /// How long a frame lasts at the least under a limit, in seconds. 0
    /// without one.
    [[nodiscard]] static constexpr double SecondsOf(const int limit)
    {
      return limit <= 0 ? 0.0 : 1.0 / static_cast<double>(limit);
    }
  };
} // neon

#endif //FRAME_LIMIT_HPP
