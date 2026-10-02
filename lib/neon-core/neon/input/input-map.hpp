#ifndef INPUT_MAP_HPP
#define INPUT_MAP_HPP

#include <string>
#include <vector>

#include "input-action.hpp"
#include "input-map-state.hpp"

namespace neon
{
  /// The actions a project plays with, what is bound to each, and the states
  /// that say which of them are live. It is read from an input map file by
  /// InputMapFile, or it is the engine's own default, which is how the
  /// engine was played before a project could say.
  class InputMap
  {
    std::vector<InputAction> _actions;
    std::vector<InputMapState> _states;

  public:
    /// Adds an action. One with the name of another replaces it.
    void Add(const InputAction &action);

    /// Adds a state. One with the name of another replaces it. The first
    /// state added is the one a game starts in.
    void Add(const InputMapState &state);

    [[nodiscard]] const std::vector<InputAction> &GetActions() const;

    [[nodiscard]] const std::vector<InputMapState> &GetStates() const;

    /// The action of a name, or nullptr.
    [[nodiscard]] const InputAction *FindAction(const std::string &name) const;

    /// The state of a name, or nullptr.
    [[nodiscard]] const InputMapState *FindState(const std::string &name) const;

    /// The state a game starts in: the first one. Empty without states.
    [[nodiscard]] std::string GetFirstState() const;

    /// The map a project without one plays with: `move` on W, A, S, D and
    /// the left stick, `look` on the mouse and the right stick at 600 pixels
    /// a second, `pause` on escape and start, all live in the state
    /// `playing`, and `pause` alone in `menu`.
    [[nodiscard]] static InputMap Default();
  };
} // neon

#endif //INPUT_MAP_HPP
