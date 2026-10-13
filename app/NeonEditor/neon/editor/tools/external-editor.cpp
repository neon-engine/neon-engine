#include "external-editor.hpp"

#include <format>

namespace neon
{
  // Helpers of ExternalEditor: the system of a platform name.
  namespace
  {
    /// The system in front of the dash of a platform name, such as `macos`
    /// of `macos-arm64`.
    std::string system_of(const std::string &platform)
    {
      return platform.substr(0, platform.find('-'));
    }
  }

  ExternalEditor::ExternalEditor(
    const Toolchains *toolchains,
    ProcessContext *processes,
    NativeFileContext *native_files,
    const std::string &platform,
    const std::shared_ptr<Logger> &logger)
    : _toolchains(toolchains),
      _processes(processes),
      _native_files(native_files),
      _platform(platform),
      _logger(logger) {}

  std::vector<std::string> ExternalEditor::DefaultPlaces() const
  {
    std::vector<std::string> places{"code"};

    std::string home;
    const bool has_home = _native_files->HomeDirectory(home);

    const std::string system = system_of(_platform);
    if (system == "macos" || system == "darwin")
    {
      const std::string in_bundle = "Visual Studio Code.app/Contents/Resources/app/bin/code";
      places.push_back("/Applications/" + in_bundle);
      if (has_home) { places.push_back(_native_files->Join(_native_files->Join(home, "Applications"), in_bundle)); }
    } else if (system == "windows")
    {
      places.emplace_back("code.cmd");
      if (has_home)
      {
        places.push_back(_native_files->Join(home, "AppData\\Local\\Programs\\Microsoft VS Code\\bin\\code.cmd"));
      }
      places.emplace_back("C:\\Program Files\\Microsoft VS Code\\bin\\code.cmd");
    } else if (system == "linux")
    {
      places.emplace_back("/usr/bin/code");
      places.emplace_back("/snap/bin/code");
      places.emplace_back("/usr/share/code/bin/code");
    }
    return places;
  }

  bool ExternalEditor::Locate(std::string &program) const
  {
    const std::string named = _toolchains->Editor(std::string(name));
    if (!named.empty())
    {
      if (_processes->Find(named, program))
      {
        _logger->Debug("VS Code is {}, as the toolchain files say", program);
        return true;
      }
      _logger->Warn(
        "The toolchain files name VS Code as {}, which is not there; the usual places are looked in instead",
        named);
    }

    for (const auto &place : DefaultPlaces())
    {
      if (_processes->Find(place, program))
      {
        _logger->Info("VS Code was found at {}", program);
        return true;
      }
    }

    _logger->Error(
      "VS Code was not found: it is not on PATH and not where it is usually installed. Say where it is under "
      "'editors: vscode:' of editor.yml or user-editor.yml, the toolchain files of this machine, "
      "see docs/editor-toolchains.md");
    return false;
  }

  ProcessCommand ExternalEditor::Command(
    const std::string &program,
    const std::string &project_root,
    const std::string &file,
    const int line)
  {
    ProcessCommand command{.program = program, .arguments = {"."}, .directory = project_root};
    if (!file.empty())
    {
      command.arguments.emplace_back("--goto");
      command.arguments.push_back(line > 0 ? std::format("{}:{}", file, line) : file);
    }
    return command;
  }

  bool ExternalEditor::Open(const std::string &project_root, const std::string &file, const int line) const
  {
    std::string program;
    if (!Locate(program)) { return false; }

    const ProcessCommand command = Command(program, project_root, file, line);
    if (std::string error; !_processes->Start(command, error))
    {
      _logger->Error("VS Code could not be started: {}", error);
      return false;
    }

    if (file.empty())
    {
      _logger->Info("Opening {} in VS Code", project_root);
    } else
    {
      _logger->Info("Opening {} in VS Code, with {} in front", project_root, file);
    }
    return true;
  }
} // neon
