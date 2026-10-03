#ifndef DISPLAY_OPTIONS_HPP
#define DISPLAY_OPTIONS_HPP

#include "command-line-options.hpp"

namespace neon
{
  /// The options about the window and the size of what is shown, owned by
  /// the runtime: they are what a player reaches for when a game comes up on
  /// a display it did not expect.
  ///
  ///   Display:
  ///   --window-size WxH      Size of the window in points
  ///   --window-mode MODE     windowed, borderless, or fullscreen
  ///   --ui-scale NUMBER      Makes the user interface larger or smaller
  class DisplayOptions final : public CommandLineOptions
  {
  public:
    void Register(CommandLine &command_line) override;

    bool Apply(const CommandLineContext &command_line, SettingsConfig &settings, std::string &error) override;
  };
} // neon

#endif //DISPLAY_OPTIONS_HPP
