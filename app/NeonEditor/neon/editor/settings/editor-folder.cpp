#include "editor-folder.hpp"

#include <vector>

namespace neon
{
  // Helpers of EditorFolder: the window mode as the log says it.
  namespace
  {
    std::string mode_name_of(const WindowMode mode)
    {
      switch (mode)
      {
        case WindowMode::Borderless:
        {
          return "borderless";
        }
        case WindowMode::Fullscreen:
        {
          return "fullscreen";
        }
        default:
        {
          return "windowed";
        }
      }
    }
  }

  EditorFolder::EditorFolder(
    FileSystemContext *file_system,
    DocumentFormat *format,
    const std::shared_ptr<Logger> &logger)
    : _file(file_system, format, logger)
  {
    _file_system = file_system;
    _logger = logger;
  }

  bool EditorFolder::ReadSettings()
  {
    const std::string path(settings_path);
    _refused = false;

    if (!_file_system->Exists(path))
    {
      _logger->Info("There is no {} yet, the editor starts with its defaults", path);
      return true;
    }

    std::vector<std::string> errors;
    if (!_file.Read(path, _settings, errors))
    {
      for (const auto &error : errors) { _logger->Error("{}", error); }
      _logger->Error(
        "{} is refused, the editor starts with its defaults and leaves the file as it is, so that it can be corrected",
        path);
      _refused = true;
      return false;
    }

    const std::string mode = mode_name_of(_settings.window_mode);
    _logger->Info(
      "The editor's settings were read from {}: a window of {} by {} points, {}",
      path,
      _settings.window_width,
      _settings.window_height,
      mode);
    return true;
  }

  bool EditorFolder::WriteSettings()
  {
    const std::string path(settings_path);
    if (_refused)
    {
      _logger->Warn("{} was refused when it was read, and is not written back", path);
      return false;
    }

    if (!_file.Write(path, _settings))
    {
      _logger->Error("The editor's settings cannot be written to {}", path);
      return false;
    }

    _logger->Info("The editor's settings were written to {}", path);
    return true;
  }

  bool EditorFolder::ReadLayout(std::string &layout)
  {
    const std::string path(layout_path);
    if (!_file_system->Exists(path))
    {
      _logger->Info("There is no {} yet, the windows take their first places", path);
      return false;
    }

    if (!_file_system->ReadText(path, layout)) { return false; }

    _logger->Info("The layout of the windows was read from {}", path);
    return true;
  }

  bool EditorFolder::WriteLayout(const std::string &layout)
  {
    const std::string path(layout_path);
    if (!_file_system->WriteText(path, layout))
    {
      _logger->Error("The layout of the windows cannot be written to {}", path);
      return false;
    }

    _logger->Info("The layout of the windows was written to {}", path);
    return true;
  }
} // neon
