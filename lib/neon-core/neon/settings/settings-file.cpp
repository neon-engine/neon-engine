#include "settings-file.hpp"

#include <algorithm>
#include <format>

#include <neon/window/frame-limit.hpp>
#include <neon/data/data-reader.hpp>

namespace neon
{
  // Helpers of SettingsFile, for this file alone.
  namespace
  {
    const std::string what_is_read = "the settings";

    /// A whole number above zero, read into a value of any integral type.
    template <typename Number>
    void read_count(const DataReader &reader, const std::string &name, Number &value)
    {
      int number = 0;
      if (!reader.Read(name, number)) { return; }

      if (number <= 0)
      {
        reader.Report(*reader.ReadValue(name), std::format(
                        "'{}' of {} is {}, where a whole number above zero was expected",
                        name, reader.GetWhere(), number));
        return;
      }

      value = static_cast<Number>(number);
    }

    /// A number above zero, read into a double.
    void read_amount(const DataReader &reader, const std::string &name, double &value)
    {
      float number = 0.0f;
      if (!reader.Read(name, number)) { return; }

      if (number <= 0.0f)
      {
        reader.Report(*reader.ReadValue(name), std::format(
                        "'{}' of {} is {}, where a number above zero was expected",
                        name, reader.GetWhere(), number));
        return;
      }

      value = number;
    }

    void read_window(const DataReader &reader, SettingsConfig &settings)
    {
      reader.Read("title", settings.title);
      read_count(reader, "width", settings.width);
      read_count(reader, "height", settings.height);

      if (std::size_t mode = 0; reader.ReadChoice("mode", {"windowed", "borderless", "fullscreen"}, mode))
      {
        settings.window_mode = static_cast<WindowMode>(mode);
      }

      reader.Finish();
    }

    void read_ui(const DataReader &reader, SettingsConfig &settings)
    {
      read_amount(reader, "scale", settings.ui_scale);
      reader.Read("start", settings.ui_path);
      reader.Read("pause_menu", settings.pause_menu);
      reader.Read("settings_menu", settings.settings_menu);
      reader.Finish();
    }

    void read_world(const DataReader &reader, SettingsConfig &settings)
    {
      read_amount(reader, "steps_per_second", settings.steps_per_second);
      read_count(reader, "most_steps_per_frame", settings.most_steps_per_frame);
      reader.Finish();
    }

    void read_input(const DataReader &reader, SettingsConfig &settings)
    {
      // the player's switch for a sensor, which the map decides without
      if (bool gyro = false; reader.Read("gyro", gyro)) { settings.gyro_enabled = gyro; }

      // whether the mouse, like a key, takes the hints back from a gamepad
      reader.Read("mouse_switches_device", settings.mouse_switches_device);
      reader.Finish();
    }

    void read_scripting(const DataReader &reader, SettingsConfig &settings)
    {
      reader.Read("jit", settings.script_jit);
      reader.Finish();
    }

