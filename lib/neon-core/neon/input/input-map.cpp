#include "input-map.hpp"

#include <algorithm>

namespace neon
{
  void InputMap::Add(const InputAction &action)
  {
    const auto found = std::ranges::find_if(_actions, [&](const InputAction &known)
    {
      return known.name == action.name;
    });

    if (found != _actions.end())
    {
      *found = action;
    } else
    {
      _actions.push_back(action);
    }
  }

  void InputMap::Add(const InputMapState &state)
  {
    const auto found = std::ranges::find_if(_states, [&](const InputMapState &known)
    {
      return known.name == state.name;
    });

    if (found != _states.end())
    {
      *found = state;
    } else
    {
      _states.push_back(state);
    }
  }

  const std::vector<InputAction> &InputMap::GetActions() const
  {
    return _actions;
  }

  const std::vector<InputMapState> &InputMap::GetStates() const
  {
    return _states;
  }

  const InputAction *InputMap::FindAction(const std::string &name) const
  {
    for (const auto &action : _actions)
    {
      if (action.name == name) { return &action; }
    }
    return nullptr;
  }

  const InputMapState *InputMap::FindState(const std::string &name) const
  {
    for (const auto &state : _states)
    {
      if (state.name == name) { return &state; }
    }
    return nullptr;
  }

  std::string InputMap::GetFirstState() const
  {
    return _states.empty() ? "" : _states.front().name;
  }

  InputMap InputMap::Default()
  {
    InputMap map;

    map.Add({
      .name = "move",
      .type = InputActionType::Axis2,
      .keys = {Key::W, Key::S, Key::A, Key::D},
      .stick = Stick::Left
    });

    // a stick turns the view at a rate, so that it is as quick as a mouse
    // moved by so many pixels in a second
    map.Add({
      .name = "look",
      .type = InputActionType::Axis2,
      .mouse_motion = true,
      .stick = Stick::Right,
      .rate = 600.0f
    });

    map.Add({
      .name = "jump",
      .type = InputActionType::Button,
      .keys = {Key::Space},
      .buttons = {ControllerButton::South}
    });

    // running is holding a key. A controller has no binding yet; the stick
    // says how fast to walk
    map.Add({
      .name = "run",
      .type = InputActionType::Button,
      .keys = {Key::LeftShift}
    });

    map.Add({
      .name = "pause",
      .type = InputActionType::Button,
      .keys = {Key::Escape},
      .buttons = {ControllerButton::Start}
    });

    map.Add({.name = "playing", .actions = {"move", "look", "jump", "run", "pause"}});
    map.Add({.name = "menu", .actions = {"pause"}});

    return map;
  }
} // neon
