#ifndef RUNTIME_OPTIONS_HPP
#define RUNTIME_OPTIONS_HPP

#include "command-line-options.hpp"

namespace neon
{
  /// The options every runtime understands, whatever it is built into.
  ///
  ///   --renderer NAME     Renderer to draw with
  ///
  ///   Development:
  ///   --frames N          Stop after N frames
  ///   --screenshot PATH   Save the last frame as a PNG image before stopping
  ///   --headless          Run without a window
  class RuntimeOptions final : public CommandLineOptions
  {
  public:
    void Register(CommandLine &command_line) override;

    bool Apply(const CommandLineContext &command_line, SettingsConfig &settings, std::string &error) override;
  };
} // neon

#endif //RUNTIME_OPTIONS_HPP