    void read_rendering(const DataReader &reader, SettingsConfig &settings)
    {
      // Written in quotes, since 1.10 as a number is 1.1
      if (const DataValue *written = reader.ReadValue("vulkan_version"); written != nullptr)
      {
        std::string text;
        ApiVersion version;
        if (!written->GetText(text) || !ApiVersion::Parse(text, version) || version.major != 1)
        {
          reader.Report(*written, std::format(
                          "'vulkan_version' of {} is not a version of Vulkan 1 in quotes, such as \"1.3\"",
                          reader.GetWhere()));
        } else
        {
          settings.vulkan_version = version;
        }
      }

      read_count(reader, "max_light_sources", settings.max_light_sources);
      read_count(reader, "max_render_objects", settings.max_render_objects);
      read_amount(reader, "shadow_distance", settings.shadow_distance);
      read_count(reader, "shadow_cascades", settings.shadow_cascades);
      if (settings.shadow_cascades > 4)
      {
        reader.Report(*reader.ReadValue("shadow_cascades"), std::format(
                        "'shadow_cascades' of {} is {}, where 1 to 4 was expected", reader.GetWhere(), settings.shadow_cascades));
        settings.shadow_cascades = 4;
      }

      // the curve of the resolve step, and how bright the scene is taken
      // to be before it, see docs/vulkan-renderer.md
      if (std::size_t curve = 0; reader.ReadChoice("tonemapper", {"none", "aces", "agx"}, curve))
      {
        settings.tonemapper = static_cast<Tonemapper>(curve);
      }
      read_amount(reader, "exposure", settings.exposure);

      // whether a frame waits for the screen, see docs/vulkan-renderer.md
      reader.Read("vsync", settings.vertical_sync);

      // how many samples a texture is read with from the side
      if (int level = 0; reader.Read("anisotropy", level))
      {
        if (!Anisotropy::IsLevel(level))
        {
          reader.Report(*reader.ReadValue("anisotropy"), std::format(
                          "'anisotropy' of {} is {}, where 1, 2, 4, 8, or 16 was expected", reader.GetWhere(), level));
        } else
        {
          settings.anisotropy = level;
        }
      }

      // the most frames a second, 0 for as many as can be drawn
      if (int most = 0; reader.Read("max_fps", most))
      {
        if (most != 0 && (most < FrameLimit::kLeast || most > FrameLimit::kMost))
        {
          reader.Report(*reader.ReadValue("max_fps"), std::format(
                          "'max_fps' of {} is {}, where 0 for no limit or a number from {} to {} was expected",
                          reader.GetWhere(), most, static_cast<int>(FrameLimit::kLeast), static_cast<int>(FrameLimit::kMost)));
        } else
        {
          settings.max_fps = most;
        }
      }
      reader.Finish();
    }

    /// The names of the groups, for a message.
    std::string names_of(const std::vector<SoundGroupSetting> &groups)
    {
      std::string names;
      for (const auto &group : groups)
      {
        if (!names.empty()) { names += ", "; }
        names += group.name;
      }
      return names;
    }

    SoundGroupSetting *find_group(std::vector<SoundGroupSetting> &groups, const std::string &name)
    {
      const auto it = std::ranges::find(groups, name, &SoundGroupSetting::name);
      return it == groups.end() ? nullptr : &*it;
    }

    /// A volume of a group: 0 for silence, 1 for the loudness of its sounds,
    /// not below 0.
    bool read_volume(const DataReader &reader, const std::string &name, float &volume)
    {
      float number = 0.0f;
      if (!reader.Read(name, number)) { return false; }

      if (number < 0.0f)
      {
        reader.Report(*reader.ReadValue(name), std::format(
                        "'{}' of {} is {}, where a number of at least 0 was expected, 1 being the loudness "
                        "of the sounds",
                        name, reader.GetWhere(), number));
        return false;
      }

      volume = number;
      return true;
    }

    /// The groups a project declares: a list of names, or of maps with a
    /// name and the volume the group starts at. Each is one more than the
    /// groups of the engine and the ones declared before it.
    void read_groups(const DataReader &reader, SettingsConfig &settings)
    {
      const DataValue *written = reader.ReadValue("groups");
      if (written == nullptr) { return; }

      if (!written->IsList())
      {
        reader.Report(*written, std::format(
                        "'groups' of {} is {}, where a list of names was expected",
                        reader.GetWhere(), DataValue::Describe(written->GetKind())));
        return;
      }

      std::size_t position = 0;
      for (const auto &item : written->GetItems())
      {
        position++;
        SoundGroupSetting group;

        if (item.IsMap())
        {
          const DataReader entry(item, reader.GetDocument(),
                                 std::format("group {} of 'groups' of {}", position, reader.GetWhere()),
                                 reader.GetErrors());
          const bool named = entry.Read("name", group.name);
          if (!named && !entry.Has("name")) { entry.Report("'name' is missing. It holds the name of the group"); }
          read_volume(entry, "volume", group.volume);
          entry.Finish();
          if (!named) { continue; }
        } else if (!item.GetText(group.name))
        {
          reader.Report(item, std::format(
                          "'groups' of {} holds {}, where the name of a group was expected, or a map with "
                          "its name and volume",
                          reader.GetWhere(), DataValue::Describe(item.GetKind())));
          continue;
        }

        if (group.name.empty())
        {
          reader.Report(item, std::format("'groups' of {} holds a group without a name", reader.GetWhere()));
          continue;
        }

        if (find_group(settings.sound_groups, group.name) != nullptr)
        {
          reader.Report(item, std::format(
                          "'groups' of {} declares '{}', which is a group already. There are: {}",
                          reader.GetWhere(), group.name, names_of(settings.sound_groups)));
          continue;
        }

        settings.sound_groups.push_back(group);
      }
    }

