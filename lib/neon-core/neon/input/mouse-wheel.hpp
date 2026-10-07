#ifndef MOUSE_WHEEL_HPP
#define MOUSE_WHEEL_HPP

#include <cstddef>
#include <string>

namespace neon
{
  /// A way the wheel of the mouse turns, as an input map names it: a button
  /// that is down for a frame in which the wheel turned that way.
  enum class MouseWheel
  {
    /// Up the page, away from the user.
    Up = 0,

    /// Down the page, toward the user.
    Down,

    // used only to keep track of the total count of ways
    // ReSharper disable once CppInconsistentNaming
    COUNT
  };

  constexpr std::size_t kMouseWheel_Size = static_cast<std::size_t>(MouseWheel::COUNT);

  /// What a way of the wheel is called in an input map, such as `wheel-up`.
  [[nodiscard]] inline std::string NameOf(const MouseWheel wheel)
  {
    switch (wheel)
    {
      case MouseWheel::Up: return "wheel-up";
      case MouseWheel::Down: return "wheel-down";
      default: return "unknown";
    }
  }

  /// The way of a name. Returns false when there is none.
  [[nodiscard]] inline bool MouseWheelOf(const std::string &name, MouseWheel &wheel)
  {
    for (std::size_t i = 0; i < kMouseWheel_Size; i++)
    {
      if (NameOf(static_cast<MouseWheel>(i)) == name)
      {
        wheel = static_cast<MouseWheel>(i);
        return true;
      }
    }
    return false;
  }
} // neon

#endif //MOUSE_WHEEL_HPP
