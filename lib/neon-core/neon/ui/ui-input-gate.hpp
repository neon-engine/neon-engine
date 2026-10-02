#ifndef UI_INPUT_GATE_HPP
#define UI_INPUT_GATE_HPP

#include <memory>

#include <neon/input/input-actions.hpp>
#include <neon/input/input-context.hpp>
#include <neon/logging/logger.hpp>

namespace neon
{
  /// What the user interface has used of the input of a frame.
  struct UiConsumed
  {
    /// Everything, as a modal user interface does.
    bool everything = false;

    /// The pointer: where it is, and its button.
    bool pointer = false;

    /// The actions that move through a user interface and choose in it.
    bool navigation = false;

    /// The keyboard, while a text is typed: the keys, the text, and the
    /// actions that only a key holds down. Moving with W, A, S, and D
    /// would otherwise follow every word that has one of them in it.
    bool keyboard = false;

    /// The wheel, when something was scrolled with it or could have been.
    bool wheel = false;

    /// The right stick of a controller, when something was scrolled with it.
    bool right_stick = false;
  };

  /// Stands between the input and the game. The game reads the input
  /// through it, and finds what the user interface has used of a frame
  /// released: a click on a button does not fire a weapon as well.
  ///
  /// The actions of the input map are worked out again from what is left,
  /// so that a key the user interface took does not fire an action either.
  /// The map and the state the game is in are the source's.
  ///
  /// It also decides whether the cursor is seen. A game that turns the
  /// view with the mouse hides it. While a user interface needs the
  /// pointer, it is shown whatever the game asked for, and hidden again
  /// afterwards.
  class UiInputGate final : public InputContext
  {
    InputContext *_source;
    InputState _state;
    InputActions _actions;

    bool _game_hides_cursor = false;
    bool _needs_pointer = false;
    bool _cursor_is_hidden = false;

    void ApplyCursor();

  public:
    UiInputGate(InputContext *source, const std::shared_ptr<Logger> &logger);

    /// Takes over the input of the frame, less what was used.
    void Refresh(const UiConsumed &consumed);

    /// Whether a user interface is shown that is used with the pointer.
    void SetNeedsPointer(bool needs_pointer);

    const InputState &GetInputState() override;

    const InputMap &GetInputMap() override;

    bool IsActionDown(const std::string &name) override;

    bool WasActionPressed(const std::string &name) override;

    glm::vec2 ActionAxis(const std::string &name) override;

    float ActionAmount(const std::string &name) override;

    glm::vec3 ActionAxis3(const std::string &name) override;

    bool SetSensorEnabled(const std::string &sensor, bool enabled) override;

    bool IsSensorEnabled(const std::string &sensor) override;

    bool SetState(const std::string &name) override;

    const std::string &GetState() override;

    void CenterAndHideCursor() override;

    void ShowCursor() override;

    void StartTextInput(const TextInputArea &caret) override;

    void StopTextInput() override;
  };
} // neon

#endif //UI_INPUT_GATE_HPP
