#ifndef MATERIAL_INFO_HPP
#define MATERIAL_INFO_HPP
#include <string>
#include "neon/common/color.hpp"

#include "double-sided.hpp"

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
    /// The sharpness of the highlight of the `basic-lit` shader. The `pbr`
    /// shader does not read it; `roughness` says the same there.
    float shininess{};
    Color color;
    bool use_textures = true;
    AlphaMode alpha_mode = AlphaMode::Opaque;

    /// How much of a metal the surface is, from 0 (a dielectric: plastic,
    /// wood, stone) to 1 (a metal), as glTF describes it. A metal reflects
    /// the light in its own colour and has no diffuse colour; without an
    /// environment to reflect it is dark, which is why 0 is the default here
    /// where glTF's is 1.
    float metallic = 0.0f;

    /// How rough the surface is, from 0 (a mirror) to 1 (matte), as glTF
    /// describes it. The `pbr` shader reads the green channel of the second
    /// texture for it, and the blue channel for metallic, as a glTF
    /// metallic-roughness texture is laid out; both multiply these numbers.
    float roughness = 0.5f;

    /// The light the surface gives off itself, added after lighting by the
    /// shaders that light, so that it shows in the dark. Written in sRGB,
    /// as `color` is, and black for a surface that gives off nothing.
    /// Black leaves it to the model file, whose `emissiveFactor` is taken
    /// then. Alpha is not read.
    Color emissive{.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 1.0f};

    /// What the emissive light is multiplied by, in linear light, so that
    /// a surface can be brighter than white: 4 gives four times the light
    /// of its colour. 0 turns the glow off, also the one of a model file.
    /// Multiplied with the file's `KHR_materials_emissive_strength`.
    float emissive_strength = 1.0f;

    /// Virtual path of a texture of what the surface gives off, which the
    /// emissive colour multiplies, or `surface://<name>` for a render
    /// target. Empty takes the model file's, when it has one.
    std::string emissive_texture;

    /// Whether both sides of every triangle are drawn: as the model file
    /// says unless the scene says otherwise, see DoubleSided. A renderer
    /// settles `Model` against the file before it draws.
    DoubleSided double_sided = DoubleSided::Model;
  };
} // neon

#endif //MATERIAL_INFO_HPP
