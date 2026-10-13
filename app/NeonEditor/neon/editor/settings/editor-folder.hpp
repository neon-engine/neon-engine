#ifndef EDITOR_FOLDER_HPP
#define EDITOR_FOLDER_HPP

#include <memory>
#include <string>
#include <string_view>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>

#include "editor-settings.hpp"
#include "editor-settings-file.hpp"

namespace neon
{
  /// The editor's own folder, `editor://`, and the two files in it: the
  /// settings of the editor, `editor-settings.yml`, see EditorSettings,
  /// and the layout of its windows, `layout.ini`, which is the toolkit's
  /// text, see ToolUiSystem::SaveLayout(). The folder is the one the
  /// platform keeps for an application, under the editor's own names, so
  /// that it is the same whichever project is open, and apart from the
  /// project's `user://`, which a run of the game writes to (#582).
  ///
  /// A settings file with a mistake is refused as a whole: every problem is
  /// logged with its line, the editor starts with its defaults, and the
  /// file is not written back at the end, so that the mistake can be
  /// corrected.
  class EditorFolder final
  {
    FileSystemContext *_file_system;
    EditorSettingsFile _file;
    std::shared_ptr<Logger> _logger;

    EditorSettings _settings;

    // whether the file that is there was refused, and is left alone
    bool _refused = false;

  public:
    /// Who the editor is to the platform, and what it is called: the names
    /// `editor://` is placed by. The toolchains of docs/editor-toolchains.md
    /// read `user-editor.yml` from the same folder.
    static constexpr std::string_view organization = "neon-engine";
    static constexpr std::string_view application = "neon-editor";

    static constexpr std::string_view settings_path = "editor://editor-settings.yml";
    static constexpr std::string_view layout_path = "editor://layout.ini";

    EditorFolder(FileSystemContext *file_system, DocumentFormat *format, const std::shared_ptr<Logger> &logger);

    /// Reads the settings, once the folder is placed. A file that is not
    /// there leaves the defaults, and is said. Returns false when the file
    /// is there and was refused, with every problem in the log.
    bool ReadSettings();

    [[nodiscard]] EditorSettings &GetSettings() { return _settings; }

    /// Writes the settings, at the end of a run. Returns false when the
    /// file was refused when it was read, which leaves it as it is, or
    /// cannot be written.
    bool WriteSettings();

    /// Reads the layout of the windows. Returns false when there is none
    /// yet, which is said.
    bool ReadLayout(std::string &layout);

    /// Writes the layout of the windows, at the end of a run.
    bool WriteLayout(const std::string &layout);
  };
} // neon

#endif //EDITOR_FOLDER_HPP
