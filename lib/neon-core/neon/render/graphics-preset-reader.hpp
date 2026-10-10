#ifndef GRAPHICS_PRESET_READER_HPP
#define GRAPHICS_PRESET_READER_HPP

#include <neon/data/data-reader.hpp>

#include "graphics-preset.hpp"

namespace neon
{
  /// Reads the values a quality preset decides from a map, on top of a
  /// GraphicsPreset: `anisotropy`, `texture_scale`, `target_scale`,
  /// `target_mipmaps`, `shadow_map_size`, `shadow_filter`,
  /// `shadow_cascades`, and `shadow_distance`, each checked as the setting
  /// of that name is. A value that is not written stays as it was, and a
  /// value that is wrong is reported with its line and leaves the one it
  /// had.
  ///
  /// It is what `rendering` of a settings file and a preset of a project's
  /// `graphics_presets` are read with, so that both take the same values
  /// and say the same about a wrong one.
  class GraphicsPresetReader final
  {
  public:
    /// The names that are read, as a file writes them.
    static constexpr const char *kAnisotropy = "anisotropy";
    static constexpr const char *kTextureScale = "texture_scale";
    static constexpr const char *kTargetScale = "target_scale";
    static constexpr const char *kTargetMipmaps = "target_mipmaps";
    static constexpr const char *kShadowMapSize = "shadow_map_size";
    static constexpr const char *kShadowFilter = "shadow_filter";
    static constexpr const char *kShadowCascades = "shadow_cascades";
    static constexpr const char *kShadowDistance = "shadow_distance";

    /// Reads the names the reader holds into `preset`. Every problem goes
    /// to the errors of the reader; the names that were asked for count as
    /// known to it, and it is not finished here.
    static void Read(const DataReader &reader, GraphicsPreset &preset);
  };
} // neon

#endif //GRAPHICS_PRESET_READER_HPP
