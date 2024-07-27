#ifndef INPUT_STATE_HPP
#define INPUT_STATE_HPP
#include <bitset>
#include <iostream>
#include <vector>

#include <glm/glm.hpp>

namespace core
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

  constexpr std::size_t kAction_Size = static_cast<std::size_t>(Action::COUNT);
  constexpr std::size_t kAxis_Size = static_cast<std::size_t>(Axis::COUNT);

  class InputState
  {
    std::bitset<kAction_Size> _action_map{};
    std::vector<glm::vec2> _axis_map{kAxis_Size};

  public:

    void SetAction(Action action)
    {
      _action_map.set(static_cast<size_t>(action));
    }

    void SetAxisMotion(const Axis axis, const float x_pos, const float y_pos)
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
          std::cerr << "unsupported axis passed in" << std::endl;
          return;
      }
    }

    void Reset()
    {
      _action_map.reset();
    }

    bool operator[](Action action) const
    {
      return _action_map.test(static_cast<size_t>(action));
    }

    const glm::vec2& operator[](Axis axis) const
    {
      return _axis_map[static_cast<size_t>(axis)];
    }
  };
}

#endif //INPUT_STATE_HPP
