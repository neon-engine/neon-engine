#include "player-settings.hpp"

#include "settings-file.hpp"

namespace neon
{
  PlayerSettings::PlayerSettings(
    FileSystemContext *file_system,
    DocumentFormat *format,
    const std::shared_ptr<Logger> &logger)
  {
    _file_system = file_system;
    _format = format;
    _logger = logger;
  }

  void PlayerSettings::Read()
  {
    if (_is_read) { return; }
    _is_read = true;

    const std::string path(SettingsFile::of_the_player);
    std::string text;
    std::string error;
    if (_file_system->Exists(path) && _file_system->ReadText(path, text))
    {
      if (_format->Read(path, text, _document, error) && _document.IsMap()) { return; }
      _logger->Warn("The settings of the player at {} cannot be read, and are written anew: {}", path, error);
    }

    _document = DataValue::Map();
    _document.Set("version", DataValue::Number(SettingsFile::version));
  }

  void PlayerSettings::Set(const std::string &section, const std::string &name, const DataValue &value)
  {
    Read();

    const DataValue *found = _document.Find(section);
    DataValue map = found != nullptr && found->IsMap() ? *found : DataValue::Map();
    map.Set(name, value);
    _document.Set(section, map);
    _changed = true;
  }

  bool PlayerSettings::Write()
  {
    if (!_changed) { return true; }

    const std::string path(SettingsFile::of_the_player);
    if (!_file_system->WriteText(path, _format->Write(_document)))
    {
      _logger->Error("The settings of the player cannot be written to {}", path);
      return false;
    }

    _logger->Info("The settings of the player were written to {}", path);
    _changed = false;
    return true;
  }

  const DataValue &PlayerSettings::Document()
  {
    Read();
    return _document;
  }
} // neon
