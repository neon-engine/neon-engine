#ifndef UI_CALL_HPP
#define UI_CALL_HPP

#include <string>
#include <string_view>
#include <vector>

#include "ui-call-argument.hpp"

namespace neon
{
  /// A function of the game that a file of a user interface names for
  /// something that happens to an element, with what it is handed, as a
  /// template of a web page does:
  ///
  ///     on_click: unlock
  ///     on_click: open('safe', 2, true)
  ///     on_click: set_door(door, $event)
  ///     on_change: tune(3, $event)
  ///
  /// An argument is a number, a text between quotes, `true` or `false`, the
  /// name of a value of the user interface, which is handed over as what
  /// it holds at the time, or `$event` for what happened. A name alone is
  /// the same as a name with `()`. Which function that is, is for what
  /// listens to decide: the scripts call it on the systems of the entity
  /// that shows the user interface.
  struct UiCall
  {
    /// The name of the function. Empty for no call.
    std::string function;

    std::vector<UiCallArgument> arguments;

    /// Reads a call as a file writes it. Returns false and says in
    /// `problem` what is wrong with a text that is none; `call` is then
    /// left empty.
    [[nodiscard]] static bool Parse(std::string_view text, UiCall &call, std::string &problem);

    /// As a file writes it, which Parse() reads back.
    [[nodiscard]] std::string AsText() const;

    [[nodiscard]] bool IsEmpty() const
    {
      return function.empty();
    }

    bool operator==(const UiCall &other) const = default;
  };
} // neon

#endif //UI_CALL_HPP
