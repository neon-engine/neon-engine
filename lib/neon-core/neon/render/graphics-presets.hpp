#ifndef GRAPHICS_PRESETS_HPP
#define GRAPHICS_PRESETS_HPP

#include <string>
#include <string_view>
#include <vector>

#include "graphics-preset.hpp"

struct SettingsConfig;

namespace neon
{
  /// The table of the quality presets, GraphicsPreset each, from the
  /// cheapest to the finest: which names `rendering.quality`, `--quality`,
  /// and the Quality row of the settings menu take.
  ///
  /// The engine brings four, and a project changes them in its
  /// `project.yml` under `graphics_presets`, see ProjectFile: a built-in
  /// name is changed value by value, a new name is added after the
  /// built-in ones with the engine's defaults for what it leaves out, and
  /// `off` drops a preset from the table and the menu. `custom` is the name
  /// of no preset and cannot be defined.
  ///
  /// | Preset | Anisotropy | Texture size | Camera pictures | Camera detail | Shadow map | Shadow edges | Shadow cascades | Shadow distance |
  /// |---|---|---|---|---|---|---|---|---|
  /// | `low` | 2 | 0.5 | 0.5 | 1 | 1024 | none | 1 | 40 m |
  /// | `medium` | 4 | 1 | 1 | 4 | 2048 | pcf | 2 | 80 m |
  /// | `high` | 8 | 1 | 1 | 0 | 2048 | pcf | 4 | 120 m |
  /// | `ultra` | 16 | 1 | 1 | 0 | 4096 | pcf | 4 | 160 m |
  ///
  /// The defaults of the engine are `high`, see SettingsConfig. The table
  /// a runtime works with is `SettingsConfig::graphics_presets`, read from
  /// the project before the settings are. See docs/settings.md and
  /// docs/projects.md.
  class GraphicsPresets final
  {
    std::vector<GraphicsPreset> _presets;

  public:
    /// The name of no preset, which changes nothing.
    static constexpr std::string_view kCustom = GraphicsPreset::kCustom;

    /// The built-in preset the defaults of the engine match.
    static constexpr std::string_view kDefault = "high";

    /// The table of the engine: the four built-in presets.
    GraphicsPresets();

    /// The built-in presets, as the engine brings them, from the cheapest
    /// to the finest.
    [[nodiscard]] static const std::vector<GraphicsPreset> &BuiltIn();

    /// Every preset of this table, in the order a menu offers them.
    [[nodiscard]] const std::vector<GraphicsPreset> &All() const;

    /// The preset of `name`, or null for `custom` and a name that is none.
    [[nodiscard]] const GraphicsPreset *Named(std::string_view name) const;

    /// Whether `name` is a preset of this table or `custom`.
    [[nodiscard]] bool IsName(std::string_view name) const;

    /// The names a setting takes, the presets and then `custom`, as a
    /// settings menu offers them.
    [[nodiscard]] std::vector<std::string> Names() const;

    /// The names a setting takes, for a message: "low, medium, high,
    /// ultra, or custom".
    [[nodiscard]] std::string NamesForAMessage() const;

    /// The name of the preset whose values these are, or `custom` when they
    /// are none's. The name of `values` is not looked at.
    [[nodiscard]] std::string_view NameOf(const GraphicsPreset &values) const;

    /// The values of the settings as a preset without a name, to compare.
    [[nodiscard]] static GraphicsPreset ValuesOf(const SettingsConfig &settings);

    /// Puts a preset into the table: in place of the one of its name, which
    /// keeps its place, or after every other for a new name. Returns false
    /// and changes nothing for `custom` and an empty name.
    bool Set(const GraphicsPreset &preset);

    /// Takes the preset of `name` out of the table. Returns false when
    /// there is none.
    bool Remove(std::string_view name);
  };
} // neon

#endif //GRAPHICS_PRESETS_HPP
