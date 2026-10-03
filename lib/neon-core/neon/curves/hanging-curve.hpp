#ifndef HANGING_CURVE_HPP
#define HANGING_CURVE_HPP

#include <glm/glm.hpp>

#include "cubic-bezier.hpp"

namespace neon
{
  /// The curve a rope of `length` hangs as between `from` and `to`: a
  /// straight line when the two are as far apart as the rope is long, or
  /// further, and otherwise a curve that sags towards `down` by as much as
  /// makes it `length` long. `down` is a direction of length 1.
  ///
  /// It is a cubic Bézier curve with both handles lowered, which is close
  /// to what a real rope hangs as and costs a few dozen sums. It is where
  /// a rope comes to rest: it does not swing or whip, see docs/curves.md.
  [[nodiscard]] CubicBezier<glm::vec3> HangingCurve(
    const glm::vec3 &from,
    const glm::vec3 &to,
    float length,
    const glm::vec3 &down = {0.0f, -1.0f, 0.0f});
} // neon

#endif //HANGING_CURVE_HPP
