#ifndef MATERIAL_INFO_HPP
#define MATERIAL_INFO_HPP
#include "neon/common/color.hpp"

namespace neon
{
  /// How a surface covers what is behind it.
  enum class AlphaMode
  {
    /// It covers everything behind it, whatever its alpha.
    Opaque = 0,

    /// Its alpha says how much of what is behind it shows through. Such
    /// surfaces are drawn after the opaque ones, from the farthest to the
    /// nearest, and blended in linear light.
    Blend
  };

  struct MaterialInfo
  {
    float shininess{};
    Color color;
    bool use_textures = true;
    AlphaMode alpha_mode = AlphaMode::Opaque;
  };
} // neon

#endif //MATERIAL_INFO_HPP
