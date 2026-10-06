#ifndef UI_CALL_ARGUMENT_HPP
#define UI_CALL_ARGUMENT_HPP

#include <string>

namespace neon
{
  /// One of the arguments a file writes for a function it names, as the
  /// `'safe'` and the `door` of `on_click: open('safe', door, $event)`.
  struct UiCallArgument
  {
    enum class Kind
    {
      /// A number that is written out, as `2` or `-0.5`.
      Number = 0,

      /// A text between quotes, single or double.
      Text,

      /// `true` or `false`.
      Flag,

      /// The name of a value of the user interface, whose name is in
      /// `text`. It is what the value holds by the time the function is
      /// called: a number, a text, or a flag.
      Value,

      /// `$event`: what happened, which the function is handed as it is.
      Event,

      /// A value that was named and is not there by the time of the call.
      Nothing
    };

    Kind kind = Kind::Number;
    double number = 0.0;
    std::string text;
    bool flag = false;

    bool operator==(const UiCallArgument &other) const = default;
  };
} // neon

#endif //UI_CALL_ARGUMENT_HPP
