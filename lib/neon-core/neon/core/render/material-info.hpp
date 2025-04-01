#ifndef MATERIAL_INFO_HPP
#define MATERIAL_INFO_HPP
#include "neon/core/common/color.hpp"

namespace neon
{
  struct MaterialInfo
  {
    float shininess{};
    Color color;
    bool use_textures = true;
  };
} // neon

#endif //MATERIAL_INFO_HPP
