#include "editor-script-loading.hpp"

namespace neon
{
  EditorScriptLoading::EditorScriptLoading(FileSystemContext *file_system, DocumentFormat *document_format)
  {
    _file_system = file_system;
    _document_format = document_format;
  }

  bool EditorScriptLoading::Load(
    const SettingsConfig &settings,
    EditorScript &script,
    std::vector<std::string> &errors) const
  {
    script = EditorScript{};
    const std::string &path = settings.editor_script_path;
    if (path.empty()) { return true; }

    std::string text;
    if (!_file_system->ReadText(path, text))
    {
      errors.push_back("The script of the editor " + path + " cannot be read");
      return false;
    }

    DataValue document;
    std::string error;
    if (!_document_format->Read(path, text, document, error))
    {
      errors.push_back(error);
      return false;
    }

    return EditorScript::Read(document, path, script, errors);
  }
} // neon
