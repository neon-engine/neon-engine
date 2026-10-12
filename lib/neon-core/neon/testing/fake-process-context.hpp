#ifndef FAKE_PROCESS_CONTEXT_HPP
#define FAKE_PROCESS_CONTEXT_HPP

#include <map>
#include <set>
#include <string>
#include <vector>

#include <neon/process/process-context.hpp>

namespace neon::testing
{
  /// What a fake process prints and ends with, under the name of its
  /// program.
  struct FakeProcessResult
  {
    std::vector<std::string> lines;
    int exit_code = 0;
  };

  /// Processes that a test decides: which programs are installed, and what
  /// each one prints and ends with when it is run. Nothing is started.
  ///
  /// A program that is not installed cannot be run, as on a real machine.
  /// Every command that was run is kept, in order, so that a test can look
  /// at what would have been started.
  class FakeProcessContext final : public ProcessContext
  {
  public:
    /// The programs that are found, by name or native path, and where each
    /// one is.
    std::map<std::string, std::string> installed;

    /// What a program does when it is run, by its name. A program that is
    /// installed and has no result prints nothing and ends with 0.
    std::map<std::string, FakeProcessResult> results;

    /// The commands that were run, in order.
    std::vector<ProcessCommand> commands;

    /// Marks a program as installed, at a made-up path.
    void Install(const std::string &program)
    {
      installed[program] = "/fake/bin/" + program;
    }

    bool Find(const std::string &program, std::string &native_path) override
    {
      const auto found = installed.find(program);
      if (found == installed.end()) { return false; }

      native_path = found->second;
      return true;
    }

    bool Run(
      const ProcessCommand &command,
      const LineCallback &on_line,
      int &exit_code,
      std::string &error) override
    {
      commands.push_back(command);

      if (!installed.contains(command.program))
      {
        error = command.program + " is not installed";
        return false;
      }

      const auto result = results.find(command.program);
      if (result == results.end())
      {
        exit_code = 0;
        return true;
      }

      for (const auto &line : result->second.lines) { on_line(line); }
      exit_code = result->second.exit_code;
      return true;
    }
  };
} // neon::testing

#endif //FAKE_PROCESS_CONTEXT_HPP
