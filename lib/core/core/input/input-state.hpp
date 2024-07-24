#ifndef INPUT_STATE_HPP
#define INPUT_STATE_HPP
#include <bitset>

#include "private/input-state-constants.hpp"

namespace core
{
  enum class Action
  {
    Forward = 0,
    Left = 1,
    Right = 2,
    Back = 3,

    // used only to keep track of the total count of actions
    // ReSharper disable once CppInconsistentNaming
    COUNT
  };

  struct InputState
  {
    std::bitset<kAction_Size> action_map{};
  };
}

#endif //INPUT_STATE_HPP
