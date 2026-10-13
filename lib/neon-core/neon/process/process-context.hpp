#ifndef PROCESS_CONTEXT_HPP
#define PROCESS_CONTEXT_HPP

#include <functional>
#include <string>

#include "process-command.hpp"

namespace neon
{
  /// What the rest of the engine sees of the processes of the operating
  /// system: whether a program is installed, and starting one and reading
  /// what it prints until it ends.
  ///
  /// The editor builds a project with it, through the programs of a
  /// toolchain: cmake, ninja, docker, and the result, and opens the code of
  /// a project in another program, which goes on by itself. The runtime
  /// starts no process. How a process is started is the backend's:
  /// `posix_spawn` on macOS and Linux, `CreateProcess` on Windows.
  class ProcessContext
  {
  protected:
    ~ProcessContext() = default;

  public:
    /// What is done with every line a process prints.
    using LineCallback = std::function<void(const std::string &line)>;

    /// Finds a program: a name is looked for on the PATH of the
    /// environment, as a shell does, and a native path has to exist and be
    /// a program. Returns false when there is none, and leaves `native_path`
    /// as it was.
    virtual bool Find(const std::string &program, std::string &native_path) = 0;

    /// Starts the command and waits until it ends. Every line it prints, on
    /// its standard output or its standard error, is handed to `on_line` as
    /// it comes, in the order it was printed and without its line break.
    /// Returns false when the program could not be started, and says why
    /// in `error`. Otherwise `exit_code` is what the program ended with: on
    /// macOS and Linux a program that was ended by a signal has 128 plus
    /// the signal, as a shell reports it.
    virtual bool Run(
      const ProcessCommand &command,
      const LineCallback &on_line,
      int &exit_code,
      std::string &error) = 0;

    /// Starts the command and returns at once: the process is not waited
    /// for, and what it prints goes nowhere. For a program that is opened
    /// for a person, such as the editor of the code, which goes on after
    /// the call and after the application. Returns false when the program
    /// could not be started, and says why in `error`.
    virtual bool Start(const ProcessCommand &command, std::string &error) = 0;
  };
} // neon

#endif //PROCESS_CONTEXT_HPP
