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
      return;
    }

    if (consumed.pointer)
    {
      _state.ClearAction(Action::Pointer_Primary);
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
} // neon
