#ifndef BEZIER_HPP
#define BEZIER_HPP

#include <cstddef>
#include <span>

namespace neon
{
  /// The weight of control point `index` of a Bézier curve of the order
  /// `order` at `t`: the Bernstein polynomial, the binomial of the two
  /// times `(1 - t)^(order - index) * t^index`. The weights of one curve
  /// add up to 1 at every `t`, which is why a curve stays within the hull
  /// of its points and moves with them.
  [[nodiscard]] inline float BernsteinWeight(const std::size_t order, const std::size_t index, const float t)
  {
    // the binomial, built up so that it stays a whole number at every step
    float weight = 1.0f;
    for (std::size_t i = 0; i < index; i++)
    {
      weight = weight * static_cast<float>(order - i) / static_cast<float>(i + 1);
    }

    for (std::size_t i = 0; i < order - index; i++) { weight *= 1.0f - t; }
    for (std::size_t i = 0; i < index; i++) { weight *= t; }
    return weight;
  }

  /// The point at `t`, from 0 to 1, of the Bézier curve of any order that
  /// `points` control: two are a line, three a quadratic curve, four a
  /// cubic one, and so on. The curve starts at the first point and ends at
  /// the last, and is pulled towards the ones between. `Point` is anything
  /// that can be added and scaled: a `glm::vec2`, a `glm::vec3`, a color,
  /// a float.
  ///
  /// A cubic curve is a CubicBezier, which is faster and can be cut and
  /// measured; a curve of many points is better a BezierPath of cubic
  /// pieces, where a point moves its own piece alone.
  template<typename Point>
  [[nodiscard]] Point BezierPoint(const std::span<const Point> points, const float t)
  {
    if (points.empty()) { return Point{}; }

    const std::size_t order = points.size() - 1;
    Point point = points[0] * BernsteinWeight(order, 0, t);
    for (std::size_t i = 1; i <= order; i++) { point += points[i] * BernsteinWeight(order, i, t); }
    return point;
  }
} // neon

#endif //BEZIER_HPP
