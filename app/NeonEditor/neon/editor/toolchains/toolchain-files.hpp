#ifndef TOOLCHAIN_FILES_HPP
#define TOOLCHAIN_FILES_HPP

#include <memory>
#include <string>
#include <string_view>

#include <neon/filesystem/file-system-context.hpp>
#include <neon/logging/logger.hpp>

#include "toolchain-file.hpp"
#include "toolchains.hpp"

namespace neon
{
  /// The toolchain files of this machine, read over the engine's defaults
  /// into one Toolchains: `editor.yml`, then `user-editor.yml`, each laid
  /// over the one before, both in the editor's own folder. The toolchains
  /// and the editors are the editor's settings, not a project's: one
  /// machine configures them once, and every project is built and opened
  /// with them. A file that is not there is skipped; one with a mistake
  /// adds nothing, and every problem in it is logged with its line, where
  /// the log window shows it.
  class ToolchainFiles final
  {
    const ToolchainFile *_file;
    FileSystemContext *_file_system;
    std::shared_ptr<Logger> _logger;

    /// Reads one file over `toolchains` when it is there.
    bool ReadOne(const std::string &path, Toolchains &toolchains) const;

  public:
    /// Where the files are: the editor's own folder, `editor://`, the same
    /// whichever project is open, see docs/editor.md#the-editors-folder.
    static constexpr std::string_view shared_path = "editor://editor.yml";
    static constexpr std::string_view user_path = ToolchainFile::user_path;

    ToolchainFiles(const ToolchainFile *file, FileSystemContext *file_system, const std::shared_ptr<Logger> &logger);

    /// Sets `toolchains` to the defaults with the files of this machine
    /// laid over them. Returns false when a file had a mistake, which the
    /// log says; the defaults and the other file are there all the same.
    bool Read(Toolchains &toolchains) const;
  };
} // neon

#endif //TOOLCHAIN_FILES_HPP
