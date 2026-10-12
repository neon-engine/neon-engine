#include "toolchain-run.hpp"

#include <format>
#include <vector>

#include "toolchain-placeholders.hpp"

namespace neon
{
  // Helpers of ToolchainRun: the words of a command line.
  namespace
  {
    /// Takes a line apart as a shell would: spaces and tabs separate the
    /// words, and a stretch in single or double quotes is one word, or part
    /// of one, without the quotes. Returns false for a quote that is not
    /// closed.
    bool split_words(const std::string &line, std::vector<std::string> &words, std::string &problem)
    {
      std::string word;
      bool in_word = false;
      char quote = 0;

      for (const char character : line)
      {
        if (quote != 0)
        {
          if (character == quote)
          {
            quote = 0;
          } else
          {
            word += character;
          }
          continue;
        }
        if (character == '\'' || character == '"')
        {
          quote = character;
          in_word = true;
          continue;
        }
        if (character == ' ' || character == '\t')
        {
          if (in_word)
          {
            words.push_back(word);
            word.clear();
            in_word = false;
          }
          continue;
        }
        word += character;
        in_word = true;
      }

      if (quote != 0)
      {
        problem = std::format("a quote is not closed in '{}'", line);
        return false;
      }
      if (in_word) { words.push_back(word); }
      return true;
    }

    const std::string &command_of(const Toolchain &toolchain, const ToolchainStep step)
    {
      switch (step)
      {
        case ToolchainStep::Configure:
        {
          return toolchain.configure;
        }
        case ToolchainStep::Build:
        {
          return toolchain.build;
        }
        case ToolchainStep::Run:
        {
          return toolchain.run;
        }
      }
      return toolchain.configure;
    }
  }

  ToolchainRun::ToolchainRun(ProcessContext *processes, const std::shared_ptr<Logger> &logger)
  {
    _processes = processes;
    _logger = logger;
  }

  bool ToolchainRun::Command(
    const Toolchain &toolchain,
    const ToolchainStep step,
    const BuildTarget &target,
    ProcessCommand &command,
    std::string &error) const
  {
    const std::string &line = command_of(toolchain, step);
    if (line.empty())
    {
      error = std::format(
        "the toolchain {} cannot {} here: its '{}' is off", toolchain.name, toolchain_step_name(step),
        toolchain_step_name(step));
      return false;
    }

    std::string filled;
    if (std::string problem; !ToolchainPlaceholders::For(toolchain, target).Fill(line, filled, problem))
    {
      error = std::format("'{}' of the toolchain {}: {}", toolchain_step_name(step), toolchain.name, problem);
      return false;
    }

    ProcessCommand made;
    made.directory = target.project;

    if (toolchain.kind == ToolchainKind::Docker)
    {
      // the whole line goes to the shell of the container, which takes it
      // apart there
      made.program = "docker";
      made.arguments = {"run", "--rm", "-v", target.project + ":" + std::string(ToolchainPlaceholders::docker_project)};
      if (!target.engine.empty())
      {
        made.arguments.push_back("-v");
        made.arguments.push_back(target.engine + ":" + std::string(ToolchainPlaceholders::docker_engine));
      }
      made.arguments.push_back("-w");
      made.arguments.emplace_back(ToolchainPlaceholders::docker_project);
      made.arguments.push_back(toolchain.image);
      made.arguments.push_back("sh");
      made.arguments.push_back("-c");
      made.arguments.push_back(filled);
    } else
    {
      std::vector<std::string> words;
      if (std::string problem; !split_words(filled, words, problem) || words.empty())
      {
        error = std::format(
          "'{}' of the toolchain {}: {}", toolchain_step_name(step), toolchain.name,
          words.empty() && problem.empty() ? "the command is empty" : problem);
        return false;
      }
      made.program = words.front();
      made.arguments.assign(words.begin() + 1, words.end());
    }

    command = made;
    return true;
  }

  bool ToolchainRun::Run(
    const Toolchain &toolchain,
    const ToolchainStep step,
    const BuildTarget &target,
    const LineCallback &on_line,
    int &exit_code,
    std::string &error) const
  {
    const std::string step_name(toolchain_step_name(step));
    ProcessCommand command;
    if (!Command(toolchain, step, target, command, error))
    {
      _logger->Error("The toolchain {} cannot {}: {}", toolchain.name, step_name, error);
      return false;
    }

    std::string line = command.program;
    for (const auto &argument : command.arguments) { line += " " + argument; }
    _logger->Info("Toolchain {} runs its {} on {} in {}: {}", toolchain.name, step_name, target.name, command.directory, line);

    if (!_processes->Run(command, on_line, exit_code, error))
    {
      _logger->Error("The toolchain {} could not {}: {}", toolchain.name, step_name, error);
      return false;
    }

    if (exit_code == 0)
    {
      _logger->Info("Toolchain {} finished the {} of {}", toolchain.name, step_name, target.name);
    } else
    {
      _logger->Error("Toolchain {} failed the {} of {} with exit code {}", toolchain.name, step_name, target.name, exit_code);
    }
    return true;
  }
} // neon
