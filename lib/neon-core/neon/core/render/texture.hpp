#ifndef TEXTURE_HPP
#define TEXTURE_HPP

namespace core
{
  enum class TextureType
  {
    Diffuse,
    Specular
  };

  struct Texture
  {
    int id;
    TextureType type;
  };
} // core

#endif //TEXTURE_HPP
