#ifndef OS_PROCESS_RUNNER_HPP
#define OS_PROCESS_RUNNER_HPP

#include <memory>
#include <string>

#include <neon/logging/logger.hpp>
#include <neon/process/process-context.hpp>

namespace neon
{
  /// The processes of the operating system: `posix_spawn` on macOS and
  /// Linux, `CreateProcess` on Windows. The platform is chosen when the
  /// backend is compiled, so main.cpp creates it as it creates the others.
  ///
  /// The standard output and the standard error of a process are read
  /// through one pipe, so that what a build prints comes in the order it
  /// was printed, errors among the rest. The process inherits the
  /// environment of the application, which is where PATH comes from.
  ///
  /// A process that is started without waiting prints nowhere, and is
  /// waited for on a thread of its own once it ends, so that it leaves
  /// nothing behind on macOS and Linux. On Windows a `.cmd` or `.bat` is a
  /// script of the command interpreter, which is started to run it.
  // ReSharper disable once CppInconsistentNaming
  class OS_ProcessRunner final : public ProcessContext
  {
    std::shared_ptr<Logger> _logger;

  public:
    explicit OS_ProcessRunner(const std::shared_ptr<Logger> &logger)
      : _logger(logger) {}

    bool Find(const std::string &program, std::string &native_path) override;

    bool Run(
      const ProcessCommand &command,
      const LineCallback &on_line,
      int &exit_code,
      std::string &error) override;

    bool Start(const ProcessCommand &command, std::string &error) override;
  };
} // neon

#endif //OS_PROCESS_RUNNER_HPP
