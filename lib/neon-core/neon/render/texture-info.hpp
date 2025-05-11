#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <string>

#include "texture-type.hpp"

namespace neon
{
  struct TextureInfo
  {
    std::string path;
    TextureType texture_type;
  };
} // neon

#endif //TEXTURE_HPP
