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
