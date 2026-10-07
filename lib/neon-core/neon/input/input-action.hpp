#ifndef INPUT_ACTION_HPP
#define INPUT_ACTION_HPP

#include <optional>
#include <string>
#include <vector>

#include "chord.hpp"
#include "controller-button.hpp"
#include "key.hpp"
#include "mouse-button.hpp"
#include "mouse-wheel.hpp"
#include "stick.hpp"
#include "controller-trigger.hpp"
#include "sensor.hpp"

namespace neon
{
  /// What kind of value an action has.
  enum class InputActionType
  {
    /// Down or not, such as `jump`.
    Button = 0,

    /// One number, such as `throttle`: from -1 to 1 from two keys or two
    /// buttons, from 0 to 1 from a trigger.
    Axis,

    /// Two numbers, x to the right and y forward, such as `move` and `look`.
    /// From -1 to 1 from keys and sticks; in pixels moved from the mouse.
    Axis2,

    /// Three numbers from a motion sensor of a controller: about x, y, and
    /// z, which are pitch, yaw, and roll.
    Axis3
  };

  /// A named action of an input map and what is bound to it, as the project
  /// wrote it down. What a binding means depends on the type: a button is
  /// down while any of its keys and buttons is, an axis is put together
  /// from two keys or buttons (positive, negative) and a trigger, an axis
  /// of two from four keys or buttons (up, down, left, right), a stick, and
  /// the motion of the mouse. Each key or button is a chord: one of them,
  /// or several that are held together.
  struct InputAction
  {
    std::string name;
    InputActionType type = InputActionType::Button;

    /// For a button, any of them. For an axis, two: positive, negative. For
    /// an axis of two, four: up, down, left, right. Each one key, or a chord
    /// of keys held together.
    std::vector<Chord<Key>> keys;

    /// The buttons of a controller, in the same way.
    std::vector<Chord<ControllerButton>> buttons;

    /// A button of the mouse, for a button.
    std::optional<MouseButton> mouse_button;

    /// A way the wheel of the mouse turns, for a button: down for a frame
    /// in which the wheel turned that way.
    std::optional<MouseWheel> mouse_wheel;

    /// Whether the motion of the mouse moves the axis of two.
    bool mouse_motion = false;

    /// A stick of a controller, for an axis of two.
    std::optional<Stick> stick;

    /// A trigger of a controller, for an axis.
    std::optional<ControllerTrigger> trigger;

    /// A motion sensor of a controller, for an axis of three. Off until the
    /// player turns it on, unless `enabled` says so.
    std::optional<Sensor> sensor;

    /// Whether the sensor is on from the start, for a game that wants it.
    bool enabled = false;

    /// How much a stick or a trigger pulled all the way counts per second,
    /// in the unit the mouse gives, pixels. Without it the raw value from
    /// -1 to 1 is taken, which is right for walking and wrong for turning.
    /// On an axis of three it scales what the sensor gives.
    std::optional<float> rate;

    /// How far a stick or a trigger of an axis or an axis2 goes before it
    /// counts, from 0 to below 1, where a stick rests. What is past it is
    /// stretched so that the end is still 1. The backends hand the devices
    /// over as they are; the map shapes them.
    float dead_zone = default_dead_zone;

    /// The power the stretched value is raised to: 1 is a straight line,
    /// 2 is gentle near the middle and quick at the end.
    float curve = 1.0f;

    /// A quarter of the way, which is what the engine took off before a map
    /// could say.
    static constexpr float default_dead_zone = 0.25f;

    [[nodiscard]] bool IsButton() const
    {
      return type == InputActionType::Button;
    }
  };
} // neon

#endif //INPUT_ACTION_HPP
