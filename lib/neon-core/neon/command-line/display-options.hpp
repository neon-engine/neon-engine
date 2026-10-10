#ifndef DISPLAY_OPTIONS_HPP
#define DISPLAY_OPTIONS_HPP

#include <string>
#include <vector>

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
  ///   --quality NAME         low, medium, high, ultra, or custom
  ///   --ui-scale NUMBER      Makes the user interface larger or smaller
  ///
  /// `--quality` names a preset of the table the settings hold, the
  /// project's, and the preset holds: an option it decides, such as
  /// `--anisotropy`, is left out with a warning while one does, and applies
  /// with `--quality custom`, or when no layer before chose a preset.
  class DisplayOptions final : public CommandLineOptions
  {
    std::vector<std::string> _warnings;

    bool Apply(const CommandLineContext &command_line, SettingsConfig &settings, std::string &error, bool with_quality);

  public:
    void Register(CommandLine &command_line) override;

    /// Applies every option, the preset first.
    bool Apply(const CommandLineContext &command_line, SettingsConfig &settings, std::string &error) override;

    /// Applies every option but `--quality`, for the pass before the
    /// project is read: the preset it names may be one the project
    /// defines, which is not known yet.
    bool ApplyExceptTheQuality(const CommandLineContext &command_line, SettingsConfig &settings, std::string &error);

    /// What the last Apply() left out and why: an option a preset decides,
    /// written next to a preset. For the log, which is not open when the
    /// options are applied.
    [[nodiscard]] const std::vector<std::string> &GetWarnings() const { return _warnings; }
  };
} // neon

#endif //DISPLAY_OPTIONS_HPP
