#ifndef TOOLCHAIN_CHECK_HPP
#define TOOLCHAIN_CHECK_HPP

#include <memory>
#include <string>
#include <vector>

#include <neon/logging/logger.hpp>
#include <neon/process/process-context.hpp>

namespace neon
{
  struct Toolchain;

  /// Whether a toolchain is usable on this machine: every program it
  /// requires is installed, and the image of a Docker toolchain is built.
  ///
  /// The editor checks every declared toolchain when it starts, and shows
  /// which are usable. A name of `requires` is looked for on PATH, a path
  /// has to exist, as ProcessContext::Find does. A Docker toolchain needs
  /// `docker` whether or not it lists it, and its image is asked of
  /// `docker image inspect`, which ends with 0 when the image is there.
  class ToolchainCheck final
  {
    ProcessContext *_processes;
    std::shared_ptr<Logger> _logger;

  public:
    ToolchainCheck(ProcessContext *processes, const std::shared_ptr<Logger> &logger);

    /// Returns true when the toolchain is usable. Otherwise `missing` says
    /// what is not there, one line each, for the editor to show.
    bool Check(const Toolchain &toolchain, std::vector<std::string> &missing) const;
  };
} // neon

#endif //TOOLCHAIN_CHECK_HPP
