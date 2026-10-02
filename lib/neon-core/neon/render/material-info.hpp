#ifndef MATERIAL_INFO_HPP
#define MATERIAL_INFO_HPP
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

    /// Whether both sides of every triangle are drawn: as the model file
    /// says unless the scene says otherwise, see DoubleSided. A renderer
    /// settles `Model` against the file before it draws.
    DoubleSided double_sided = DoubleSided::Model;
  };
} // neon

#endif //MATERIAL_INFO_HPP
