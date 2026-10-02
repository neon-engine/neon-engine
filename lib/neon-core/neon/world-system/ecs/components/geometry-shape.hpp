#ifndef GEOMETRY_SHAPE_HPP
#define GEOMETRY_SHAPE_HPP

namespace neon
{
  /// What a Geometry component builds.
  enum class GeometryShape
  {
    Box = 0,
    Plane,
    Ramp,
    Prism
  };
} // neon

#endif //GEOMETRY_SHAPE_HPP
