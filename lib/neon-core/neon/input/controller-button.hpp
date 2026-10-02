#ifndef CONTROLLER_BUTTON_HPP
#define CONTROLLER_BUTTON_HPP

#include <cstddef>
#include <string>

namespace neon
{
  /// A button of a controller, named by where it is and not by the letter
  /// on it, since the letters differ between makers: `south` is A on one
  /// controller and B on another, and the lower button on both.
  enum class ControllerButton
  {
    South = 0,
    East,
    West,
    North,
    LeftShoulder,
    RightShoulder,
    LeftTrigger,
    RightTrigger,
    LeftStick,
    RightStick,
    Start,
    Back,
    Guide,
    DpadUp,
    DpadDown,
    DpadLeft,
    DpadRight,

    // used only to keep track of the total count of buttons
    // ReSharper disable once CppInconsistentNaming
    COUNT
  };

  constexpr std::size_t kControllerButton_Size = static_cast<std::size_t>(ControllerButton::COUNT);

  /// What a button is called in an input map, such as `south` and
  /// `right-trigger`.
  [[nodiscard]] inline std::string NameOf(const ControllerButton button)
  {
    switch (button)
    {
      case ControllerButton::South: return "south";
      case ControllerButton::East: return "east";
      case ControllerButton::West: return "west";
      case ControllerButton::North: return "north";
      case ControllerButton::LeftShoulder: return "left-shoulder";
      case ControllerButton::RightShoulder: return "right-shoulder";
      case ControllerButton::LeftTrigger: return "left-trigger";
      case ControllerButton::RightTrigger: return "right-trigger";
      case ControllerButton::LeftStick: return "left-stick";
      case ControllerButton::RightStick: return "right-stick";
      case ControllerButton::Start: return "start";
      case ControllerButton::Back: return "back";
      case ControllerButton::Guide: return "guide";
      case ControllerButton::DpadUp: return "dpad-up";
      case ControllerButton::DpadDown: return "dpad-down";
      case ControllerButton::DpadLeft: return "dpad-left";
      case ControllerButton::DpadRight: return "dpad-right";
      default: return "unknown";
    }
  }

  /// The button of a name. Returns false when there is none.
  [[nodiscard]] inline bool ControllerButtonOf(const std::string &name, ControllerButton &button)
  {
    for (std::size_t i = 0; i < kControllerButton_Size; i++)
    {
      if (NameOf(static_cast<ControllerButton>(i)) == name)
      {
        button = static_cast<ControllerButton>(i);
        return true;
      }
    }
    return false;
  }
} // neon

#endif //CONTROLLER_BUTTON_HPP