    /// The volumes of the groups, by name: those of the engine, and those
    /// the project declared, in this file or one read before it.
    void read_volumes(const DataReader &reader, SettingsConfig &settings)
    {
      bool found = false;
      const DataReader volumes = reader.ReadMap("volumes", found);
      if (!found) { return; }

      for (const auto &[name, value] : reader.ReadValue("volumes")->GetEntries())
      {
        SoundGroupSetting *group = find_group(settings.sound_groups, name);
        if (group == nullptr)
        {
          volumes.Report(value, std::format(
                           "'{}' of {} is not a group. There are: {}",
                           name, volumes.GetWhere(), names_of(settings.sound_groups)));
          continue;
        }

        read_volume(volumes, name, group->volume);
      }
    }

    void read_audio(const DataReader &reader, SettingsConfig &settings)
    {
      // the groups first, so that a volume may name a group declared above it
      read_groups(reader, settings);
      read_volumes(reader, settings);
      reader.Finish();
    }

    /// Reads the map under a name, when it is written.
    void read_part(
      const DataReader &reader,
      const std::string &name,
      SettingsConfig &settings,
      void (*read)(const DataReader &, SettingsConfig &))
    {
      bool found = false;
      const DataReader part = reader.ReadMap(name, found);
      if (found) { read(part, settings); }
    }
  }

  SettingsFile::SettingsFile(
    FileSystemContext *file_system,
    DocumentFormat *format,
    const std::shared_ptr<Logger> &logger)
  {
    _file_system = file_system;
    _format = format;
    _logger = logger;
  }

  bool SettingsFile::Read(
    const std::string &path,
    SettingsConfig &settings,
    bool &found,
    std::vector<std::string> &errors) const
  {
    found = _file_system->Exists(path);
    if (!found)
    {
      _logger->Info("There are no settings at {}", path);
      return true;
    }

    _logger->Info("Reading the settings from {}", path);

    std::string text;
    if (!_file_system->ReadText(path, text))
    {
      errors.push_back(std::format("{}: the settings cannot be read", path));
      return false;
    }

    DataValue document;
    if (std::string error; !_format->Read(path, text, document, error))
    {
      errors.push_back(error);
      return false;
    }

    const std::size_t before = errors.size();
    const DataReader reader(document, path, what_is_read, errors);

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
                      "the settings have version {}, and this engine reads up to version {}",
                      written, version));
    }

    // read on top of a copy, so that a file with a mistake changes nothing
    SettingsConfig read = settings;
    read_part(reader, "window", read, read_window);
    read_part(reader, "ui", read, read_ui);
    read_part(reader, "world", read, read_world);
    read_part(reader, "input", read, read_input);
    read_part(reader, "scripting", read, read_scripting);
    read_part(reader, "rendering", read, read_rendering);
    read_part(reader, "audio", read, read_audio);
    reader.Finish();

    if (errors.size() > before) { return false; }

    settings = read;
    return true;
  }
} // neon
