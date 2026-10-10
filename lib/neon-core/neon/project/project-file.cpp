#include "project-file.hpp"

#include <algorithm>
#include <format>

#include <neon/data/data-reader.hpp>
#include <neon/render/graphics-preset-reader.hpp>
#include <neon/settings/settings-store.hpp>

namespace neon
{
  // Helpers of ProjectFile, for this file alone.
  namespace
  {
    const std::string what_is_read = "the project";
    const std::string graphics_presets = "graphics_presets";

    /// The names of the presets of a table, for a message.
    std::string names_of(const GraphicsPresets &presets)
    {
      std::string names;
      for (const auto &preset : presets.All())
      {
        if (!names.empty()) { names += ", "; }
        names += preset.name;
      }
      return names;
    }

    /// Reads `graphics_presets`, when it is written, on top of the table
    /// of the engine: a name with a map changes the preset of that name
    /// value by value, or adds one with the defaults of the engine for what
    /// it leaves out; a name with `off` drops the preset.
    void read_graphics_presets(const DataReader &reader, GraphicsPresets &presets)
    {
      const DataValue *written = reader.ReadValue(graphics_presets);
      if (written == nullptr) { return; }

      if (written->GetKind() != DataValue::Kind::Map)
      {
        reader.Report(*written, std::format(
                        "'{}' is {}, where a map of presets by name was expected", graphics_presets,
                        DataValue::Describe(written->GetKind())));
        return;
      }

      for (const auto &[name, value] : written->GetEntries())
      {
        if (name == GraphicsPresets::kCustom)
        {
          reader.Report(value, std::format(
                          "'{}' of '{}' is the name of no preset: it is what the settings menu shows when the "
                          "values match none, and cannot be defined or dropped",
                          name, graphics_presets));
          continue;
        }

        if (!ProjectFile::IsPlainName(name))
        {
          reader.Report(value, std::format(
                          "'{}' of '{}' is not a plain name: lowercase letters, digits, and dashes, starting with "
                          "a letter, as a preset is named in the settings and on the command line",
                          name, graphics_presets));
          continue;
        }

        // `off` drops a preset, so that a menu does not offer it
        if (std::string text; value.GetText(text) && text == "off")
        {
          if (!presets.Remove(name))
          {
            reader.Report(value, std::format(
                            "'{}' of '{}' is off, and there is no preset {} to drop. There are: {}",
                            name, graphics_presets, name, names_of(presets)));
          }
          continue;
        }

        if (value.GetKind() != DataValue::Kind::Map)
        {
          reader.Report(value, std::format(
                          "'{}' of '{}' is {}, where a map of the values of the preset was expected, or off to "
                          "drop it",
                          name, graphics_presets, DataValue::Describe(value.GetKind())));
          continue;
        }

        // on top of the preset of that name, or the defaults of the engine
        GraphicsPreset preset;
        if (const GraphicsPreset *existing = presets.Named(name); existing != nullptr) { preset = *existing; }
        preset.name = name;

        const DataReader values(value, reader.GetDocument(), std::format("preset '{}' of the project", name),
                                reader.GetErrors());
        GraphicsPresetReader::Read(values, preset);
        values.Finish();

        (void) presets.Set(preset);
      }

      if (presets.All().empty())
      {
        reader.Report(*written, std::format(
                        "'{}' drops every preset, and the settings menu offers at least one", graphics_presets));
      }
    }

    SettingDeclaration *find_setting(std::vector<SettingDeclaration> &settings, const std::string &name)
    {
      const auto it = std::ranges::find(settings, name, &SettingDeclaration::name);
      return it == settings.end() ? nullptr : &*it;
    }

