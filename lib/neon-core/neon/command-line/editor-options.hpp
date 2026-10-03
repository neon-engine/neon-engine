#ifndef EDITOR_OPTIONS_HPP
#define EDITOR_OPTIONS_HPP

#include "command-line-options.hpp"

namespace neon
{
  /// The set the editor owns: starting another scene, running without a
  /// window, screenshots, input from a script. Everything that loads other
  /// content or looks inside a game. The runtime owns RuntimeOptions and
  /// DisplayOptions; NeonRuntime registers this set too until NeonEditor
  /// exists, see docs/command-line.md (#143).
  ///
  ///   Editor:
  ///   --scene PATH              Scene to start with, in place of the entry scene
  ///   --ui PATH                 User interface to show on top
  ///   --frames N                Stop after N frames
  ///   --screenshot PATH         Save the last frame as a PNG image
  ///   --screenshot-at N[,N...]  Save these frames instead of the last one
  ///   --output-dir DIR          Folder of this machine that output:// stands for
  ///   --time-step SECONDS       Advance the game by this much in every frame
  ///   --headless-renderer       Render without a window
  ///   --render-scale NUMBER     Pixels for each point, without a window
  ///   --input SCRIPT            Input in place of devices, without a window
  ///   --input-script PATH       The same, from a file
  ///   --spawn PATH              Spawn this prefab once the scene is read
  ///   --headless                Run as a dedicated server. Refused until there is one (#144)
  class EditorOptions final : public CommandLineOptions
  {
  public:
    void Register(CommandLine &command_line) override;

    bool Apply(const CommandLineContext &command_line, SettingsConfig &settings, std::string &error) override;
  };
} // neon

#endif //EDITOR_OPTIONS_HPP
