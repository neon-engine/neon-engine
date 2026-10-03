#ifndef SKY_TYPE_HPP
#define SKY_TYPE_HPP

namespace neon
{
  /// What a sky is made of.
  enum class SkyType
  {
    /// Six images, the faces of a cube seen from its middle.
    Box,

    /// One panorama, wrapped around a sphere seen from its middle.
    Sphere
  };
} // neon

#endif //SKY_TYPE_HPP
