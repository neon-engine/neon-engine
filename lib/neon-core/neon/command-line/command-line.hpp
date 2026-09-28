#ifndef COMMAND_LINE_HPP
#define COMMAND_LINE_HPP

#include <map>
#include <string>
#include <vector>

#include "command-line-context.hpp"

namespace neon
{
  /// One option an application understands.
  struct CommandLineOption
  {
    /// Without dashes. `renderer` is given as `--renderer`.
    std::string name;

    /// What the help text calls the value, such as `PATH`. An option without
    /// one is a switch: it takes no value and is either given or not.
    std::string value_name;

    std::string description;

    /// Heading the option is listed under in the help text. Options without
    /// one come first.
    std::string group;

    /// The values that are accepted. Empty accepts any.
    std::vector<std::string> allowed_values;

    /// The value when the option is not given.
    std::string default_value;
  };

  /// Reads the command line of an application.
  ///
  /// It knows nothing about any particular application. Each one declares the
  /// options it understands before parsing, usually through the sets in
  /// command-line-options.hpp. The runtime and the editor therefore share this
  /// class while accepting different options.
  ///
  /// An option is written as `--name` for a switch, and as `--name value` or
  /// `--name=value` for one that takes a value. `--help` is always understood.
  class CommandLine final : public CommandLineContext
  {
    std::string _program;
    std::string _summary;
    std::vector<CommandLineOption> _options;
    std::map<std::string, std::string> _values;
    std::string _error;

    [[nodiscard]] const CommandLineOption *Find(const std::string &name) const;

  public:
    /// `program` is the name the help text shows, and `summary` one line on
    /// what the program does.
    CommandLine(const std::string &program, const std::string &summary);

    /// Declares an option. Declaring a name twice replaces the earlier one,
    /// which lets an application adjust an option that a shared set declared.
    void Add(const CommandLineOption &option);

    /// Reads the arguments as they are handed to main(). Returns false when
    /// they cannot be understood, and GetError() says why.
    bool Parse(int argc, const char *const argv[]);

    /// Why Parse() failed, in words meant for the person at the keyboard.
    [[nodiscard]] const std::string &GetError() const;

    /// Whether `--help` was given. The application is expected to print
    /// GetHelp() and stop.
    [[nodiscard]] bool WantsHelp() const;

    /// The text that explains every declared option.
    [[nodiscard]] std::string GetHelp() const;

    [[nodiscard]] bool IsSet(const std::string &name) const override;

    [[nodiscard]] std::string GetValue(const std::string &name) const override;

    [[nodiscard]] bool GetInteger(const std::string &name, long &value) const override;
  };
} // neon

#endif //COMMAND_LINE_HPP
