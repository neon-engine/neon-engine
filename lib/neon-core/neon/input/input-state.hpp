#ifndef INPUT_STATE_HPP
#define INPUT_STATE_HPP

#include <bitset>
#include <iostream>
#include <vector>

#include "neon/logging/logger.hpp"

namespace neon
{
  enum class Action
  {
    L_Up,
    L_Right,
    L_Down,
    L_Left,

    R_Up,
    R_Right,
    R_Down,
    R_Left,

    Mouse,

    /// Moving through a user interface and choosing in it, with the keys
    /// or a controller.
    Ui_Up,
    Ui_Right,
    Ui_Down,
    Ui_Left,
    Ui_Accept,
    Ui_Cancel,

    /// The first button of the pointer, for as long as it is held down.
    Pointer_Primary,

    // used only to keep track of the total count of actions
    // ReSharper disable once CppInconsistentNaming
    COUNT
  };

  enum class Axis
  {
    Mouse = 0,

    // used only to keep track of the total count of axis
    // ReSharper disable once CppInconsistentNaming
    COUNT
  };

  struct AxisState
  {
    double x = 0;
    double y = 0;
  };

  constexpr std::size_t kAction_Size = static_cast<std::size_t>(Action::COUNT);
  constexpr std::size_t kAxis_Size = static_cast<std::size_t>(Axis::COUNT);

  class InputState
  {
    std::bitset<kAction_Size> _action_map{};
    std::vector<AxisState> _axis_map{kAxis_Size};
    bool _has_pointer = false;
    AxisState _pointer;
    std::shared_ptr<Logger> _logger;

  public:
    explicit InputState(const std::shared_ptr<Logger> &logger)
    {
      _logger = logger;
    }

    void SetAction(Action action)
    {
      _action_map.set(static_cast<size_t>(action));
    }

    void SetAxisMotion(const Axis axis, const double x_pos, const double y_pos)
    {
      switch (axis)
      {
        case Axis::Mouse:
        {
          SetAction(Action::Mouse);
          _axis_map[static_cast<size_t>(axis)].x = x_pos;
          _axis_map[static_cast<size_t>(axis)].y = y_pos;
          break;
        }
        default:
        {
          _logger->Error("unsupported axis passed in");
        }
      }
    }

    /// Releases one action, for what hands the input on after it has
    /// used a part of it.
    void ClearAction(Action action)
    {
      _action_map.reset(static_cast<size_t>(action));
    }

    /// Where the pointer is, in pixels of what is drawn to, counted from
    /// the left top corner. It stays there until it is set again.
    void SetPointer(const double x_pos, const double y_pos)
    {
      _has_pointer = true;
      _pointer.x = x_pos;
      _pointer.y = y_pos;
    }

    /// For when there is no pointer to be seen: without a window, outside
    /// the window, and while the cursor is hidden.
    void ClearPointer()
    {
      _has_pointer = false;
    }

    [[nodiscard]] bool HasPointer() const
    {
      return _has_pointer;
    }

    /// Only means something while HasPointer() is true.
    [[nodiscard]] const AxisState &GetPointer() const
    {
      return _pointer;
    }

    /// Releases every action. Where the pointer is does not change by
    /// itself, so it is kept.
    void Reset()
    {
      _action_map.reset();
    }

    bool operator[](Action action) const
    {
      return _action_map.test(static_cast<size_t>(action));
    }

    const AxisState& operator[](Axis axis) const
    {
      return _axis_map[static_cast<size_t>(axis)];
    }
  };
}

#endif //INPUT_STATE_HPP
