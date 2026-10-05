#ifndef GAMEPAD_KIND_HPP
#define GAMEPAD_KIND_HPP

#include <string_view>

namespace neon
{
  /// Which family of controller a gamepad is, as far as it says: what a
  /// game shows the buttons of. A controller of another maker often says it
  /// is the one it was made to stand in for, and is taken as that.
  enum class GamepadKind
  {
    /// One that is none of the others, or that does not say.
    Other = 0,

    /// An Xbox 360, Xbox One, or Xbox Series controller: A, B, X, Y.
    Xbox,

    /// A DualShock 4, and what came before it: cross, circle, square,
    /// triangle, Share and Options.
    PlayStation4,

    /// A DualSense: the same shapes, Create and Options.
    PlayStation5,

    /// A Switch Pro controller or a pair of Joy-Cons: B, A, Y, X, with A
    /// to the right.
    Switch,
  };

  /// The name a kind has in an input script and in the log.
  [[nodiscard]] constexpr std::string_view NameOf(const GamepadKind kind)
  {
    switch (kind)
    {
      case GamepadKind::Xbox: return "xbox";
      case GamepadKind::PlayStation4: return "playstation4";
      case GamepadKind::PlayStation5: return "playstation5";
      case GamepadKind::Switch: return "switch";
      default: return "other";
    }
  }

  /// The kind of a name. False for a name that is none.
  [[nodiscard]] constexpr bool FindGamepadKind(const std::string_view name, GamepadKind &kind)
  {
    for (const GamepadKind known : {
           GamepadKind::Other, GamepadKind::Xbox, GamepadKind::PlayStation4, GamepadKind::PlayStation5,
           GamepadKind::Switch})
    {
      if (NameOf(known) == name)
      {
        kind = known;
        return true;
      }
    }
    return false;
  }
} // neon

#endif //GAMEPAD_KIND_HPP
