#ifndef FAKE_PROCESS_CONTEXT_HPP
#define FAKE_PROCESS_CONTEXT_HPP

#include <algorithm>
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

    /// The commands that were started without waiting, in order.
    std::vector<ProcessCommand> started;

    /// Makes every start fail, as a program that cannot be run does.
    bool refuses_start = false;

    /// Marks a program as installed, at a made-up path.
    void Install(const std::string &program)
    {
      installed[program] = "/fake/bin/" + program;
    }

    /// Whether a program is installed, by its name or by where it is, as
    /// a real runner starts the path it found.
    [[nodiscard]] bool IsInstalled(const std::string &program) const
    {
      if (installed.contains(program)) { return true; }
      return std::ranges::any_of(installed, [&program](const auto &entry) { return entry.second == program; });
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

      if (!IsInstalled(command.program))
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

    bool Start(const ProcessCommand &command, std::string &error) override
    {
      started.push_back(command);

      if (!IsInstalled(command.program))
      {
        error = command.program + " is not installed";
        return false;
      }
      if (refuses_start)
      {
        error = command.program + " could not be started";
        return false;
      }
      return true;
    }
  };
} // neon::testing

#endif //FAKE_PROCESS_CONTEXT_HPP
