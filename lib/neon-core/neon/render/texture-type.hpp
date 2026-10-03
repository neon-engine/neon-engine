#ifndef TEXTURE_TYPE_HPP
#define TEXTURE_TYPE_HPP

namespace neon
{
  enum class TextureType
  {
    Diffuse,
    Specular,

    /// What a surface gives off itself, `emissiveTexture` of glTF.
    Emissive
  };
} // neon

#endif //TEXTURE_TYPE_HPP
