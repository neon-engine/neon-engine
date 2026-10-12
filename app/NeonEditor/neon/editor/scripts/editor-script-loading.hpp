#ifndef EDITOR_SCRIPT_LOADING_HPP
#define EDITOR_SCRIPT_LOADING_HPP

#include <string>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/runtime/settings-config.hpp>

#include "editor-script.hpp"

namespace neon
{
  /// The step of the editor's start-up that reads the script of its tools,
  /// the file `--editor-script` names, see EditorScript. It reads the file
  /// through the file system and the document through the format, so that
  /// a test hands it a file in memory.
  class EditorScriptLoading final
  {
    FileSystemContext *_file_system;
    DocumentFormat *_document_format;

  public:
    EditorScriptLoading(FileSystemContext *file_system, DocumentFormat *document_format);

    /// Reads the script the settings name, if any, into `script`. False
    /// with the errors when the file cannot be read, is no document, or
    /// says something that is no step. Without a path, true and empty.
    bool Load(const SettingsConfig &settings, EditorScript &script, std::vector<std::string> &errors) const;
  };
} // neon

#endif //EDITOR_SCRIPT_LOADING_HPP
