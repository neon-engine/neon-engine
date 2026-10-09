#ifndef POLYLINE_HPP
#define POLYLINE_HPP

#include <algorithm>
#include <cstddef>
#include <vector>

#include <glm/glm.hpp>

namespace neon
{
  /// How long a line of straight pieces through `points` is.
  template<typename Point>
  [[nodiscard]] float PolylineLength(const std::vector<Point> &points)
  {
    float length = 0.0f;
    for (std::size_t i = 1; i < points.size(); i++) { length += glm::length(points[i] - points[i - 1]); }
    return length;
  }

  /// Fills `evenly` with `count` points along the line through `points`,
  /// the first and the last included, each as far along it from the one
  /// before as every other. `count` is at least two.
  ///
  /// The parameter of a curve does not move along it at an even speed, so
  /// what follows a curve at one - a particle on a trail, a camera on a
  /// track, the rings of a rope - flattens the curve first and asks here.
  /// `evenly` keeps what it had room for, so that calling it every frame
  /// takes no memory after the first.
  template<typename Point>
  void ResamplePolyline(const std::vector<Point> &points, const std::size_t count, std::vector<Point> &evenly)
  {
    evenly.clear();
    if (points.empty()) { return; }

    const std::size_t wanted = std::max<std::size_t>(count, 2);
    const float step = PolylineLength(points) / static_cast<float>(wanted - 1);

    evenly.push_back(points.front());

    // walks the line once: `walked` is how far the start of piece `piece`
    // is along it
    std::size_t piece = 1;
    float walked = 0.0f;
    for (std::size_t i = 1; i + 1 < wanted; i++)
    {
      const float target = step * static_cast<float>(i);
      while (piece + 1 < points.size() && walked + glm::length(points[piece] - points[piece - 1]) < target)
      {
        walked += glm::length(points[piece] - points[piece - 1]);
        piece++;
      }

      if (piece >= points.size())
      {
        evenly.push_back(points.back());
        continue;
      }

      const float span = glm::length(points[piece] - points[piece - 1]);
      const float along = span > 0.0f ? std::clamp((target - walked) / span, 0.0f, 1.0f) : 0.0f;
      evenly.push_back(points[piece - 1] + (points[piece] - points[piece - 1]) * along);
    }

    evenly.push_back(points.back());
  }
} // neon

#endif //POLYLINE_HPP
