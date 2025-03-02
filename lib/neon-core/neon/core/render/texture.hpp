#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include <string>

#include "texture-type.hpp"

namespace core
{
  struct Texture
  {
    std::string path;
    TextureType texture_type;
  };
} // core

#endif //TEXTURE_HPP
