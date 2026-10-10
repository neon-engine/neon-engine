#include "graphics-presets.hpp"

#include <algorithm>

#include <neon/runtime/settings-config.hpp>

namespace neon
{
  // The built-in table: the one place the engine's values are written down.
  namespace
  {
    const std::vector<GraphicsPreset> built_in = {
      {.name = "low", .anisotropy = 2, .texture_scale = 0.5, .target_scale = 0.5, .target_mipmaps = 1,
       .shadow_map_size = 1024, .shadow_filter = ShadowFilter::None, .shadow_cascades = 1, .shadow_distance = 40.0},
      {.name = "medium", .anisotropy = 4, .texture_scale = 1.0, .target_scale = 1.0, .target_mipmaps = 4,
       .shadow_map_size = 2048, .shadow_filter = ShadowFilter::Pcf, .shadow_cascades = 2, .shadow_distance = 80.0},
      {.name = "high", .anisotropy = 8, .texture_scale = 1.0, .target_scale = 1.0, .target_mipmaps = 0,
       .shadow_map_size = 2048, .shadow_filter = ShadowFilter::Pcf, .shadow_cascades = 4, .shadow_distance = 120.0},
      {.name = "ultra", .anisotropy = 16, .texture_scale = 1.0, .target_scale = 1.0, .target_mipmaps = 0,
       .shadow_map_size = 4096, .shadow_filter = ShadowFilter::Pcf, .shadow_cascades = 4, .shadow_distance = 160.0},
    };
  }

  GraphicsPresets::GraphicsPresets()
  {
    _presets = built_in;
  }

  const std::vector<GraphicsPreset> &GraphicsPresets::BuiltIn()
  {
    return built_in;
  }

  const std::vector<GraphicsPreset> &GraphicsPresets::All() const
  {
    return _presets;
  }

  const GraphicsPreset *GraphicsPresets::Named(const std::string_view name) const
  {
    for (const auto &preset : _presets)
    {
      if (preset.name == name) { return &preset; }
    }
    return nullptr;
  }

  bool GraphicsPresets::IsName(const std::string_view name) const
  {
    return name == kCustom || Named(name) != nullptr;
  }

  std::vector<std::string> GraphicsPresets::Names() const
  {
    std::vector<std::string> names;
    for (const auto &preset : _presets) { names.push_back(preset.name); }
    names.emplace_back(kCustom);
    return names;
  }

  std::string GraphicsPresets::NamesForAMessage() const
  {
    const std::vector<std::string> names = Names();
    std::string message;
    for (std::size_t i = 0; i < names.size(); i++)
    {
      if (i > 0) { message += i + 1 == names.size() ? ", or " : ", "; }
      message += names[i];
    }
    return message;
  }

  std::string_view GraphicsPresets::NameOf(const GraphicsPreset &values) const
  {
    for (const auto &preset : _presets)
    {
      if (preset.HasTheValuesOf(values)) { return preset.name; }
    }
    return kCustom;
  }

  GraphicsPreset GraphicsPresets::ValuesOf(const SettingsConfig &settings)
  {
    return {
      .name = std::string(kCustom),
      .anisotropy = settings.anisotropy,
      .texture_scale = settings.texture_scale,
      .target_scale = settings.target_scale,
      .target_mipmaps = settings.target_mipmaps,
      .shadow_map_size = settings.shadow_map_size,
      .shadow_filter = settings.shadow_filter,
      .shadow_cascades = static_cast<int>(settings.shadow_cascades),
      .shadow_distance = settings.shadow_distance,
    };
  }

  bool GraphicsPresets::Set(const GraphicsPreset &preset)
  {
    if (preset.name.empty() || preset.name == kCustom) { return false; }

    for (auto &existing : _presets)
    {
      if (existing.name == preset.name)
      {
        existing = preset;
        return true;
      }
    }

    _presets.push_back(preset);
    return true;
  }

  bool GraphicsPresets::Remove(const std::string_view name)
  {
    const auto found = std::ranges::find_if(_presets, [&](const GraphicsPreset &preset) { return preset.name == name; });
    if (found == _presets.end()) { return false; }

    _presets.erase(found);
    return true;
  }
} // neon