    /// A setting the game declares: a map with its kind, its default, for
    /// a number its least and most, for a choice its choices, and where and
    /// how the settings menu shows it.
    void read_declaration(
      const DataReader &reader,
      const std::string &name,
      const DataValue &written,
      std::vector<SettingDeclaration> &settings)
    {
      if (!SettingsStore::IsName(name))
      {
        reader.Report(written, std::format(
                        "'{}' of {} is not the name of a setting: letters, digits, and underscores were expected",
                        name, reader.GetWhere()));
        return;
      }


      if (find_setting(settings, name) != nullptr)
      {
        reader.Report(written, std::format("'{}' of {} is declared twice", name, reader.GetWhere()));
        return;
      }

      const DataReader entry(written, reader.GetDocument(), std::format("'{}' of {}", name, reader.GetWhere()),
                             reader.GetErrors());
      SettingDeclaration declaration;
      declaration.name = name;

      std::string kind;
      if (!entry.Read("kind", kind))
      {
        if (!entry.Has("kind")) { entry.Report("'kind' is missing. It is flag, number, text, choice, or action"); }
        return;
      }
      if (!setting_kind_of(kind, declaration.kind))
      {
        entry.Report(*entry.ReadValue("kind"), std::format(
                       "'kind' of {} is '{}', where flag, number, text, choice, or action was expected", entry.GetWhere(), kind));
        return;
      }

      const DataValue *fallback = entry.ReadValue("default");
      if (declaration.kind == SettingKind::Action)
      {
        if (fallback != nullptr)
        {
          entry.Report(*fallback, std::format("'default' of {} is not taken: an action holds nothing", entry.GetWhere()));
          return;
        }
      } else if (fallback == nullptr)
      {
        entry.Report("'default' is missing. It is what the setting holds until the player or the game changes it");
        return;
      } else
      {
        declaration.default_value = *fallback;
      }

      if (declaration.kind == SettingKind::Number)
      {
        if (double least = 0.0; entry.Read("least", least)) { declaration.least = least; }
        if (double most = 0.0; entry.Read("most", most)) { declaration.most = most; }
        if (declaration.least.has_value() && declaration.most.has_value() && *declaration.least > *declaration.most)
        {
          entry.Report(*entry.ReadValue("least"), std::format(
                         "'least' of {} is {}, which is above 'most', {}", entry.GetWhere(), *declaration.least,
                         *declaration.most));
        }
        if (double step = 0.0; entry.Read("step", step))
        {
          if (step <= 0.0)
          {
            entry.Report(*entry.ReadValue("step"), std::format(
                           "'step' of {} is {}, where a number above zero was expected", entry.GetWhere(), step));
          } else
          {
            declaration.step = step;
          }
        }
      } else if (declaration.kind == SettingKind::Choice)
      {
        if (!entry.Read("choices", declaration.choices) || declaration.choices.empty())
        {
          if (!entry.Has("choices")) { entry.Report("'choices' is missing. It lists the texts the setting may be"); }
          else if (declaration.choices.empty()) { entry.Report(*entry.ReadValue("choices"), std::format("'choices' of {} is empty", entry.GetWhere())); }
          return;
        }
      }

      // where and how the menu shows it
      entry.Read("category", declaration.category);
      entry.Read("group", declaration.group);
      entry.Read("order", declaration.order);
      entry.Read("label", declaration.label);
      if (std::string control; entry.Read("control", control))
      {
        SettingControl chosen = SettingControl::Field;
        if (!setting_control_of(control, chosen))
        {
          entry.Report(*entry.ReadValue("control"), std::format(
                         "'control' of {} is '{}', where slider, toggle, checkbox, dropdown, field, or button was expected",
                         entry.GetWhere(), control));
          return;
        }
        if (!control_fits_kind(chosen, declaration.kind))
        {
          entry.Report(*entry.ReadValue("control"), std::format(
                         "'control' of {} is {}, which does not show {}: a slider shows a number, a toggle and a "
                         "checkbox a flag, a dropdown a choice, a field a text or a number, a button an action",
                         entry.GetWhere(), control, setting_kind_name(declaration.kind)));
          return;
        }
        if (chosen == SettingControl::Slider && !(declaration.least.has_value() && declaration.most.has_value()))
        {
          entry.Report(*entry.ReadValue("control"), std::format(
                         "'control' of {} is a slider, which needs 'least' and 'most'", entry.GetWhere()));
          return;
        }
        declaration.control = chosen;
      }

      // what a kind has no use for is a mistake, as any other name is; a
      // declaration refused above is not looked at further
      entry.Finish();

      if (std::string why; !SettingsStore::Fits(declaration, declaration.default_value, why))
      {
        entry.Report(*fallback, std::format("'default' of {} does not fit: {}", entry.GetWhere(), why));
        return;
      }

      settings.push_back(declaration);
    }

