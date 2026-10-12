#ifndef TOOLCHAIN_HPP
#define TOOLCHAIN_HPP

#include <string>
#include <vector>

#include "toolchain-kind.hpp"

namespace neon
{
  /// One way of building a project and running the result, as the editor's
  /// toolchain files declare it under a name of its own, see ToolchainFile.
  ///
  /// The three commands are lines as one would type them, with the
  /// placeholders of ToolchainPlaceholders where the project, its build
  /// folder, its name, the engine, or the platform go. A native toolchain
  /// runs them on this machine, a Docker one inside a container of `image`
  /// with the project mounted, see ToolchainRun.
  struct Toolchain
  {
    /// What the toolchain is called in the file and in the editor, a plain
    /// name such as `macos-arm64`.
    std::string name;

    ToolchainKind kind = ToolchainKind::Native;

    /// What it builds for, in the engine's naming of a platform:
    /// `darwin-arm64`, `linux-x86_64`, `windows-x86_64`.
    std::string platform;

    /// The Docker image of a Docker toolchain, such as
    /// `neon-engine/linux-x64`. Empty for a native one.
    std::string image;

    /// The programs that have to be installed for the toolchain to be
    /// usable, each a name that is looked for on PATH or a native path,
    /// see ToolchainCheck.
    std::vector<std::string> required;

    std::string configure;
    std::string build;

    /// How the result is started. Empty when it cannot be run on this
    /// machine, which the file writes as `off`.
    std::string run;

    [[nodiscard]] bool CanRun() const
    {
      return !run.empty();
    }
  };
} // neon

#endif //TOOLCHAIN_HPP
