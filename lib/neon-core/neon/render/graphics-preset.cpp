#include "graphics-preset.hpp"

#include <neon/runtime/settings-config.hpp>

namespace neon
{
  bool GraphicsPreset::HasTheValuesOf(const GraphicsPreset &other) const
  {
    return anisotropy == other.anisotropy && texture_scale == other.texture_scale &&
           target_scale == other.target_scale && target_mipmaps == other.target_mipmaps &&
           shadow_map_size == other.shadow_map_size && shadow_filter == other.shadow_filter &&
           shadow_cascades == other.shadow_cascades && shadow_distance == other.shadow_distance;
  }

  void GraphicsPreset::ApplyTo(SettingsConfig &settings) const
  {
    settings.anisotropy = anisotropy;
    settings.texture_scale = texture_scale;
    settings.target_scale = target_scale;
    settings.target_mipmaps = target_mipmaps;
    settings.shadow_map_size = shadow_map_size;
    settings.shadow_filter = shadow_filter;
    settings.shadow_cascades = static_cast<std::size_t>(shadow_cascades);
    settings.shadow_distance = shadow_distance;
  }
} // neon
