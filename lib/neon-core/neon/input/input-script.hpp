#ifndef INPUT_SCRIPT_HPP
#define INPUT_SCRIPT_HPP

#include <cstddef>
#include <string>
#include <vector>

#include "input-state.hpp"

namespace neon
{
  /// Input that is written down, frame by frame, in place of devices. It is
  /// how a run without a window is operated: by a test, and by an agent that
  /// works with the engine and has no mouse to move.
  ///
  ///     # frame: what happens in it
  ///     1: pointer 640 360
  ///     2: click
  ///     5: text Ada
  ///     6: key enter
  ///     8: wheel 0 3
  ///     9: hold ui-down
  ///
  /// A line without a frame in front belongs to the frame of the line
  /// before. Lines are set apart by line feeds or by `;`, so that a script
  /// fits on a command line. Frames count from 1, as screenshots do.
  ///
  /// | Line | Does |
  /// |---|---|
  /// | `pointer X Y` | Puts the pointer there, in pixels of what is drawn to. It stays |
  /// | `pointer none` | Takes the pointer away |
  /// | `down`, `up` | Holds the button of the pointer down, and lets it go |
  /// | `click` | Holds it down in this frame and lets it go in the next |
  /// | `key NAME [shift] [control] [alt] [shortcut] [word]` | Presses a key, such as `left` and `page-down` |
  /// | `text TEXT` | Types the rest of the line. `\n` is a line feed, `\s` a space |
  /// | `compose TEXT` | What an input method is putting together. Without a text it ends |
  /// | `wheel X Y [precise]` | Turns the wheel, in notches to the right and down |
  /// | `hold ACTION [FRAMES]` | Holds an action down for one frame or more: one of the user interface, such as `ui-accept`, or a button of the input map, such as `jump` |
  /// | `hold-key KEY [FRAMES]` | Holds a key down by where it is, such as `w`, for the input map to read |
  /// | `stick X Y [FRAMES]` | Pushes the right stick, from -1 to 1 |
  /// | `device keyboard` or `device gamepad` | What the player uses from now on, which a user interface shows hints for |
  /// | `look X Y` | Moves the mouse by so much in one frame while the view is turned with it, to the right and down |
  class InputScript
  {
  public:
    enum class Kind
    {
      Pointer = 0,
      PointerNone,
      Down,
      Up,
      Click,
      Key,
      Text,
      Compose,
      Wheel,
      Hold,
      HoldKey,
      Stick,
      Device,
      Look
    };

    struct Step
    {
      std::size_t frame = 1;
      Kind kind = Kind::Pointer;
      double x = 0.0;
      double y = 0.0;
      bool precise = false;
      KeyEvent key;
      std::string text;

      /// For a hold: an action of the user interface, or, when `action_name`
      /// is not empty, an action of the input map by its name.
      Action action = Action::Ui_Accept;
      std::string action_name;
      std::size_t frames = 1;
      InputDevice device = InputDevice::KeyboardAndMouse;
    };

  private:
    struct Held
    {
      Kind kind = Kind::Hold;
      Action action = Action::Ui_Accept;
      std::string action_name;
      Key key = Key::Unknown;
      std::size_t until = 0;
    };

    std::vector<Step> _steps;

    // what lasts from frame to frame
    std::size_t _applied = 0;
    bool _has_pointer = false;
    double _pointer_x = 0.0;
    double _pointer_y = 0.0;
    bool _is_down = false;
    std::size_t _up_at = 0;
    std::vector<Held> _held;
    double _stick_x = 0.0;
    double _stick_y = 0.0;
    std::size_t _stick_until = 0;
    TextComposition _composition;
    InputDevice _device = InputDevice::KeyboardAndMouse;

  public:
    /// Reads a script. `name` is what it is called in messages. Returns
    /// false when a line cannot be read, with a message for every such line
    /// in `errors`. Each names the script and the line.
    static bool Parse(
      const std::string &text,
      const std::string &name,
      InputScript &script,
      std::vector<std::string> &errors);

    [[nodiscard]] const std::vector<Step> &GetSteps() const;

    [[nodiscard]] bool IsEmpty() const;

    /// The last frame in which something happens.
    [[nodiscard]] std::size_t GetLastFrame() const;

    /// Puts what happens in a frame into a state that was reset. Frames
    /// are handed over in rising order. One that is left out still moves
    /// the pointer and holds what it holds.
    void Apply(std::size_t frame, InputState &state);
  };

  /// What an action is called in a script, such as `ui-accept`. Empty for
  /// what is no action.
  [[nodiscard]] std::string NameOf(Action action);
} // neon

#endif //INPUT_SCRIPT_HPP
