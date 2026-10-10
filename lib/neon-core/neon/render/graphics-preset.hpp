#ifndef GRAPHICS_PRESET_HPP
#define GRAPHICS_PRESET_HPP

#include <string>
#include <string_view>

#include <neon/render/anisotropy.hpp>
#include <neon/render/shadow-filter.hpp>
#include <neon/render/shadow-map-size.hpp>

struct SettingsConfig;

namespace neon
{
  /// A quality preset of the graphics: one name that sets every graphics
  /// setting that costs frame time at once. `rendering.quality` of the
  /// settings names one, `--quality` on the command line does, and so does
  /// the Quality row of the settings menu; the individual settings are read
  /// after it, so a file may name a preset and then change one of its
  /// values. The settings that are a matter of taste rather than cost, the
  /// tonemapper and the exposure, vertical sync, the frame limit, and the
  /// window, are not part of one.
  ///
  /// Which presets there are is the table GraphicsPresets: the engine
  /// brings `low`, `medium`, `high`, and `ultra`, and a project changes,
  /// adds, and drops presets in its `project.yml`. This is one row of it.
  ///
  /// `custom` is the name of no preset: it is what the menu shows when the
  /// values match none, and what a file writes to change nothing.
  ///
  /// Whether shadows are drawn at all, `rendering.shadows`, is not part of a
  /// preset: off changes what is seen, not how finely, and is the player's
  /// own choice, as vertical sync is.
  ///
  /// The values a preset starts with are the defaults of the engine, which
  /// are those of `high`, see SettingsConfig: a preset a project writes
  /// takes them for every value it leaves out. See docs/settings.md and
  /// docs/projects.md.
  struct GraphicsPreset
  {
    std::string name;

    int anisotropy = Anisotropy::kDefault;
    double texture_scale = 1.0;
    double target_scale = 1.0;
    int target_mipmaps = 0;
    int shadow_map_size = ShadowMapSize::kDefault;
    ShadowFilter shadow_filter = ShadowFilter::Pcf;
    int shadow_cascades = 4;
    double shadow_distance = 120.0;

    /// The name of no preset, which changes nothing.
    static constexpr std::string_view kCustom = "custom";

    /// Whether `other` holds the same values, whatever the two are named.
    [[nodiscard]] bool HasTheValuesOf(const GraphicsPreset &other) const;

    /// Sets the settings this preset decides, and leaves the rest as they
    /// are.
    void ApplyTo(SettingsConfig &settings) const;
  };
} // neon

#endif //GRAPHICS_PRESET_HPP
