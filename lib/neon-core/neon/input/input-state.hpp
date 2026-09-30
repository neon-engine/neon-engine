#ifndef INPUT_STATE_HPP
#define INPUT_STATE_HPP

#include <bitset>
#include <iostream>
#include <string>
#include <vector>

#include "key.hpp"
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

  /// Text that is being put together by an input method and is not part of
  /// the text yet, as the letters are that a character of Japanese is typed
  /// with. It is shown at the caret until it is committed or given up.
  struct TextComposition
  {
    /// UTF-8. Empty when nothing is being put together.
    std::string text;

    /// Where the input method has its caret and what it has selected, in
    /// characters from the start of the text.
    int cursor = 0;
    int selection_length = 0;

    bool operator==(const TextComposition &other) const = default;
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

    // the actions by what holds them down, so that the keyboard can be
    // taken away while a text is typed and a controller still counts
    std::bitset<kAction_Size> _keyboard_actions{};
    std::bitset<kAction_Size> _other_actions{};

    // what happened in the frame, in the order it happened in
    std::vector<KeyEvent> _key_events;
    std::string _text;
    TextComposition _composition;

    AxisState _wheel;
    bool _wheel_is_precise = false;
    AxisState _right_stick;

  public:
    explicit InputState(const std::shared_ptr<Logger> &logger)
    {
      _logger = logger;
    }

    void SetAction(Action action)
    {
      _action_map.set(static_cast<size_t>(action));
      _other_actions.set(static_cast<size_t>(action));
    }

    /// An action that a key of the keyboard holds down. It is released by
    /// ClearKeyboard(), unless something else holds it down as well.
    void SetKeyboardAction(Action action)
    {
      _action_map.set(static_cast<size_t>(action));
      _keyboard_actions.set(static_cast<size_t>(action));
    }

    /// Whether something else than the keyboard holds an action down, such
    /// as a controller. While a text is typed, backspace deletes and does
    /// not cancel, and the button of a controller still does.
    [[nodiscard]] bool IsHeldByOtherThanKeyboard(Action action) const
    {
      return _other_actions.test(static_cast<size_t>(action));
    }

    /// A key that went down or up in this frame.
    void AddKeyEvent(const KeyEvent &event)
    {
      _key_events.push_back(event);
    }

    [[nodiscard]] const std::vector<KeyEvent> &GetKeyEvents() const
    {
      return _key_events;
    }

    /// Text that was typed in this frame, as UTF-8. It is what the keyboard
    /// and the input method came to, and not the keys that were pressed.
    void AddText(const std::string &text)
    {
      _text += text;
    }

    [[nodiscard]] const std::string &GetText() const
    {
      return _text;
    }

    /// What an input method is putting together. It stays until it is set
    /// again, and is empty when nothing is put together.
    void SetComposition(const TextComposition &composition)
    {
      _composition = composition;
    }

    [[nodiscard]] const TextComposition &GetComposition() const
    {
      return _composition;
    }

    /// How far the wheel was turned in this frame, to the right and down the
    /// page. One is a notch of a wheel, which a user interface takes for
    /// three lines. `precise` says that the device reports parts of a
    /// notch, as a trackpad does.
    void AddWheel(const double x, const double y, const bool precise)
    {
      _wheel.x += x;
      _wheel.y += y;
      _wheel_is_precise = _wheel_is_precise || precise;
    }

    [[nodiscard]] const AxisState &GetWheel() const
    {
      return _wheel;
    }

    [[nodiscard]] bool IsWheelPrecise() const
    {
      return _wheel_is_precise;
    }

    /// Where the right stick of a controller is, from -1 to 1, to the right
    /// and down. It is released with the actions.
    void SetRightStick(const double x, const double y)
    {
      _right_stick.x = x;
      _right_stick.y = y;
    }

    [[nodiscard]] const AxisState &GetRightStick() const
    {
      return _right_stick;
    }

    /// Takes away what the keyboard did in this frame: the keys, the text,
    /// and the actions that only a key holds down. For what hands the input
    /// on while a text is typed.
    void ClearKeyboard()
    {
      _action_map = _other_actions;
      _keyboard_actions.reset();
      _key_events.clear();
      _text.clear();
    }

    void ClearWheel()
    {
      _wheel = {};
      _wheel_is_precise = false;
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
      _keyboard_actions.reset(static_cast<size_t>(action));
      _other_actions.reset(static_cast<size_t>(action));
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

    /// Releases every action, and forgets what happened in the frame: the
    /// keys, the text, and the wheel. Where the pointer is does not change
    /// by itself, so it is kept, and so is what an input method is putting
    /// together.
    void Reset()
    {
      _action_map.reset();
      _keyboard_actions.reset();
      _other_actions.reset();
      _key_events.clear();
      _text.clear();
      _wheel = {};
      _wheel_is_precise = false;
      _right_stick = {};
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
