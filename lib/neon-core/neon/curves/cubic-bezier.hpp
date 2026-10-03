#ifndef CUBIC_BEZIER_HPP
#define CUBIC_BEZIER_HPP

#include <algorithm>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

namespace neon
{
  /// A cubic Bézier curve: it starts at `p0`, ends at `p3`, leaves `p0`
  /// towards `p1` and arrives at `p3` from `p2`. `Point` is anything that
  /// can be added, scaled, and measured with `glm::length`: a `glm::vec2`
  /// on a screen, a `glm::vec3` in the world, a float for a value that
  /// eases over time.
  ///
  ///     const CubicBezier<glm::vec3> arc{{0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0}};
  ///     const glm::vec3 top = arc.At(0.5f);
  ///     const glm::vec3 heading = arc.Tangent(0.5f);
  ///
  /// It is what a path for a camera or a particle is made of, and what a
  /// rope hangs as, see docs/curves.md.
  template<typename Point>
  struct CubicBezier
  {
    Point p0{};
    Point p1{};
    Point p2{};
    Point p3{};

    /// The point at `t`, from 0 at `p0` to 1 at `p3`. `t` moves along the
    /// curve faster where the control points are far apart, so even steps
    /// of `t` are not even steps of distance, see ResamplePolyline().
    [[nodiscard]] Point At(const float t) const
    {
      const float s = 1.0f - t;
      return p0 * (s * s * s) + p1 * (3.0f * s * s * t) + p2 * (3.0f * s * t * t) + p3 * (t * t * t);
    }

    /// The derivative at `t`: the direction the curve goes there, as long
    /// as the speed `t` moves along it at. Not of length 1.
    [[nodiscard]] Point Tangent(const float t) const
    {
      const float s = 1.0f - t;
      return (p1 - p0) * (3.0f * s * s) + (p2 - p1) * (6.0f * s * t) + (p3 - p2) * (3.0f * t * t);
    }

    /// The curve cut in two at `t`, by de Casteljau's construction: the
    /// first is the curve from 0 to `t`, the second from `t` to 1, and
    /// together they are the same curve.
    [[nodiscard]] std::pair<CubicBezier, CubicBezier> Split(const float t) const
    {
      const Point q0 = p0 + (p1 - p0) * t;
      const Point q1 = p1 + (p2 - p1) * t;
      const Point q2 = p2 + (p3 - p2) * t;
      const Point r0 = q0 + (q1 - q0) * t;
      const Point r1 = q1 + (q2 - q1) * t;
      const Point x = r0 + (r1 - r0) * t;
      return {CubicBezier{p0, q0, r0, x}, CubicBezier{x, r1, q2, p3}};
    }

    /// How far the curve is from the straight line between its ends, at
    /// most. 0 for a curve that is that line, walked evenly.
    [[nodiscard]] float Flatness() const
    {
      // how far each control point is from where it would sit on the line
      const float first = glm::length(p1 * 3.0f - p0 * 2.0f - p3);
      const float second = glm::length(p2 * 3.0f - p3 * 2.0f - p0);
      return std::max(first, second) * 0.25f;
    }

    /// Adds the curve to `points` as a line of straight pieces that is
    /// nowhere further than `tolerance` from it: few where it is nearly
    /// straight, more where it bends. `p0` is left out, so that a path adds
    /// one piece after another, and `p3` is the last point added.
    void Flatten(const float tolerance, std::vector<Point> &points) const
    {
      FlattenPiece(*this, tolerance, 0, points);
    }

    /// How long the curve is, to within `tolerance`.
    [[nodiscard]] float Length(const float tolerance = 0.001f) const
    {
      return LengthOfPiece(*this, tolerance, 0);
    }

  private:
    /// A curve is cut in half this often at most, which is 65536 pieces.
    static constexpr int most_cuts = 16;

    static void FlattenPiece(const CubicBezier &piece, const float tolerance, const int cuts, std::vector<Point> &points)
    {
      if (cuts >= most_cuts || piece.Flatness() <= tolerance)
      {
        points.push_back(piece.p3);
        return;
      }

      const auto [first, second] = piece.Split(0.5f);
      FlattenPiece(first, tolerance, cuts + 1, points);
      FlattenPiece(second, tolerance, cuts + 1, points);
    }

    static float LengthOfPiece(const CubicBezier &piece, const float tolerance, const int cuts)
    {
      // the curve is longer than the line between its ends and shorter
      // than the line through its control points, and close to both once
      // the two are close to each other
      const float chord = glm::length(piece.p3 - piece.p0);
      const float hull = glm::length(piece.p1 - piece.p0)
                         + glm::length(piece.p2 - piece.p1)
                         + glm::length(piece.p3 - piece.p2);
      if (cuts >= most_cuts || hull - chord <= tolerance) { return (2.0f * chord + hull) / 3.0f; }

      const auto [first, second] = piece.Split(0.5f);
      return LengthOfPiece(first, tolerance * 0.5f, cuts + 1) + LengthOfPiece(second, tolerance * 0.5f, cuts + 1);
    }
  };
} // neon

#endif //CUBIC_BEZIER_HPP
