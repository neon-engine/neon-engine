#ifndef EDITOR_SETTINGS_FILE_HPP
#define EDITOR_SETTINGS_FILE_HPP

#include <memory>
#include <string>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>

#include "editor-settings.hpp"

namespace neon
{
  /// The file of the editor's own settings, see EditorSettings:
  ///
  ///     version: 1
  ///     window:
  ///       width: 1600
  ///       height: 1000
  ///       mode: windowed
  ///     last_project: /Users/someone/projects/museum
  ///     projects_folder: /Users/someone/projects
  ///     open_windows: [Log]
  ///
  /// It is read as every recipe is: with the document format of the
  /// engine, every problem reported with its line, a name that is not known
  /// refused. A name that is left out keeps its default. `version` is the
  /// version of this layout. `mode` is windowed, borderless, or fullscreen,
  /// as `window.mode` of a project's settings is. The two folders are
  /// paths of this machine, which is where the file stays.
  class EditorSettingsFile final
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;
    std::shared_ptr<Logger> _logger;

  public:
    /// The version of the layout of the file that is read and written.
    static constexpr int version = 1;

    EditorSettingsFile(FileSystemContext *file_system, DocumentFormat *format, const std::shared_ptr<Logger> &logger);

    /// Reads the file at a virtual path into `settings`, over what is
    /// there. Returns false when the file cannot be read or something in
    /// it is wrong, and every problem that was found is in `errors` with
    /// its line, not only the first. Nothing is changed then.
    bool Read(const std::string &path, EditorSettings &settings, std::vector<std::string> &errors) const;

    /// Reads the text of a file, as Read() does once it has it. `name` is
    /// what the file is called in messages.
    bool ReadText(
      const std::string &name,
      const std::string &text,
      EditorSettings &settings,
      std::vector<std::string> &errors) const;

    /// Writes the settings to a virtual path, replacing the file. Returns
    /// false when it cannot be written, which the file system has said.
    bool Write(const std::string &path, const EditorSettings &settings) const;

    /// The text Write() writes.
    [[nodiscard]] std::string WriteText(const EditorSettings &settings) const;
  };
} // neon

#endif //EDITOR_SETTINGS_FILE_HPP
