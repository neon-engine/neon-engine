#include "ui-call.hpp"

#include <cctype>
#include <charconv>
#include <format>

namespace neon
{
  // What reads a call, for this file alone.
  namespace
  {
    bool is_name_start(const char c)
    {
      return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
    }

    bool is_name_part(const char c)
    {
      return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
    }

    void skip_spaces(const std::string_view text, std::size_t &at)
    {
      while (at < text.size() && std::isspace(static_cast<unsigned char>(text[at])) != 0) { at++; }
    }

    /// A name from `at` on, which is moved past it. Empty when there is
    /// none.
    std::string_view read_name(const std::string_view text, std::size_t &at)
    {
      const std::size_t start = at;
      if (at < text.size() && is_name_start(text[at]))
      {
        while (at < text.size() && is_name_part(text[at])) { at++; }
      }
      return text.substr(start, at - start);
    }

    /// One argument from `at` on, which is moved past it.
    bool read_argument(const std::string_view text, std::size_t &at, UiCallArgument &argument, std::string &problem)
    {
      const char first = text[at];

      if (first == '\'' || first == '"')
      {
        const std::size_t end = text.find(first, at + 1);
        if (end == std::string_view::npos)
        {
          problem = std::format("the text that starts at {} is not closed with {}", text.substr(at), first);
          return false;
        }

        argument.kind = UiCallArgument::Kind::Text;
        argument.text = std::string(text.substr(at + 1, end - at - 1));
        at = end + 1;
        return true;
      }

      if (first == '$')
      {
        at++;
        if (read_name(text, at) != "event")
        {
          problem = "$event is the only name that starts with $";
          return false;
        }

        argument.kind = UiCallArgument::Kind::Event;
        return true;
      }

      if (std::isdigit(static_cast<unsigned char>(first)) != 0 || first == '-' || first == '.')
      {
        std::size_t end = at + 1;
        while (end < text.size() && (std::isdigit(static_cast<unsigned char>(text[end])) != 0 || text[end] == '.')) { end++; }

        double number = 0.0;
        const char *last = text.data() + end;
        const auto [stopped, error] = std::from_chars(text.data() + at, last, number);
        if (error != std::errc{} || stopped != last)
        {
          problem = std::format("'{}' is not a number", text.substr(at, end - at));
          return false;
        }

        argument.kind = UiCallArgument::Kind::Number;
        argument.number = number;
        at = end;
        return true;
      }

      const std::string_view name = read_name(text, at);
      if (name.empty())
      {
        problem = std::format(
          "'{}' is no argument: a number, a text between quotes, true, false, the name of a value, or $event",
          text.substr(at));
        return false;
      }

      if (name == "true" || name == "false")
      {
        argument.kind = UiCallArgument::Kind::Flag;
        argument.flag = name == "true";
        return true;
      }

      argument.kind = UiCallArgument::Kind::Value;
      argument.text = std::string(name);
      return true;
    }
  }

  bool UiCall::Parse(const std::string_view text, UiCall &call, std::string &problem)
  {
    call = {};
    UiCall read;

    std::size_t at = 0;
    skip_spaces(text, at);
    read.function = std::string(read_name(text, at));
    if (read.function.empty())
    {
      problem = "it has to start with the name of a function: letters, digits, and _, and no digit first";
      return false;
    }

    skip_spaces(text, at);
    if (at == text.size())
    {
      call = read;
      return true;
    }

    if (text[at] != '(')
    {
      problem = std::format("after {} comes '{}', where ( or nothing was expected", read.function, text.substr(at));
      return false;
    }
    at++;

    skip_spaces(text, at);
    while (at < text.size() && text[at] != ')')
    {
      UiCallArgument argument;
      if (!read_argument(text, at, argument, problem)) { return false; }
      read.arguments.push_back(argument);

      skip_spaces(text, at);
      if (at < text.size() && text[at] == ',')
      {
        at++;
        skip_spaces(text, at);
        if (at == text.size() || text[at] == ')')
        {
          problem = "an argument is missing after the last ,";
          return false;
        }
      } else if (at < text.size() && text[at] != ')')
      {
        problem = std::format("'{}' comes where , or ) was expected", text.substr(at));
        return false;
      }
    }

    if (at == text.size())
    {
      problem = "the ( is not closed with )";
      return false;
    }
    at++;

    skip_spaces(text, at);
    if (at != text.size())
    {
      problem = std::format("'{}' comes after the ), where nothing was expected", text.substr(at));
      return false;
    }

    call = read;
    return true;
  }

  std::string UiCall::AsText() const
  {
    if (function.empty()) { return ""; }
    if (arguments.empty()) { return function; }

    std::string text = function + "(";
    for (std::size_t i = 0; i < arguments.size(); i++)
    {
      if (i > 0) { text += ", "; }

      const UiCallArgument &argument = arguments[i];
      switch (argument.kind)
      {
        case UiCallArgument::Kind::Number: text += std::format("{}", argument.number);
          break;
        case UiCallArgument::Kind::Text:
          // in the quotes the text does not hold itself
          text += argument.text.find('\'') == std::string::npos ? "'" + argument.text + "'" : "\"" + argument.text + "\"";
          break;
        case UiCallArgument::Kind::Flag: text += argument.flag ? "true" : "false";
          break;
        case UiCallArgument::Kind::Value: text += argument.text;
          break;
        case UiCallArgument::Kind::Event: text += "$event";
          break;
        case UiCallArgument::Kind::Nothing: text += "nothing";
          break;
      }
    }
    return text + ")";
  }
} // neon
