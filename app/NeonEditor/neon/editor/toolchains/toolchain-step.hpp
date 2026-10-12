#ifndef TOOLCHAIN_STEP_HPP
#define TOOLCHAIN_STEP_HPP

#include <string_view>

namespace neon
{
  /// The three things a toolchain does with a project, each a command of
  /// its own in the file. Building and running are independent: a build
  /// need not be run, and what was built last can be run again.
  enum class ToolchainStep
  {
    Configure,
    Build,
    Run
  };

  /// The name of a step as it is written in the file.
  [[nodiscard]] inline std::string_view toolchain_step_name(const ToolchainStep step)
  {
    switch (step)
    {
      case ToolchainStep::Configure:
      {
        return "configure";
      }
      case ToolchainStep::Build:
      {
        return "build";
      }
      case ToolchainStep::Run:
      {
        return "run";
      }
    }
    return "configure";
  }
} // neon

#endif //TOOLCHAIN_STEP_HPP
