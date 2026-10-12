#ifndef TOOLCHAIN_RUN_HPP
#define TOOLCHAIN_RUN_HPP

#include <memory>
#include <string>

#include <neon/logging/logger.hpp>
#include <neon/process/process-context.hpp>

#include "build-target.hpp"
#include "toolchain-step.hpp"
#include "toolchain.hpp"

namespace neon
{
  /// Runs one step of a toolchain on a project: configures it, builds it,
  /// or starts what was built, as a process of its own, and hands every
  /// line it prints on as it comes.
  ///
  /// The command of the step has its placeholders filled in, see
  /// ToolchainPlaceholders, and is taken apart into words as a shell would,
  /// spaces separating them and quotes holding them together. A native
  /// toolchain starts the first word as the program, in the folder of the
  /// project. A Docker toolchain starts the whole line in a shell inside a
  /// container of its image, with the project mounted at `/src` and the
  /// engine at `/engine`:
  ///
  ///     docker run --rm -v <project>:/src -v <engine>:/engine -w /src <image> sh -c '<command>'
  ///
  /// The call returns when the process has ended. The editor runs it off
  /// its own thread, so that it stays usable while a build runs.
  class ToolchainRun final
  {
    ProcessContext *_processes;
    std::shared_ptr<Logger> _logger;

  public:
    using LineCallback = ProcessContext::LineCallback;

    ToolchainRun(ProcessContext *processes, const std::shared_ptr<Logger> &logger);

    /// The command a step would start, for a log or a test. Returns false
    /// when the step cannot be run, with the reason in `error`: the
    /// toolchain has no `run`, or the command cannot be taken apart.
    [[nodiscard]] bool Command(
      const Toolchain &toolchain,
      ToolchainStep step,
      const BuildTarget &target,
      ProcessCommand &command,
      std::string &error) const;

    /// Runs the step and waits until it has ended. Returns false when it
    /// could not be started, and says why in `error`. Otherwise
    /// `exit_code` is what the step ended with, 0 when it succeeded.
    bool Run(
      const Toolchain &toolchain,
      ToolchainStep step,
      const BuildTarget &target,
      const LineCallback &on_line,
      int &exit_code,
      std::string &error) const;
  };
} // neon

#endif //TOOLCHAIN_RUN_HPP
