#include "project-file.hpp"

#include <algorithm>
#include <format>

#include <neon/data/data-reader.hpp>
#include <neon/render/graphics-preset-reader.hpp>

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

    reader.Finish();

    if (errors.size() > before) { return false; }

    project = read;
    return true;
  }
} // neon
