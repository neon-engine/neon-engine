#ifndef DISPLAY_OPTIONS_HPP
#define DISPLAY_OPTIONS_HPP

#include "command-line-options.hpp"

namespace neon
{
  /// The options about the size of what is shown, and about input that is
  /// written down. They make the sizes and densities of displays testable
  /// on a machine that has neither.
  ///
  ///   Display:
  ///   --window-size WxH      Size of the window in points
  ///   --render-scale NUMBER  Pixels for each point, without a window
  ///   --ui-scale NUMBER      Makes the user interface larger or smaller
  ///
  ///   Development:
  ///   --input SCRIPT         Input in place of devices, without a window
  ///   --input-script PATH    The same, from a file
  class DisplayOptions final : public CommandLineOptions
  {
  public:
    void Register(CommandLine &command_line) override;

    bool Apply(const CommandLineContext &command_line, SettingsConfig &settings, std::string &error) override;
  };
} // neon

#endif //DISPLAY_OPTIONS_HPP
