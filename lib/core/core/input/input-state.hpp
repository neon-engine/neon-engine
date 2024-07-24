#ifndef INPUT_STATE_HPP
#define INPUT_STATE_HPP
#include <bitset>

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

  constexpr std::size_t kAction_Size = static_cast<std::size_t>(Action::COUNT);

  class InputState
  {
    std::bitset<kAction_Size> _action_map{};

  public:
    void SetAction(Action action)
    {
      _action_map.set(static_cast<size_t>(action));
    }

    void Reset()
    {
      _action_map.reset();
    }
  };
}

#endif //INPUT_STATE_HPP
