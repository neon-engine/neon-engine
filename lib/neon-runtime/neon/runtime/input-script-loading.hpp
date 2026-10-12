#ifndef INPUT_SCRIPT_LOADING_HPP
#define INPUT_SCRIPT_LOADING_HPP

#include <string>
#include <vector>

#include <neon/filesystem/file-system-context.hpp>
#include <neon/input/headless-input-system.hpp>
#include <neon/runtime/settings-config.hpp>

namespace neon
{
  /// The step of the start-up that gives a run without a window its input,
  /// see RuntimeComposition: input that is written down, with `--input` or
  /// in the file `--input-script` names, in place of the devices the run
  /// does not have. It names actions of the input map, so it comes after
  /// the map is loaded.
  class InputScriptLoading final
  {
    FileSystemContext *_file_system;

  public:
    explicit InputScriptLoading(FileSystemContext *file_system);

    /// Reads the script the settings name, if any, and hands it to
    /// `input_system`. False with the errors when the file cannot be read,
    /// or the script says something the input cannot do.
    bool Load(const SettingsConfig &settings, Headless_InputSystem &input_system, std::vector<std::string> &errors) const;
  };
} // neon

#endif //INPUT_SCRIPT_LOADING_HPP
