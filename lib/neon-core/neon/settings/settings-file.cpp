#include "settings-file.hpp"

#include <format>

#include <neon/data/data-reader.hpp>

namespace neon
{
  namespace
  {
    const std::string what_is_read = "the settings";
  }

  template <typename Number>
  void SettingsFile::ReadCount(const DataReader &reader, const std::string &name, Number &value)
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

  void SettingsFile::ReadAmount(const DataReader &reader, const std::string &name, double &value)
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

  void SettingsFile::ReadWindow(const DataReader &reader, SettingsConfig &settings)
  {
    reader.Read("title", settings.title);
    ReadCount(reader, "width", settings.width);
    ReadCount(reader, "height", settings.height);

    if (std::size_t mode = 0; reader.ReadChoice("mode", {"windowed", "borderless", "fullscreen"}, mode))
    {
      settings.window_mode = static_cast<WindowMode>(mode);
    }

    reader.Finish();
  }

  void SettingsFile::ReadUi(const DataReader &reader, SettingsConfig &settings)
  {
    ReadAmount(reader, "scale", settings.ui_scale);
    reader.Read("start", settings.ui_path);
    reader.Read("pause_menu", settings.pause_menu);
    reader.Read("settings_menu", settings.settings_menu);
    reader.Finish();
  }

  void SettingsFile::ReadWorld(const DataReader &reader, SettingsConfig &settings)
  {
    ReadAmount(reader, "steps_per_second", settings.steps_per_second);
    ReadCount(reader, "most_steps_per_frame", settings.most_steps_per_frame);
    reader.Finish();
  }

  void SettingsFile::ReadInput(const DataReader &reader, SettingsConfig &settings)
  {
    // the player's switch for a sensor, which the map decides without
    if (bool gyro = false; reader.Read("gyro", gyro)) { settings.gyro_enabled = gyro; }
    reader.Finish();
  }

  void SettingsFile::ReadRendering(const DataReader &reader, SettingsConfig &settings)
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

    ReadCount(reader, "max_light_sources", settings.max_light_sources);
    reader.Finish();
  }

  void SettingsFile::ReadPart(
    const DataReader &reader,
    const std::string &name,
    SettingsConfig &settings,
    void (*read)(const DataReader &, SettingsConfig &))
  {
    bool found = false;
    const DataReader part = reader.ReadMap(name, found);
    if (found) { read(part, settings); }
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
    ReadPart(reader, "window", read, ReadWindow);
    ReadPart(reader, "ui", read, ReadUi);
    ReadPart(reader, "world", read, ReadWorld);
    ReadPart(reader, "input", read, ReadInput);
    ReadPart(reader, "rendering", read, ReadRendering);
    reader.Finish();

    if (errors.size() > before) { return false; }

    settings = read;
    return true;
  }
} // neon
