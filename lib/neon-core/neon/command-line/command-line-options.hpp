#ifndef COMMAND_LINE_OPTIONS_HPP
#define COMMAND_LINE_OPTIONS_HPP

#include <string>
#include <neon/runtime/settings-config.hpp>

#include "command-line.hpp"

namespace neon
{
  /// A set of options that belong together, and what they mean.
  ///
  /// An application picks the sets it wants to offer. The runtime offers the
  /// runtime set. The editor offers that one and a set of its own, which the
  /// runtime never sees.
  class CommandLineOptions
  {
  protected:
    ~CommandLineOptions() = default;

  public:
    /// Declares the options of the set. Called before parsing.
    virtual void Register(CommandLine &command_line) = 0;

    /// Carries what was parsed over into the settings. Called after parsing.
    /// Returns false when the options do not make sense together, and says
    /// why in `error`.
    virtual bool Apply(const CommandLineContext &command_line, SettingsConfig &settings, std::string &error) = 0;
  };
} // neon

#endif //COMMAND_LINE_OPTIONS_HPP
