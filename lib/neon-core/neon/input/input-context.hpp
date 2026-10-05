#ifndef INPUT_CONTEXT_HPP
#define INPUT_CONTEXT_HPP

#include <string>

#include <glm/glm.hpp>

#include "input-map.hpp"
#include "input-state.hpp"

namespace neon
{
  /// Where a text is typed, in pixels of what is drawn to. An input method
  /// shows its candidates next to it.
  struct TextInputArea
  {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    bool operator==(const TextInputArea &other) const = default;
  };

  /// The input as the engine reads it. The devices as they are, in
  /// GetInputState(), are for the user interface and the pointer; a game
  /// reads the actions of its input map by their names, which only fire in
  /// the state the game is in.
  class InputContext
  {
  protected:

    ~InputContext() = default;

  public:
    virtual const InputState& GetInputState() = 0;

    /// The map the actions are read with: the project's, or the engine's
    /// default.
    virtual const InputMap &GetInputMap() = 0;

    /// What the player used last, keyboard and mouse or a gamepad, and
    /// which family of controller the gamepad is. A game shows the buttons
    /// of that. See InputState::GetDevice().
    [[nodiscard]] virtual InputDevice GetDevice() { return GetInputState().GetDevice(); }
    [[nodiscard]] virtual GamepadKind GetGamepadKind() { return GetInputState().GetGamepadKind(); }

    /// Whether an action of the map is down in this frame. False for a name
    /// the map does not have, and for an action outside the current state.
    virtual bool IsActionDown(const std::string &name) = 0;

    /// Whether an action went down in this frame and was not down in the
    /// one before.
    virtual bool WasActionPressed(const std::string &name) = 0;

    /// Where an axis of two of the map is: x to the right, y forward, from
    /// -1 to 1 from keys and sticks, in pixels moved from the mouse.
    virtual glm::vec2 ActionAxis2(const std::string &name) = 0;

    /// Where an axis of one of the map is: from -1 to 1 from two keys or
    /// buttons, from 0 to 1 from a trigger. The largest wins when several
    /// are used at once.
    virtual float ActionAxis(const std::string &name) = 0;

    /// Where an axis of three of the map is, from a motion sensor of a
    /// controller: about x, y, and z, which are pitch, yaw, and roll. A
    /// gyro gives radians turned in the frame. Zero while the sensor is off
    /// or the controller has none.
    virtual glm::vec3 ActionAxis3(const std::string &name) = 0;

    /// Turns a motion sensor on or off: `gyro` or `accelerometer`. Off, an
    /// action bound to it reads zero, and the controller is not asked for
    /// it. Returns false for a name that is no sensor.
    virtual bool SetSensorEnabled(const std::string &sensor, bool enabled) = 0;

    [[nodiscard]] virtual bool IsSensorEnabled(const std::string &sensor) = 0;

    /// Puts the game into a state of the map, such as `menu`, from the next
    /// frame on. Returns false and changes nothing when the map has no such
    /// state.
    virtual bool SetState(const std::string &name) = 0;

    /// The state the game is in.
    virtual const std::string &GetState() = 0;

    virtual void CenterAndHideCursor() = 0;

    virtual void ShowCursor() = 0;

    /// Says that a text is typed from now on, and where its caret is. The
    /// platform then hands over text and not only keys, lets an input
    /// method put characters together, and shows a keyboard on the screen
    /// where it has one. Called again when the caret moved. An input
    /// without a keyboard does nothing.
    virtual void StartTextInput(const TextInputArea &caret) {}

    /// Says that no text is typed any more.
    virtual void StopTextInput() {}
  };
} // neon

#endif //INPUT_CONTEXT_HPP
