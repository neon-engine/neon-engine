#include "input-system.hpp"

#include "glfw-input-system.hpp"

namespace core
{
  InputSystem::InputSystem(const SettingsConfig &settings_config)
  {
    _settings_config = settings_config;
  }

  void InputSystem::Initialize() {}
  void InputSystem::ProcessInput()
  {
    Glfw_InputSystem::ProcessInputWrapper();
  }
  void InputSystem::CleanUp() {}
} // core
