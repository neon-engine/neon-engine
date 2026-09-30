#ifndef COMMAND_LINE_CONTEXT_HPP
#define COMMAND_LINE_CONTEXT_HPP

#include <string>

namespace neon
{
  /// What was asked for on the command line, for the parts of an application
  /// that want to know. Options are named without their dashes, so
  /// `--renderer` is `renderer`.
  class CommandLineContext
  {
  protected:
    ~CommandLineContext() = default;

  public:
    /// Whether the option was given on the command line.
    [[nodiscard]] virtual bool IsSet(const std::string &name) const = 0;

    /// The value of the option. When it was not given this is its default,
    /// which is empty unless the option was declared with one.
    [[nodiscard]] virtual std::string GetValue(const std::string &name) const = 0;

    /// The value of the option as a whole number. Returns false when the
    /// value is not one, and leaves `value` alone.
    [[nodiscard]] virtual bool GetInteger(const std::string &name, long &value) const = 0;

    /// The value of the option as a number that may have a fraction, written
    /// with a dot, such as `0.25`. Returns false when the value is not one,
    /// and leaves `value` alone.
    [[nodiscard]] virtual bool GetNumber(const std::string &name, double &value) const = 0;
  };
} // neon

#endif //COMMAND_LINE_CONTEXT_HPP
