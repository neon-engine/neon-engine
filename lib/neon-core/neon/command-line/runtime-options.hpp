#ifndef RUNTIME_OPTIONS_HPP
#define RUNTIME_OPTIONS_HPP

#include "command-line-options.hpp"

namespace neon
{
  /// The options the runtime owns, with DisplayOptions: how to draw.
  ///
  ///   --renderer NAME         Renderer to draw with
  ///   --vulkan-version 1.N    Highest version of Vulkan to render with
  ///
  /// What the editor owns, starting another scene, running without a
  /// window, screenshots, is EditorOptions. See docs/command-line.md.
  class RuntimeOptions final : public CommandLineOptions
  {
  public:
    void Register(CommandLine &command_line) override;

    bool Apply(const CommandLineContext &command_line, SettingsConfig &settings, std::string &error) override;
  };
} // neon

#endif //RUNTIME_OPTIONS_HPP
