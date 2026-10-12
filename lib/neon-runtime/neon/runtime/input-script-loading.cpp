#include "input-script-loading.hpp"

#include <neon/input/input-script.hpp>

namespace neon
{
  InputScriptLoading::InputScriptLoading(FileSystemContext *file_system) : _file_system(file_system) {}

  bool InputScriptLoading::Load(
    const SettingsConfig &settings,
    Headless_InputSystem &input_system,
    std::vector<std::string> &errors) const
  {
    std::string text = settings.input_script;
    std::string name = "--input";

    if (!settings.input_script_path.empty())
    {
      name = settings.input_script_path;
      if (!_file_system->ReadText(settings.input_script_path, text))
      {
        errors.push_back("The script of input " + name + " cannot be read");
        return false;
      }
    }

    if (text.empty()) { return true; }

    InputScript script;
    return InputScript::Parse(text, name, script, errors) && input_system.SetScript(script, errors);
  }
} // neon
