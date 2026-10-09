#ifndef MODEL_MATERIAL_HPP
#define MODEL_MATERIAL_HPP

#include <optional>
#include <vector>
#include <neon/common/color.hpp>

#include "texture-info.hpp"

namespace neon
{
  /// What a model file says about the look of one of its materials, as far
  /// as the renderer can use it: the textures it names, the color it
  /// multiplies them with, and what the surface is made of. A scene fills
  /// in what it does not say.
  struct ModelMaterial
  {
    /// Diffuse first, then specular, as a material of the renderer lists
    /// them.
    std::vector<TextureInfo> textures;

    /// The base color factor of a glTF material, white when there is none.
    /// A renderer multiplies it into the color of the material.
    Color color;

    /// What the material gives off itself: the `emissiveFactor` of a glTF
    /// material, black when there is none, the strength of
    /// `KHR_materials_emissive_strength`, 1 when there is none, and the
    /// `emissiveTexture`, which the factor multiplies. A renderer shows
    /// them when the scene writes no emissive of its own.
    Color emissive{.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f};
    float emissive_strength = 1.0f;
    std::optional<TextureInfo> emissive_texture;

    /// The `metallicFactor` and `roughnessFactor` of a glTF material, or
    /// the `Pm` and `Pr` of an .obj, none when the file says nothing. glTF
    /// always has them, 1 when the file leaves them out, as the standard
    /// says. A renderer multiplies the scene's numbers into them, see
    /// MaterialInfo::MetallicWith().
    std::optional<float> metallic;
    std::optional<float> roughness;

    /// Whether the file draws both sides of every triangle, `doubleSided`
    /// of a glTF material. Off when the file says nothing. A scene whose
    /// `double_sided` is `model` takes it.
    bool double_sided = false;
  };
} // neon

#endif //MODEL_MATERIAL_HPP
