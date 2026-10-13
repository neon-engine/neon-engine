#ifndef EXTERNAL_EDITOR_HPP
#define EXTERNAL_EDITOR_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <neon/editor/toolchains/toolchains.hpp>
#include <neon/filesystem/native-file-context.hpp>
#include <neon/logging/logger.hpp>
#include <neon/process/process-command.hpp>
#include <neon/process/process-context.hpp>

namespace neon
{
  /// Opens the code of the project in Visual Studio Code: at the root of
  /// the project, never the engine's checkout, with a file in front when
  /// one is named. Where VS Code is comes from `editors: vscode:` of the
  /// toolchain files of this machine, see Toolchains and ToolchainFiles.
  /// When they say
  /// nothing, or name a place that is not there, the usual places of the
  /// platform are looked in, in order: `code` on PATH, then where the
  /// installer puts it, see DefaultPlaces(). When it is found nowhere the
  /// log says so and where to set it, in place of nothing happening.
  ///
  /// VS Code is started without waiting, through ProcessContext::Start(),
  /// so the editor goes on while it opens. The command is
  /// `code . --goto <file>:<line>`, started in the folder of the project,
  /// so that the root of the project is what opens whether the folder was
  /// given as an absolute path or relative to where the editor started.
  class ExternalEditor final
  {
    const Toolchains *_toolchains;
    ProcessContext *_processes;
    NativeFileContext *_native_files;
    std::string _platform;
    std::shared_ptr<Logger> _logger;

  public:
    /// The name of VS Code in `editors:` of the toolchain files.
    static constexpr std::string_view name = "vscode";

    /// `platform` is what the platform is called where a library is named
    /// for it, `macos-arm64`, `linux-x86_64`, `windows-x86_64`, as
    /// LibraryLoader::GetPlatform() says it; the system in front of the
    /// dash chooses the places.
    ExternalEditor(
      const Toolchains *toolchains,
      ProcessContext *processes,
      NativeFileContext *native_files,
      const std::string &platform,
      const std::shared_ptr<Logger> &logger);

    /// The places VS Code is looked for when the files say nothing, in
    /// order: a name on PATH, or a native path, under the home folder for
    /// an installation of one user.
    [[nodiscard]] std::vector<std::string> DefaultPlaces() const;

    /// Where VS Code is, as the program to start: what the files name, or
    /// the first of the places above that is there. Returns false when it
    /// is nowhere, and the log says where to set it.
    bool Locate(std::string &program) const;

    /// The command that opens the project at `project_root`, a native
    /// folder, with `file` at `line` in front when `file` is not empty.
    /// `file` is a native path, relative to the root or absolute; a `line`
    /// of 0 opens the file where it was left.
    [[nodiscard]] static ProcessCommand Command(
      const std::string &program,
      const std::string &project_root,
      const std::string &file,
      int line);

    /// Opens the project, with the file in front when one is named. Returns
    /// false when VS Code was not found or could not be started, which the
    /// log says either way.
    bool Open(const std::string &project_root, const std::string &file = "", int line = 0) const;
  };
} // neon

#endif //EXTERNAL_EDITOR_HPP
