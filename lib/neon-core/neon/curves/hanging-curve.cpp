#include "hanging-curve.hpp"

namespace neon
{
  // Helpers of HangingCurve().
  namespace
  {
    /// The curve between the two ends with both handles lowered by `sag`.
    CubicBezier<glm::vec3> Lowered(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &down, const float sag)
    {
      const glm::vec3 across = to - from;
      return {from, from + across / 3.0f + down * sag, from + across * (2.0f / 3.0f) + down * sag, to};
    }
  }

  CubicBezier<glm::vec3> HangingCurve(
    const glm::vec3 &from,
    const glm::vec3 &to,
    const float length,
    const glm::vec3 &down)
  {
    // taut, or stretched further than it is long: a straight line
    if (!(length > glm::length(to - from))) { return Lowered(from, to, down, 0.0f); }

    // The curve is longer the lower its handles are, so the sag that makes
    // it `length` long is found by halving. Handles lowered by `length`
    // take the curve three quarters of that down and back, which is longer
    // than the rope, so the answer lies between.
    const float tolerance = length * 0.0005f;
    float least = 0.0f;
    float most = length;
    for (int i = 0; i < 20; i++)
    {
      const float sag = (least + most) * 0.5f;
      if (Lowered(from, to, down, sag).Length(tolerance) < length)
      {
        least = sag;
      } else
      {
        most = sag;
      }
    }

    return Lowered(from, to, down, (least + most) * 0.5f);
  }
} // neon
