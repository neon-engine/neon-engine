#ifndef PLAYER_SETTINGS_HPP
#define PLAYER_SETTINGS_HPP

#include <memory>
#include <string>

#include <neon/data/data-value.hpp>
#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>

namespace neon
{
  /// What the player chose in a settings menu, kept in user://settings.yml,
  /// the layer that is read on top of the project's settings at the next
  /// start, see SettingsFile. What the file holds already is kept: a value
  /// that is set replaces its own name alone, and what the player never
  /// changed stays the project's to decide.
  class PlayerSettings final
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;
    std::shared_ptr<Logger> _logger;

    DataValue _document;
    bool _is_read = false;
    bool _changed = false;

    /// Reads the file once, or starts a new one when there is none or it
    /// cannot be read, which is logged.
    void Read();

  public:
    PlayerSettings(FileSystemContext *file_system, DocumentFormat *format, const std::shared_ptr<Logger> &logger);

    /// Sets `name` of `section`, such as `vsync` of `rendering`, to `value`.
    /// Nothing is written until Write().
    void Set(const std::string &section, const std::string &name, const DataValue &value);

    /// Sets `name` of the map `map` of `section`, such as `music` of
    /// `volumes` of `audio`, keeping what else that map holds. Nothing is
    /// written until Write().
    void Set(const std::string &section, const std::string &map, const std::string &name, const DataValue &value);

    /// Whether anything was set since the last Write().
    [[nodiscard]] bool HasChanges() const { return _changed; }

    /// Writes the file when anything was set. Returns false when it cannot
    /// be written, which is logged.
    bool Write();

    /// What the file holds, as it would be written.
    [[nodiscard]] const DataValue &Document();
  };
} // neon

#endif //PLAYER_SETTINGS_HPP
