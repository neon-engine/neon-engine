#ifndef TOOLCHAIN_PLACEHOLDERS_HPP
#define TOOLCHAIN_PLACEHOLDERS_HPP

#include <string>
#include <string_view>
#include <vector>

#include "build-target.hpp"
#include "toolchain.hpp"

namespace neon
{
  /// What the placeholders of a toolchain's commands stand for, and how
  /// they are filled in.
  ///
  /// A command of the file names its paths through placeholders, so that
  /// one file serves every project and every machine:
  ///
  /// | Placeholder  | Stands for |
  /// |---|---|
  /// | `{project}`  | The folder of the project |
  /// | `{build}`    | The build folder of the project for this toolchain, `build/<toolchain>` inside it |
  /// | `{name}`     | The name of the project, which is what the result is called |
  /// | `{engine}`   | The checkout of the engine the project is built against |
  /// | `{platform}` | The platform the toolchain builds for |
  ///
  /// For a native toolchain the paths are those of this machine. For a
  /// Docker one they are where ToolchainRun mounts the folders inside the
  /// container: the project at `/src`, the engine at `/engine`. A brace
  /// that is not one of these is a mistake, and the file is refused with
  /// it, see Check().
  struct ToolchainPlaceholders
  {
    std::string project;
    std::string build;
    std::string name;
    std::string engine;
    std::string platform;

    /// Where a Docker toolchain finds the folders inside its container.
    static constexpr std::string_view docker_project = "/src";
    static constexpr std::string_view docker_engine = "/engine";

    /// The names of the placeholders, for a message.
    [[nodiscard]] static const std::vector<std::string> &Names();

    /// Whether every brace in the text opens a placeholder that is known
    /// and closes it. Says what is wrong in `problem` otherwise.
    [[nodiscard]] static bool Check(const std::string &text, std::string &problem);

    /// The placeholders of a toolchain run on a target: the paths of this
    /// machine for a native one, those inside the container for a Docker
    /// one.
    [[nodiscard]] static ToolchainPlaceholders For(const Toolchain &toolchain, const BuildTarget &target);

    /// Writes the text with every placeholder filled in. Returns false,
    /// and says what is wrong in `problem`, for a brace that Check()
    /// refuses.
    [[nodiscard]] bool Fill(const std::string &text, std::string &filled, std::string &problem) const;
  };
} // neon

#endif //TOOLCHAIN_PLACEHOLDERS_HPP
