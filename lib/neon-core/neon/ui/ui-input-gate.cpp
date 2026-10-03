#include "ui-input-gate.hpp"

namespace neon
{
  UiInputGate::UiInputGate(InputContext *source, const std::shared_ptr<Logger> &logger)
    : _state(logger)
  {
    _source = source;
  }

  void UiInputGate::Refresh(const UiConsumed &consumed)
  {
    _state = _source->GetInputState();

    if (consumed.everything)
    {
      _state.Reset();
      _state.ClearPointer();
      _state.SetComposition({});
      _actions.Refresh(_source->GetInputMap(), _source->GetState(), _state);
      return;
    }

    if (consumed.keyboard) { _state.ClearKeyboard(); }
    if (consumed.wheel) { _state.ClearWheel(); }
    if (consumed.right_stick) { _state.SetRightStick(0.0, 0.0); }

    if (consumed.pointer)
    {
      _state.ClearAction(Action::Pointer_Primary);
      _state.ClearMouseButtons();
      _state.ClearPointer();
    }

    if (consumed.navigation)
    {
      for (const Action action : {
             Action::Ui_Up, Action::Ui_Right, Action::Ui_Down, Action::Ui_Left,
             Action::Ui_Accept, Action::Ui_Cancel
           })
      {
        _state.ClearAction(action);
      }
    }

    _actions.Refresh(_source->GetInputMap(), _source->GetState(), _state);
  }

  void UiInputGate::ApplyCursor()
  {
    const bool hides = _game_hides_cursor && !_needs_pointer;
    if (hides == _cursor_is_hidden) { return; }

    _cursor_is_hidden = hides;

    if (hides)
    {
      _source->CenterAndHideCursor();
    } else
    {
      _source->ShowCursor();
    }
  }

  void UiInputGate::SetNeedsPointer(const bool needs_pointer)
  {
    _needs_pointer = needs_pointer;
    ApplyCursor();
  }

  const InputState &UiInputGate::GetInputState()
  {
    return _state;
  }

  const InputMap &UiInputGate::GetInputMap()
  {
    return _source->GetInputMap();
  }

  bool UiInputGate::IsActionDown(const std::string &name)
  {
    return _actions.IsDown(name);
  }

  bool UiInputGate::WasActionPressed(const std::string &name)
  {
    return _actions.WasPressed(name);
  }

  glm::vec2 UiInputGate::ActionAxis2(const std::string &name)
  {
    return _actions.GetAxis2(name);
  }

  float UiInputGate::ActionAxis(const std::string &name)
  {
    return _actions.GetAxis(name);
  }

  glm::vec3 UiInputGate::ActionAxis3(const std::string &name)
  {
    return _actions.GetAxis3(name);
  }

  bool UiInputGate::SetSensorEnabled(const std::string &sensor, const bool enabled)
  {
    return _source->SetSensorEnabled(sensor, enabled);
  }

  bool UiInputGate::IsSensorEnabled(const std::string &sensor)
  {
    return _source->IsSensorEnabled(sensor);
  }

  bool UiInputGate::SetState(const std::string &name)
  {
    return _source->SetState(name);
  }

  const std::string &UiInputGate::GetState()
  {
    return _source->GetState();
  }

  void UiInputGate::CenterAndHideCursor()
  {
    _game_hides_cursor = true;
    ApplyCursor();
  }

  void UiInputGate::ShowCursor()
  {
    _game_hides_cursor = false;
    ApplyCursor();
  }

  void UiInputGate::StartTextInput(const TextInputArea &caret)
  {
    _source->StartTextInput(caret);
  }

  void UiInputGate::StopTextInput()
  {
    _source->StopTextInput();
  }
} // neon
