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
    Prism,
    Sphere,
    Cylinder,
    Quad
  };
} // neon

#endif //GEOMETRY_SHAPE_HPP
