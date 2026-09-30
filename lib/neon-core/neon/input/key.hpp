#ifndef KEY_HPP
#define KEY_HPP

#include <string>

namespace neon
{
  /// A key of a keyboard, named by what it does and not by where it is. It
  /// is what editing a text and moving through a user interface ask for. A
  /// key that types is not in here as what it types: that arrives as text,
  /// since which character a key gives depends on the layout of the keyboard
  /// and on the input method.
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

    /// The letters that editing has shortcuts for.
    A,
    C,
    V,
    X,
    Y,
    Z,

    // used only to keep track of the total count of keys
    // ReSharper disable once CppInconsistentNaming
    COUNT
  };

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

  /// What a key is called in a script of input and in an event, such as
  /// `left` and `page-down`.
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
      case Key::C: return "c";
      case Key::V: return "v";
      case Key::X: return "x";
      case Key::Y: return "y";
      case Key::Z: return "z";
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
