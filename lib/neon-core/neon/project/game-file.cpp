#include "game-file.hpp"

#include <algorithm>

#include <neon/render/graphics-preset-reader.hpp>

namespace neon
{
  // Helpers of GameFile: what each part of a project is written as.
  namespace
  {
    /// The filter as `shadow_filter` of a preset is written.
    const char *shadow_filter_word(const ShadowFilter filter)
    {
      return filter == ShadowFilter::Pcf ? "pcf" : "none";
    }

    /// The values of `preset` that differ from `base`, which is what the
    /// reader takes for the values a preset leaves out.
    DataValue values_that_differ(const GraphicsPreset &preset, const GraphicsPreset &base)
    {
      DataValue values = DataValue::Map();
      if (preset.anisotropy != base.anisotropy)
      {
        values.Set(GraphicsPresetReader::kAnisotropy, DataValue::Number(preset.anisotropy));
      }
      if (preset.texture_scale != base.texture_scale)
      {
        values.Set(GraphicsPresetReader::kTextureScale, DataValue::Number(preset.texture_scale));
      }
      if (preset.target_scale != base.target_scale)
      {
        values.Set(GraphicsPresetReader::kTargetScale, DataValue::Number(preset.target_scale));
      }
      if (preset.target_mipmaps != base.target_mipmaps)
      {
        values.Set(GraphicsPresetReader::kTargetMipmaps, DataValue::Number(preset.target_mipmaps));
      }
      if (preset.shadow_map_size != base.shadow_map_size)
      {
        values.Set(GraphicsPresetReader::kShadowMapSize, DataValue::Number(preset.shadow_map_size));
      }
      if (preset.shadow_filter != base.shadow_filter)
      {
        values.Set(GraphicsPresetReader::kShadowFilter, DataValue::Text(shadow_filter_word(preset.shadow_filter)));
      }
      if (preset.shadow_cascades != base.shadow_cascades)
      {
        values.Set(GraphicsPresetReader::kShadowCascades, DataValue::Number(preset.shadow_cascades));
      }
      if (preset.shadow_distance != base.shadow_distance)
      {
        values.Set(GraphicsPresetReader::kShadowDistance, DataValue::Number(preset.shadow_distance));
      }
      return values;
    }

    /// `graphics_presets` as the project wrote it, found from the table:
    /// a preset of the engine the project dropped is `off`, one it changed
    /// holds the values that differ, and one it added holds the values
    /// that differ from the defaults of the engine. Empty when the table
    /// is the engine's.
    DataValue presets_of(const GraphicsPresets &presets)
    {
      const std::vector<GraphicsPreset> &built_in = GraphicsPresets::BuiltIn();
      DataValue written = DataValue::Map();

      for (const auto &preset : built_in)
      {
        if (presets.Named(preset.name) == nullptr) { written.Set(preset.name, DataValue::Text("off")); }
      }

      for (const auto &preset : presets.All())
      {
        const auto engines = std::ranges::find(built_in, preset.name, &GraphicsPreset::name);
        if (engines != built_in.end())
        {
          if (preset.HasTheValuesOf(*engines)) { continue; }
          written.Set(preset.name, values_that_differ(preset, *engines));
        } else
        {
          written.Set(preset.name, values_that_differ(preset, GraphicsPreset{}));
        }
      }
      return written;
    }

    /// A setting of the game as it is declared, with the names in the
    /// order docs/projects.md lists them and what is at its default left
    /// out.
    DataValue declaration_of(const SettingDeclaration &declaration)
    {
      DataValue written = DataValue::Map();
      written.Set("kind", DataValue::Text(setting_kind_name(declaration.kind)));
      if (declaration.kind != SettingKind::Action) { written.Set("default", declaration.default_value); }
      if (declaration.least.has_value()) { written.Set("least", DataValue::Number(*declaration.least)); }
      if (declaration.most.has_value()) { written.Set("most", DataValue::Number(*declaration.most)); }
      if (declaration.kind == SettingKind::Choice)
      {
        DataValue choices = DataValue::List();
        for (const auto &choice : declaration.choices) { choices.Add(DataValue::Text(choice)); }
        written.Set("choices", choices);
      }
      if (!declaration.category.empty()) { written.Set("category", DataValue::Text(declaration.category)); }
      if (!declaration.group.empty()) { written.Set("group", DataValue::Text(declaration.group)); }
      if (declaration.order != 0.0) { written.Set("order", DataValue::Number(declaration.order)); }
      if (declaration.control.has_value())
      {
        written.Set("control", DataValue::Text(setting_control_name(*declaration.control)));
      }
      if (declaration.step.has_value()) { written.Set("step", DataValue::Number(*declaration.step)); }
      if (!declaration.label.empty()) { written.Set("label", DataValue::Text(declaration.label)); }
      return written;
    }
  }

  GameFile::GameFile(
    FileSystemContext *file_system,
    DocumentFormat *format,
    const std::shared_ptr<Logger> &logger)
  {
    _file_system = file_system;
    _format = format;
    _logger = logger;
  }

  bool GameFile::Read(Project &game, std::vector<std::string> &errors) const
  {
    const std::string name(path);
    _logger->Info("Reading the game from {}", name);

    return ProjectFile::ReadLayout(_file_system, _format, name, "game", game, errors);
  }

  std::string GameFile::WriteFrom(const Project &project) const
  {
    DataValue document = DataValue::Map();
    document.Set("version", DataValue::Number(version));
    document.Set("name", DataValue::Text(project.name));
    if (project.organization != Project::default_organization)
    {
      document.Set("organization", DataValue::Text(project.organization));
    }

    DataValue scenes = DataValue::List();
    for (const auto &scene : project.scenes) { scenes.Add(DataValue::Text(scene)); }
    document.Set("scenes", scenes);

    // the first scene unless the project says otherwise, as the reader has it
    if (!project.scenes.empty() && project.entry_scene != project.scenes.front())
    {
      document.Set("entry_scene", DataValue::Text(project.entry_scene));
    }

    if (!project.input.empty()) { document.Set("input", DataValue::Text(project.input)); }

    if (DataValue presets = presets_of(project.graphics_presets); !presets.GetEntries().empty())
    {
      document.Set("graphics_presets", presets);
    }

    if (!project.settings.empty())
    {
      DataValue settings = DataValue::Map();
      for (const auto &declaration : project.settings)
      {
        settings.Set(declaration.name, declaration_of(declaration));
      }
      document.Set("settings", settings);
    }

    return _format->Write(document);
  }
} // neon
