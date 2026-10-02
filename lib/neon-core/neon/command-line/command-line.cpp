#include "command-line.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <string_view>

namespace neon
{
  // Helpers of CommandLineOption, for this file alone.
  namespace
  {
    constexpr std::string_view help_option = "help";

    std::string join(const std::vector<std::string> &values, const std::string_view separator)
    {
      std::string joined;
      for (const auto &value : values)
      {
        if (!joined.empty()) { joined += separator; }
        joined += value;
      }
      return joined;
    }

    /// How an option appears in the first column of the help text.
    std::string usage_of(const CommandLineOption &option)
    {
      std::string usage = "--" + option.name;
      if (option.value_name.empty()) { return usage; }

      // the accepted values say more than a placeholder does
      usage += " ";
      usage += option.allowed_values.empty() ? option.value_name : join(option.allowed_values, "|");
      return usage;
    }
  }

  CommandLine::CommandLine(const std::string &program, const std::string &summary)
  {
    _program = program;
    _summary = summary;

    Add({.name = std::string(help_option), .description = "Show this text"});
  }

  const CommandLineOption *CommandLine::Find(const std::string &name) const
  {
    const auto found = std::ranges::find_if(_options, [&name](const CommandLineOption &option)
    {
      return option.name == name;
    });
    return found == _options.end() ? nullptr : &*found;
  }

  void CommandLine::Add(const CommandLineOption &option)
  {
    const auto existing = std::ranges::find_if(_options, [&option](const CommandLineOption &candidate)
    {
      return candidate.name == option.name;
    });

    if (existing != _options.end())
    {
      *existing = option;
    } else
    {
      _options.push_back(option);
    }
  }

  bool CommandLine::Parse(const int argc, const char *const argv[])
  {
    _values.clear();
    _error.clear();

    // the first argument is the program itself
    for (int i = 1; i < argc; i++)
    {
      const std::string_view argument = argv[i];

      if (!argument.starts_with("--") || argument.size() == 2)
      {
        _error = "'" + std::string(argument) + "' is not an option. Options start with two dashes";
        return false;
      }

      // --name=value carries its value along, --name value has it follow
      const size_t equals = argument.find('=');
      const bool has_inline_value = equals != std::string_view::npos;
      const std::string name(argument.substr(2, has_inline_value ? equals - 2 : std::string_view::npos));

      const CommandLineOption *option = Find(name);
      if (option == nullptr)
      {
        _error = "Unknown option '--" + name + "'";
        return false;
      }

      if (_values.contains(name))
      {
        _error = "Option '--" + name + "' was given more than once";
        return false;
      }

      if (option->value_name.empty())
      {
        if (has_inline_value)
        {
          _error = "Option '--" + name + "' takes no value";
          return false;
        }
        _values[name] = "";
        continue;
      }

      std::string value;
      if (has_inline_value)
      {
        value = argument.substr(equals + 1);
      } else if (i + 1 < argc && !std::string_view(argv[i + 1]).starts_with("--"))
      {
        value = argv[++i];
      } else
      {
        _error = "Option '--" + name + "' needs a value";
        return false;
      }

      if (value.empty())
      {
        _error = "Option '--" + name + "' needs a value";
        return false;
      }

      if (!option->allowed_values.empty() &&
          std::ranges::find(option->allowed_values, value) == option->allowed_values.end())
      {
        _error = "'" + value + "' is not a value of '--" + name + "'. It accepts " +
                 join(option->allowed_values, ", ");
        return false;
      }

      _values[name] = value;
    }

    return true;
  }

  const std::string &CommandLine::GetError() const
  {
    return _error;
  }

  bool CommandLine::WantsHelp() const
  {
    return IsSet(std::string(help_option));
  }

  std::string CommandLine::GetHelp() const
  {
    // headings in the order their first option was declared, with the
    // options that have none ahead of the rest
    std::vector<std::string> groups{""};
    size_t widest = 0;

    for (const auto &option : _options)
    {
      if (std::ranges::find(groups, option.group) == groups.end()) { groups.push_back(option.group); }
      widest = std::max(widest, usage_of(option).size());
    }

    std::string help = _summary.empty() ? "" : _summary + "\n\n";
    help += "Usage: " + _program + " [options]\n";

    for (const auto &group : groups)
    {
      help += "\n";
      if (!group.empty()) { help += group + ":\n"; }

      for (const auto &option : _options)
      {
        if (option.group != group) { continue; }

        const std::string usage = usage_of(option);
        help += "  " + usage + std::string(widest - usage.size() + 2, ' ') + option.description;
        if (!option.default_value.empty()) { help += ". Default: " + option.default_value; }
        help += "\n";
      }
    }

    return help;
  }

  bool CommandLine::IsSet(const std::string &name) const
  {
    return _values.contains(name);
  }

  std::string CommandLine::GetValue(const std::string &name) const
  {
    if (const auto found = _values.find(name); found != _values.end()) { return found->second; }

    const CommandLineOption *option = Find(name);
    return option == nullptr ? "" : option->default_value;
  }

  bool CommandLine::GetInteger(const std::string &name, long &value) const
  {
    const std::string text = GetValue(name);
    if (text.empty()) { return false; }

    long parsed = 0;
    const char *end = text.data() + text.size();
    const auto [stopped_at, error] = std::from_chars(text.data(), end, parsed);

    // the whole value has to be the number, "12abc" is not one
    if (error != std::errc{} || stopped_at != end) { return false; }

    value = parsed;
    return true;
  }

  bool CommandLine::GetNumber(const std::string &name, double &value) const
  {
    const std::string text = GetValue(name);

    // Read by hand. The functions of the standard library either follow the
    // language of the machine, where the dot may be a comma, or are missing
    // from one of the compilers the engine is built with.
    double whole = 0.0;
    double fraction = 0.0;
    double scale = 1.0;
    size_t digits = 0;
    size_t position = 0;
    const bool negative = text.starts_with('-');
    if (negative) { position++; }

    for (; position < text.size() && text[position] >= '0' && text[position] <= '9'; position++, digits++)
    {
      whole = whole * 10.0 + (text[position] - '0');
    }

    if (position < text.size() && text[position] == '.')
    {
      position++;
      for (; position < text.size() && text[position] >= '0' && text[position] <= '9'; position++, digits++)
      {
        scale *= 10.0;
        fraction = fraction * 10.0 + (text[position] - '0');
      }
    }

    // the whole value has to be the number, and "." or "-" is not one
    if (digits == 0 || position != text.size()) { return false; }

    const double parsed = whole + fraction / scale;
    if (!std::isfinite(parsed)) { return false; }

    value = negative ? -parsed : parsed;
    return true;
  }
} // neon
