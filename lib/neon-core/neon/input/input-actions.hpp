#ifndef INPUT_ACTIONS_HPP
#define INPUT_ACTIONS_HPP

#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "input-map.hpp"
#include "input-state.hpp"

namespace neon
{
  /// The actions of an input map as they are in a frame: which are down,
  /// which went down in this frame, and where the axes are. It is worked
  /// out once per frame from the devices in an InputState and the map, for
  /// the state the game is in. Only the actions of that state fire.
  ///
  /// An input system has one, and so has what stands between the input and
  /// the game, so that an action is read from what the user interface left.
  class InputActions
  {
    struct Value
    {
      std::string name;
      bool is_down = false;
      bool was_down = false;
      glm::vec2 axis{0.0f, 0.0f};
      float amount = 0.0f;
      glm::vec3 axis3{0.0f, 0.0f, 0.0f};
    };

    std::vector<Value> _values;

    [[nodiscard]] const Value *Find(const std::string &name) const;

  public:
    /// Works the actions out for a frame. `state` is the state the game is
    /// in; an action that is not in it is not down. One that is not in the
    /// map is never down. What counts per second takes the time of the
    /// frame from the input.
    void Refresh(const InputMap &map, const std::string &state, const InputState &input);

    /// Forgets the frame, so that nothing is down and nothing was.
    void Reset();

    /// Whether the action is down in this frame. An axis is down while it
    /// is away from its middle.
    [[nodiscard]] bool IsDown(const std::string &name) const;

    /// Whether the action went down in this frame and was not down in the
    /// one before.
    [[nodiscard]] bool WasPressed(const std::string &name) const;

    /// Where an axis of two is: x to the right, y forward, from -1 to 1 from
    /// keys and sticks, in pixels moved from the mouse or at a rate. Zero
    /// for anything else.
    [[nodiscard]] glm::vec2 GetAxis(const std::string &name) const;

    /// Where an axis of one is: from -1 to 1 from two keys or buttons, from
    /// 0 to 1 from a trigger, the larger of them. Zero for anything else.
    [[nodiscard]] float GetAmount(const std::string &name) const;

    /// What a motion sensor gave an axis of three: about x, y, and z. A gyro
    /// is turned into radians turned in the frame. Zero for anything else,
    /// and while the sensor is off.
    [[nodiscard]] glm::vec3 GetAxis3(const std::string &name) const;
  };
} // neon

#endif //INPUT_ACTIONS_HPP
