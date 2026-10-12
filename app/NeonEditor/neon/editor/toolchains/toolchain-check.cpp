#include "toolchain-check.hpp"

#include <algorithm>
#include <format>

#include "toolchain.hpp"

namespace neon
{
  // Helpers of ToolchainCheck, for this file alone.
  namespace
  {
    const std::string docker = "docker";

    bool is_path(const std::string &program)
    {
      return program.find('/') != std::string::npos || program.find('\\') != std::string::npos;
    }
  }

  ToolchainCheck::ToolchainCheck(ProcessContext *processes, const std::shared_ptr<Logger> &logger)
  {
    _processes = processes;
    _logger = logger;
  }

  bool ToolchainCheck::Check(const Toolchain &toolchain, std::vector<std::string> &missing) const
  {
    const std::size_t before = missing.size();

    std::vector<std::string> programs = toolchain.required;
    if (toolchain.kind == ToolchainKind::Docker && std::ranges::find(programs, docker) == programs.end())
    {
      programs.push_back(docker);
    }

    for (const auto &program : programs)
    {
      if (std::string found; _processes->Find(program, found))
      {
        _logger->Debug("Toolchain {} finds {} at {}", toolchain.name, program, found);
        continue;
      }
      missing.push_back(is_path(program)
                          ? std::format("{} is not there", program)
                          : std::format("{} is not on PATH", program));
    }

    // the image is asked of docker, when docker is there to ask
    if (toolchain.kind == ToolchainKind::Docker && missing.size() == before)
    {
      const ProcessCommand inspect{.program = docker, .arguments = {"image", "inspect", toolchain.image}};
      int exit_code = 0;
      std::string error;
      if (!_processes->Run(inspect, [](const std::string &) {}, exit_code, error))
      {
        missing.push_back(std::format("docker could not be asked for the image {}: {}", toolchain.image, error));
      } else if (exit_code != 0)
      {
        missing.push_back(std::format("the Docker image {} is not built", toolchain.image));
      }
    }

    if (missing.size() > before)
    {
      for (std::size_t i = before; i < missing.size(); i++)
      {
        _logger->Info("Toolchain {} is not usable: {}", toolchain.name, missing[i]);
      }
      return false;
    }

    _logger->Info("Toolchain {} is usable, for {}", toolchain.name, toolchain.platform);
    return true;
  }
} // neon
