#ifndef KEY_HPP
#define KEY_HPP

#include <cstddef>
#include <string>

namespace neon
{
  /// A key of a keyboard, in two uses. A key event names the keys that
  /// editing a text and moving through a user interface ask for, by what
  /// they do, and the letters of the shortcuts by what they type, so that a
  /// shortcut is where the layout of the keyboard has its letter. A binding
  /// of an input map holds any key by where it is, as on a keyboard of the
  /// US layout, since a game is played by position: `w` is the key above
  /// `s` on every layout. A key that types is in no event as what it types:
  /// that arrives as text, since which character a key gives depends on the
  /// layout of the keyboard and on the input method.
  enum class Key
  {
    Unknown = 0,

    Left,
    Right,
    Up,
    Down,
    Home,
    End,
    PageUp,
    PageDown,

    Backspace,
    Delete,
    Enter,
    Tab,
    Escape,
    Space,

    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    /// The digits of the row above the letters, named `0` to `9`.
    Digit0,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
    Digit6,
    Digit7,
    Digit8,
    Digit9,

    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    LeftShift,
    RightShift,
    LeftControl,
    RightControl,
    LeftAlt,
    RightAlt,

    // used only to keep track of the total count of keys
    // ReSharper disable once CppInconsistentNaming
    COUNT
  };

  constexpr std::size_t kKey_Size = static_cast<std::size_t>(Key::COUNT);

  /// What is held down next to a key.
  struct KeyModifiers
  {
    bool shift = false;
    bool control = false;
    bool alt = false;

    /// The key with the sign of the system on it: command on a Mac, the
    /// Windows key elsewhere.
    bool super = false;

    /// The key shortcuts are made with on this platform, such as copy and
    /// paste: command on a Mac, control elsewhere. The backend knows the
    /// platform and says so here, so that nothing else has to.
    bool shortcut = false;

    /// The key that makes left and right move by a word: option on a Mac,
    /// control elsewhere.
    bool word = false;

    bool operator==(const KeyModifiers &other) const = default;
  };

  /// A key that went down or up in a frame.
  struct KeyEvent
  {
    Key key = Key::Unknown;
    bool is_down = true;

    /// Whether the key went down again because it is held, as a keyboard
    /// does after a moment.
    bool is_repeat = false;

    KeyModifiers modifiers;
  };

  /// What a key is called in a script of input, in an event, and in an
  /// input map, such as `left`, `page-down`, `w`, `3`, and `f5`.
  [[nodiscard]] inline std::string NameOf(const Key key)
  {
    switch (key)
    {
      case Key::Left: return "left";
      case Key::Right: return "right";
      case Key::Up: return "up";
      case Key::Down: return "down";
      case Key::Home: return "home";
      case Key::End: return "end";
      case Key::PageUp: return "page-up";
      case Key::PageDown: return "page-down";
      case Key::Backspace: return "backspace";
      case Key::Delete: return "delete";
      case Key::Enter: return "enter";
      case Key::Tab: return "tab";
      case Key::Escape: return "escape";
      case Key::Space: return "space";
      case Key::A: return "a";
      case Key::B: return "b";
      case Key::C: return "c";
      case Key::D: return "d";
      case Key::E: return "e";
      case Key::F: return "f";
      case Key::G: return "g";
      case Key::H: return "h";
      case Key::I: return "i";
      case Key::J: return "j";
      case Key::K: return "k";
      case Key::L: return "l";
      case Key::M: return "m";
      case Key::N: return "n";
      case Key::O: return "o";
      case Key::P: return "p";
      case Key::Q: return "q";
      case Key::R: return "r";
      case Key::S: return "s";
      case Key::T: return "t";
      case Key::U: return "u";
      case Key::V: return "v";
      case Key::W: return "w";
      case Key::X: return "x";
      case Key::Y: return "y";
      case Key::Z: return "z";
      case Key::Digit0: return "0";
      case Key::Digit1: return "1";
      case Key::Digit2: return "2";
      case Key::Digit3: return "3";
      case Key::Digit4: return "4";
      case Key::Digit5: return "5";
      case Key::Digit6: return "6";
      case Key::Digit7: return "7";
      case Key::Digit8: return "8";
      case Key::Digit9: return "9";
      case Key::F1: return "f1";
      case Key::F2: return "f2";
      case Key::F3: return "f3";
      case Key::F4: return "f4";
      case Key::F5: return "f5";
      case Key::F6: return "f6";
      case Key::F7: return "f7";
      case Key::F8: return "f8";
      case Key::F9: return "f9";
      case Key::F10: return "f10";
      case Key::F11: return "f11";
      case Key::F12: return "f12";
      case Key::LeftShift: return "left-shift";
      case Key::RightShift: return "right-shift";
      case Key::LeftControl: return "left-control";
      case Key::RightControl: return "right-control";
      case Key::LeftAlt: return "left-alt";
      case Key::RightAlt: return "right-alt";
      default: return "unknown";
    }
  }

  /// The key of a name, or Key::Unknown.
  [[nodiscard]] inline Key KeyOf(const std::string &name)
  {
    for (int key = 1; key < static_cast<int>(Key::COUNT); key++)
    {
      if (NameOf(static_cast<Key>(key)) == name) { return static_cast<Key>(key); }
    }
    return Key::Unknown;
  }
} // neon

#endif //KEY_HPP
