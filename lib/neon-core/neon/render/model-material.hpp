#ifndef MODEL_MATERIAL_HPP
#define MODEL_MATERIAL_HPP

#include <vector>
#include <neon/common/color.hpp>

#include "texture-info.hpp"

namespace neon
{
  /// What a model file says about the look of one of its materials, as far
  /// as the renderer can use it: the textures it names and the colour it
  /// multiplies them with. A scene fills in what it does not say.
  struct ModelMaterial
  {
    /// Diffuse first, then specular, as a material of the renderer lists
    /// them.
    std::vector<TextureInfo> textures;

    /// The base colour factor of a glTF material, white when there is none.
    /// A renderer multiplies it into the colour of the material.
    Color color;
  };
} // neon

#endif //MODEL_MATERIAL_HPP
