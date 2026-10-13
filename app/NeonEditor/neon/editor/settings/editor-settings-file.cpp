#include "editor-settings-file.hpp"

#include <cstddef>
#include <format>

#include <neon/data/data-reader.hpp>

namespace neon
{
  // Helpers of EditorSettingsFile: the names of the window modes, and one
  // reader per part of the file.
  namespace
  {
    const std::string what_is_read = "the editor's settings";

    const std::vector<std::string> mode_names = {"windowed", "borderless", "fullscreen"};

    /// A whole number of points above zero.
    void read_points(const DataReader &reader, const std::string &name, int &value)
    {
      int number = 0;
      if (!reader.Read(name, number)) { return; }

      if (number <= 0)
      {
        reader.Report(*reader.ReadValue(name), std::format(
                        "'{}' of {} is {}, where a whole number of points above zero was expected",
                        name, reader.GetWhere(), number));
        return;
      }

      value = number;
    }

    void read_window(const DataReader &reader, EditorSettings &settings)
    {
      bool found = false;
      const DataReader window = reader.ReadMap("window", found);
      if (!found) { return; }

      read_points(window, "width", settings.window_width);
      read_points(window, "height", settings.window_height);
      if (std::size_t mode = 0; window.ReadChoice("mode", mode_names, mode))
      {
        settings.window_mode = static_cast<WindowMode>(mode);
      }

      window.Finish();
    }

    /// A path of this machine, which may be empty for none.
    void read_folder(const DataReader &reader, const std::string &name, std::string &value)
    {
      reader.Read(name, value);
    }

    void read_open_windows(const DataReader &reader, EditorSettings &settings)
    {
      std::vector<std::string> titles;
      if (!reader.Read("open_windows", titles)) { return; }

      for (const auto &title : titles)
      {
        if (title.empty())
        {
          reader.Report(*reader.ReadValue("open_windows"), "'open_windows' holds an empty title");
          return;
        }
      }
      settings.open_windows = titles;
    }
  }

  EditorSettingsFile::EditorSettingsFile(
    FileSystemContext *file_system,
    DocumentFormat *format,
    const std::shared_ptr<Logger> &logger)
  {
    _file_system = file_system;
    _format = format;
    _logger = logger;
  }

  bool EditorSettingsFile::Read(
    const std::string &path,
    EditorSettings &settings,
    std::vector<std::string> &errors) const
  {
    std::string text;
    if (!_file_system->ReadText(path, text))
    {
      errors.push_back(std::format("{}: the file cannot be read", path));
      return false;
    }

    return ReadText(path, text, settings, errors);
  }

  bool EditorSettingsFile::ReadText(
    const std::string &name,
    const std::string &text,
    EditorSettings &settings,
    std::vector<std::string> &errors) const
  {
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
                      "the file has version {}, and this editor reads up to version {}", written, version));
    }

    // read apart, so that a file with a mistake changes nothing
    EditorSettings read = settings;
    read_window(reader, read);
    read_folder(reader, "last_project", read.last_project);
    read_folder(reader, "projects_folder", read.projects_folder);
    read_open_windows(reader, read);

    reader.Finish();

    if (errors.size() > before) { return false; }

    settings = read;
    return true;
  }

  std::string EditorSettingsFile::WriteText(const EditorSettings &settings) const
  {
    DataValue document = DataValue::Map();
    document.Set("version", DataValue::Number(version));

    DataValue window = DataValue::Map();
    window.Set("width", DataValue::Number(settings.window_width));
    window.Set("height", DataValue::Number(settings.window_height));
    window.Set("mode", DataValue::Text(mode_names[static_cast<std::size_t>(settings.window_mode)]));
    document.Set("window", window);

    // a folder that was never chosen is left out, not written empty
    if (!settings.last_project.empty()) { document.Set("last_project", DataValue::Text(settings.last_project)); }
    if (!settings.projects_folder.empty())
    {
      document.Set("projects_folder", DataValue::Text(settings.projects_folder));
    }

    DataValue titles = DataValue::List();
    for (const auto &title : settings.open_windows) { titles.Add(DataValue::Text(title)); }
    document.Set("open_windows", titles);

    return _format->Write(document);
  }

  bool EditorSettingsFile::Write(const std::string &path, const EditorSettings &settings) const
  {
    return _file_system->WriteText(path, WriteText(settings));
  }
} // neon
