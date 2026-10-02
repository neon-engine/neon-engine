#ifndef MOUSE_BUTTON_HPP
#define MOUSE_BUTTON_HPP

#include <cstddef>
#include <string>

namespace neon
{
  /// A button of the mouse, as an input map names it.
  enum class MouseButton
  {
    Left = 0,
    Right,
    Middle,

    // used only to keep track of the total count of buttons
    // ReSharper disable once CppInconsistentNaming
    COUNT
  };

  constexpr std::size_t kMouseButton_Size = static_cast<std::size_t>(MouseButton::COUNT);

  /// What a button is called in an input map, such as `left`.
  [[nodiscard]] inline std::string NameOf(const MouseButton button)
  {
    switch (button)
    {
      case MouseButton::Left: return "left";
      case MouseButton::Right: return "right";
      case MouseButton::Middle: return "middle";
      default: return "unknown";
    }
  }

  /// The button of a name. Returns false when there is none.
  [[nodiscard]] inline bool MouseButtonOf(const std::string &name, MouseButton &button)
  {
    for (std::size_t i = 0; i < kMouseButton_Size; i++)
    {
      if (NameOf(static_cast<MouseButton>(i)) == name)
      {
        button = static_cast<MouseButton>(i);
        return true;
      }
    }
    return false;
  }
} // neon

#endif //MOUSE_BUTTON_HPP
