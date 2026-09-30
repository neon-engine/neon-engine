#ifndef UI_HANDLE_HPP
#define UI_HANDLE_HPP

#include <cstdint>
#include <functional>
#include <string>

#include <neon/input/key.hpp>

namespace neon
{
  /// Names an element of a user interface from outside of it, as a game
  /// and a script do.
  ///
  /// It is a number and no pointer. An element that was removed, or whose
  /// file is no longer shown, leaves its handles behind: they name nothing
  /// from then on, which IsAlive() of the user interface tells, and every
  /// function that is handed one does nothing and says so. A number is
  /// given once and never again, so a handle cannot come to name another
  /// element.
  struct UiHandle
  {
    std::uint64_t id = 0;

    /// Whether it ever named an element. Whether the element is still
    /// there is asked of the user interface.
    [[nodiscard]] bool IsSet() const
    {
      return id != 0;
    }

    bool operator==(const UiHandle &other) const = default;
  };

  /// Something that happened to an element, which a game and a script are
  /// told of.
  ///
  /// An event goes from the element it happened to up to the element at the
  /// top, as in the DOM, and is handed to what listens at each of them.
  /// Those that say nothing about where the pointer is, such as
  /// `pointer_enter`, stay at the element.
  struct UiElementEvent
  {
    /// What happened:
    ///
    ///   pointer_enter  pointer_leave  pointer_down  pointer_up
    ///   click  double_click  wheel  scroll
    ///   focused  blurred  key_down  key_up
    ///   changed  submitted
    ///   transition_ended  animation_ended
    ///   drag_start  drag_over  drop  drag_end
    ///   cancel
    std::string name;

    /// The element it happened to, and its name in its file.
    UiHandle target;
    std::string target_name;

    /// The element whose listener is called, which is the target or one
    /// above it.
    UiHandle current;

    /// What the file the element is from calls itself.
    std::string document;

    /// Where the pointer is, in units of the file of the element.
    float x = 0.0f;
    float y = 0.0f;

    /// How often the button went down in one place without a pause.
    int clicks = 0;

    Key key = Key::Unknown;
    KeyModifiers modifiers;

    /// How far the wheel was turned, in units of the file.
    float wheel_x = 0.0f;
    float wheel_y = 0.0f;

    /// What the element holds now for `changed` and `submitted`, the
    /// property for `transition_ended`, and the name of the keyframes for
    /// `animation_ended`.
    std::string value;

    /// The other element of what happened: for `drag_over` and `drop` the
    /// element that is dragged, for `focused` the one that had the focus.
    UiHandle related;

    /// Whether it goes up to the elements above.
    bool bubbles = true;

    /// Keeps the event from the elements above the one whose listener is
    /// called.
    void Stop() const
    {
      is_stopped = true;
    }

    mutable bool is_stopped = false;
  };

  using UiListener = std::function<void(const UiElementEvent &event)>;
} // neon

#endif //UI_HANDLE_HPP