    /// The settings of the game, each declared under its name.
    void read_settings(const DataReader &reader, std::vector<SettingDeclaration> &settings)
    {
      const DataValue *written = reader.ReadValue("settings");
      if (written == nullptr) { return; }

      if (!written->IsMap())
      {
        reader.Report(*written, std::format(
                        "'settings' of {} is {}, where a map of settings was expected",
                        reader.GetWhere(), DataValue::Describe(written->GetKind())));
        return;
      }

      const DataReader part(*written, reader.GetDocument(), std::format("'settings' of {}", reader.GetWhere()), reader.GetErrors());
      for (const auto &[name, declaration] : written->GetEntries())
      {
        if (!declaration.IsMap())
        {
          part.Report(declaration, std::format(
                        "'{}' of {} is {}, where a map with the kind and the default of the setting was expected. "
                        "What a setting is set to is in settings.yml under 'game'",
                        name, part.GetWhere(), DataValue::Describe(declaration.GetKind())));
          continue;
        }
        read_declaration(part, name, declaration, settings);
      }
    }
  }

  ProjectFile::ProjectFile(
    FileSystemContext *file_system,
    DocumentFormat *format,
    const std::shared_ptr<Logger> &logger)
  {
    _file_system = file_system;
    _format = format;
    _logger = logger;
  }

  bool ProjectFile::IsPlainName(const std::string_view name)
  {
    if (name.empty() || name.front() < 'a' || name.front() > 'z') { return false; }

    return std::ranges::all_of(name, [](const char character)
    {
      return (character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') || character == '-';
    });
  }

  bool ProjectFile::Read(Project &project, std::vector<std::string> &errors) const
  {
    const std::string name(path);
    _logger->Info("Reading the project from {}", name);

    std::string text;
    if (!_file_system->ReadText(name, text))
    {
      errors.push_back(std::format("{}: there is no project here, the file cannot be read", name));
      return false;
    }

    DataValue document;
    if (std::string error; !_format->Read(name, text, document, error))
    {
      errors.push_back(error);
      return false;
    }

    const std::size_t before = errors.size();
    const DataReader reader(document, name, what_is_read, errors);

    // The version is the first thing read, so that a file of a later layout
    // says so before its names are reported as unknown.
    int written = 0;
    if (!reader.Read("version", written))
    {
      if (!reader.Has("version"))
      {
        reader.Report(std::format("'version' is missing. It holds the version of the layout, which is {}", version));
      }
    } else if (written > version)
    {
      reader.Report(*document.Find("version"), std::format(
                      "the project has version {}, and this engine reads up to version {}",
                      written, version));
    }

    Project read;

    /// Reads a plain name. Returns false when it is not written, or empty.
    const auto read_name = [&](const std::string &key, std::string &value)
    {
      if (!reader.Read(key, value)) { return false; }
      if (value.empty()) { return false; }

      if (!IsPlainName(value))
      {
        reader.Report(*document.Find(key), std::format(
                        "'{}' is '{}', where a plain name was expected: lowercase letters, digits, and dashes, "
                        "starting with a letter. It becomes a folder name on every platform",
                        key, value));
      }
      return true;
    };

    // the name is what a project is known by, and there is no default for it
    if (!read_name("name", read.name))
    {
      if (!reader.Has("name"))
      {
        reader.Report("'name' is missing. It is what the project is called");
      } else if (read.name.empty())
      {
        reader.Report(*document.Find("name"), "'name' is empty. It is what the project is called");
      }
    }

    // a project without an organization, or with an empty one, is one of
    // the engine's own
    if (!read_name("organization", read.organization))
    {
      read.organization = Project::default_organization;
    }

    if (!reader.Read("scenes", read.scenes))
    {
      if (!reader.Has("scenes")) { reader.Report("'scenes' is missing. It lists the scenes of the project"); }
    } else if (read.scenes.empty())
    {
      reader.Report(*document.Find("scenes"), "'scenes' is empty, a project has at least one scene");
    }

    if (reader.Read("entry_scene", read.entry_scene))
    {
      if (!read.scenes.empty() && std::ranges::find(read.scenes, read.entry_scene) == read.scenes.end())
      {
        reader.Report(*document.Find("entry_scene"), std::format(
                        "'entry_scene' is '{}', which is not one of the scenes", read.entry_scene));
      }
    } else if (!read.scenes.empty())
    {
      read.entry_scene = read.scenes.front();
    }

    // the map is read later, by InputMapFile, so only that it is named with
    // a path is checked here
    if (reader.Read("input", read.input) && read.input.empty())
    {
      reader.Report(*document.Find("input"), "'input' is empty. It is the path of the input map, or left out");
    }

    // the quality presets the game offers, on top of the engine's
    read_graphics_presets(reader, read.graphics_presets);
    read_settings(reader, read.settings);

    reader.Finish();

    if (errors.size() > before) { return false; }

    project = read;
    return true;
  }
} // neon
