#ifndef TOOLCHAIN_FILE_HPP
#define TOOLCHAIN_FILE_HPP

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <neon/data/document-format.hpp>
#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>

#include "toolchains.hpp"

namespace neon
{
  /// A file that declares toolchains for the editor: how a project is
  /// configured, built, and run, on this machine or in a Docker image.
  ///
  ///     version: 1
  ///     toolchains:
  ///       macos-arm64:
  ///         kind: native
  ///         platform: darwin-arm64
  ///         requires: [cmake, ninja, /opt/homebrew/opt/llvm@20/bin/clang++]
  ///         configure: cmake -S {project} -B {build} --preset macos-arm64-release
  ///         build: cmake --build {build}
  ///         run: {build}/{name}/{name}
  ///       linux-x64:
  ///         kind: docker
  ///         platform: linux-x86_64
  ///         image: neon-engine/linux-x64
  ///         requires: [docker]
  ///         configure: cmake -S {project} -B {build} --preset linux-x64-release
  ///         build: cmake --build {build}
  ///         run: off
  ///     editors:
  ///       vscode: /Applications/Visual Studio Code.app/Contents/Resources/app/bin/code
  ///
  /// There are two such files. `editor.yml` at the root of a project is
  /// the team's, committed with it. `user-editor.yml` is one machine's: the
  /// editor reads it from its own folder, `editor://`, and then from the root
  /// of the project, where it is ignored by git, the second laid over the
  /// first and both over the team's, see Toolchains::Apply. A toolchain of
  /// the user's file replaces the whole entry of its name, and `off` in
  /// its place drops it.
  ///
  /// `version` is the version of this layout. A toolchain is `native` or
  /// `docker`, says the `platform` it builds for in the engine's naming,
  /// lists in `requires` the programs that have to be installed, see
  /// ToolchainCheck, and gives its three commands with the placeholders of
  /// ToolchainPlaceholders. A Docker one names its `image`. `run` is `off`,
  /// or left out, when the result cannot be started on this machine.
  /// `editors` holds where the editors the editor opens are, by name, which
  /// is read by whoever opens one. A name that is not known is an error,
  /// as in every recipe.
  ///
  /// Which format the file has is up to the DocumentFormat that is handed in.
  class ToolchainFile final
  {
    FileSystemContext *_file_system;
    DocumentFormat *_format;
    std::shared_ptr<Logger> _logger;

  public:
    /// The version of the layout of the file that is read, and the highest
    /// that is understood.
    static constexpr int version = 1;

    /// The team's file, at the root of the project.
    static constexpr std::string_view shared_name = "editor.yml";

    /// One machine's file, at the root of the project, and in the editor's
    /// own folder, `editor://`, see EditorFolder.
    static constexpr std::string_view user_name = "user-editor.yml";
    static constexpr std::string_view user_path = "editor://user-editor.yml";

    ToolchainFile(FileSystemContext *file_system, DocumentFormat *format, const std::shared_ptr<Logger> &logger);

    /// Reads the file at a virtual path into `toolchains`, on top of what
    /// is there. Returns false when the file cannot be read or something in
    /// it is wrong, and every problem that was found is in `errors` with
    /// its line, not only the first. Nothing is added then.
    bool Read(const std::string &path, Toolchains &toolchains, std::vector<std::string> &errors) const;

    /// Reads the text of a file, as Read() does once it has it. `name` is
    /// what the file is called in messages.
    bool ReadText(
      const std::string &name,
      const std::string &text,
      Toolchains &toolchains,
      std::vector<std::string> &errors) const;
  };
} // neon

#endif //TOOLCHAIN_FILE_HPP
