#include "headless-input-system.hpp"

namespace neon
{
  void Headless_InputSystem::Initialize()
  {
    _logger->Info("Initializing headless input system");
  }

  void Headless_InputSystem::ProcessInput()
  {
    _input_state.Reset();

    _frame++;
    if (!_script.IsEmpty()) { _script.Apply(_frame, _input_state); }
  }

  void Headless_InputSystem::SetScript(const InputScript &script)
  {
    _script = script;
    _frame = 0;
  }

  void Headless_InputSystem::StartTextInput(const TextInputArea &caret)
  {
    _is_typing = true;
    _caret = caret;
  }

  void Headless_InputSystem::StopTextInput()
  {
    _is_typing = false;
  }

  bool Headless_InputSystem::IsTextInputActive() const
  {
    return _is_typing;
  }

  const TextInputArea &Headless_InputSystem::GetTextInputArea() const
  {
    return _caret;
  }

  void Headless_InputSystem::CleanUp()
  {
    _logger->Info("Cleaning up headless input system");
  }

  const InputState &Headless_InputSystem::GetInputState()
  {
    return _input_state;
  }

  void Headless_InputSystem::CenterAndHideCursor() {}

  void Headless_InputSystem::ShowCursor() {}
} // neon
