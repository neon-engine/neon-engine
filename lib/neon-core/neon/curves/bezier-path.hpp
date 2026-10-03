#ifndef BEZIER_PATH_HPP
#define BEZIER_PATH_HPP

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

#include "cubic-bezier.hpp"

namespace neon
{
  /// A curve of cubic Bézier pieces laid end to end, where the last point
  /// of one piece is the first of the next. One piece takes four points,
  /// two take seven, `n` take `3n + 1`: every third point is on the curve,
  /// and the two between are the handles that pull it.
  ///
  ///     const BezierPath<glm::vec3> track({{0, 0, 0}, {1, 0, 0}, {2, 1, 0}, {3, 1, 0}, {4, 1, 0}, {5, 0, 0}, {6, 0, 0}});
  ///     const glm::vec3 halfway = track.At(1.0f);   // where the two pieces meet
  ///
  /// A point moves its own piece alone, which a single curve of many points
  /// does not do. The pieces meet without a corner where the handles on
  /// either side of a shared point are opposite and as long, see IsSmooth().
  template<typename Point>
  class BezierPath
  {
    std::vector<Point> _points;

    /// The piece `u` falls in, and how far through it, from 0 to 1.
    [[nodiscard]] std::pair<std::size_t, float> Locate(const float u) const
    {
      const auto count = static_cast<float>(PieceCount());
      const float within = std::clamp(u, 0.0f, count);
      const auto piece = std::min(static_cast<std::size_t>(within), PieceCount() - 1);
      return {piece, within - static_cast<float>(piece)};
    }

  public:
    BezierPath() = default;

    /// From `3n + 1` points. Points beyond the last whole piece are left
    /// out, and fewer than four make an empty path.
    explicit BezierPath(std::vector<Point> points)
      : _points(std::move(points))
    {
      if (_points.size() < 4)
      {
        _points.clear();
        return;
      }
      _points.resize(_points.size() - (_points.size() - 1) % 3);
    }

    /// Whether `count` points make whole pieces: 4, 7, 10, and so on.
    [[nodiscard]] static bool FitsPieces(const std::size_t count)
    {
      return count >= 4 && (count - 1) % 3 == 0;
    }

    [[nodiscard]] bool IsEmpty() const
    {
      return _points.empty();
    }

    [[nodiscard]] std::size_t PieceCount() const
    {
      return _points.empty() ? 0 : (_points.size() - 1) / 3;
    }

    [[nodiscard]] const std::vector<Point> &Points() const
    {
      return _points;
    }

    /// The piece at `index`, counted from 0.
    [[nodiscard]] CubicBezier<Point> Piece(const std::size_t index) const
    {
      const std::size_t first = index * 3;
      return {_points[first], _points[first + 1], _points[first + 2], _points[first + 3]};
    }

    /// The point at `u`, which goes from 0 at the start to PieceCount() at
    /// the end, one for every piece: 1.5 is half way through the second.
    [[nodiscard]] Point At(const float u) const
    {
      if (_points.empty()) { return Point{}; }
      const auto [piece, t] = Locate(u);
      return Piece(piece).At(t);
    }

    /// The direction the path goes at `u`, see CubicBezier::Tangent().
    [[nodiscard]] Point Tangent(const float u) const
    {
      if (_points.empty()) { return Point{}; }
      const auto [piece, t] = Locate(u);
      return Piece(piece).Tangent(t);
    }

    /// Whether the pieces meet without a corner and without a change of
    /// speed: at every shared point, the handle before it and the handle
    /// after it mirror each other, to within `tolerance`.
    [[nodiscard]] bool IsSmooth(const float tolerance = 0.0001f) const
    {
      for (std::size_t shared = 3; shared + 1 < _points.size(); shared += 3)
      {
        const Point before = _points[shared] - _points[shared - 1];
        const Point after = _points[shared + 1] - _points[shared];
        if (glm::length(after - before) > tolerance) { return false; }
      }
      return true;
    }

    /// The path as a line of straight pieces that is nowhere further than
    /// `tolerance` from it, from its first point to its last.
    [[nodiscard]] std::vector<Point> Flatten(const float tolerance) const
    {
      std::vector<Point> points;
      if (_points.empty()) { return points; }

      points.push_back(_points.front());
      for (std::size_t piece = 0; piece < PieceCount(); piece++) { Piece(piece).Flatten(tolerance, points); }
      return points;
    }

    /// How long the path is, to within `tolerance` for every piece.
    [[nodiscard]] float Length(const float tolerance = 0.001f) const
    {
      float length = 0.0f;
      for (std::size_t piece = 0; piece < PieceCount(); piece++) { length += Piece(piece).Length(tolerance); }
      return length;
    }
  };
} // neon

#endif //BEZIER_PATH_HPP
