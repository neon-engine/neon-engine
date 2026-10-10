#include "graphics-preset-reader.hpp"

#include <format>

#include <neon/render/anisotropy.hpp>
#include <neon/render/shadow-map-size.hpp>
#include <neon/render/target-quality.hpp>
#include <neon/render/texture-scale.hpp>

namespace neon
{
  void GraphicsPresetReader::Read(const DataReader &reader, GraphicsPreset &preset)
  {
    // how many samples a texture is read with from the side
    if (int level = 0; reader.Read(kAnisotropy, level))
    {
      if (!Anisotropy::IsLevel(level))
      {
        reader.Report(*reader.ReadValue(kAnisotropy), std::format(
                        "'{}' of {} is {}, where 1, 2, 4, 8, or 16 was expected", kAnisotropy, reader.GetWhere(), level));
      } else
      {
        preset.anisotropy = level;
      }
    }

    // the size textures read from files are kept at
    if (double scale = 0.0; reader.Read(kTextureScale, scale))
    {
      if (!TextureScale::IsScale(scale))
      {
        reader.Report(*reader.ReadValue(kTextureScale), std::format(
                        "'{}' of {} is {}, where 1, 0.5, 0.25, or 0.125 was expected", kTextureScale, reader.GetWhere(), scale));
      } else
      {
        preset.texture_scale = scale;
      }
    }

    // the quality of render targets
    if (double scale = 0.0; reader.Read(kTargetScale, scale))
    {
      if (!TargetQuality::IsScale(scale))
      {
        reader.Report(*reader.ReadValue(kTargetScale), std::format(
                        "'{}' of {} is {}, where 1, 0.5, or 0.25 was expected", kTargetScale, reader.GetWhere(), scale));
      } else
      {
        preset.target_scale = scale;
      }
    }
    if (int mipmaps = 0; reader.Read(kTargetMipmaps, mipmaps))
    {
      if (!TargetQuality::IsMipmaps(mipmaps))
      {
        reader.Report(*reader.ReadValue(kTargetMipmaps), std::format(
                        "'{}' of {} is {}, where 0 for as many as the size allows, or 1 to 16 was expected",
                        kTargetMipmaps, reader.GetWhere(), mipmaps));
      } else
      {
        preset.target_mipmaps = mipmaps;
      }
    }

    // how fine the shadow map is, and how it is compared against, see
    // docs/vulkan-renderer.md
    if (int size = 0; reader.Read(kShadowMapSize, size))
    {
      if (!ShadowMapSize::IsSize(size))
      {
        reader.Report(*reader.ReadValue(kShadowMapSize), std::format(
                        "'{}' of {} is {}, where 512, 1024, 2048, or 4096 was expected", kShadowMapSize, reader.GetWhere(), size));
      } else
      {
        preset.shadow_map_size = size;
      }
    }
    if (std::size_t filter = 0; reader.ReadChoice(kShadowFilter, {"none", "pcf"}, filter))
    {
      preset.shadow_filter = static_cast<ShadowFilter>(filter);
    }

    // how many slices the shadow map has, and how far it reaches
    if (int cascades = 0; reader.Read(kShadowCascades, cascades))
    {
      if (cascades <= 0)
      {
        reader.Report(*reader.ReadValue(kShadowCascades), std::format(
                        "'{}' of {} is {}, where a whole number above zero was expected",
                        kShadowCascades, reader.GetWhere(), cascades));
      } else if (cascades > 4)
      {
        reader.Report(*reader.ReadValue(kShadowCascades), std::format(
                        "'{}' of {} is {}, where 1 to 4 was expected", kShadowCascades, reader.GetWhere(), cascades));
      } else
      {
        preset.shadow_cascades = cascades;
      }
    }
    if (float distance = 0.0f; reader.Read(kShadowDistance, distance))
    {
      if (distance <= 0.0f)
      {
        reader.Report(*reader.ReadValue(kShadowDistance), std::format(
                        "'{}' of {} is {}, where a number above zero was expected",
                        kShadowDistance, reader.GetWhere(), distance));
      } else
      {
        preset.shadow_distance = distance;
      }
    }
  }
} // neon
