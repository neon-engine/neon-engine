#ifndef INPUT_MAP_STATE_HPP
#define INPUT_MAP_STATE_HPP

#include <algorithm>
#include <string>
#include <vector>

namespace neon
{
  /// A state of an input map: which actions are live while the game is in
  /// it, such as `walking` and `menu`. An action that is not in the current
  /// state does not fire, whatever is pressed.
  struct InputMapState
  {
    std::string name;
    std::vector<std::string> actions;

    [[nodiscard]] bool Has(const std::string &action) const
    {
      return std::ranges::find(actions, action) != actions.end();
    }
  };
} // neon

#endif //INPUT_MAP_STATE_HPP
