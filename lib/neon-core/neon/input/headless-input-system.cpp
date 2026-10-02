#include "headless-input-system.hpp"

#include <format>

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

    if (IsSensorEnabled(Sensor::Gyro)) { _input_state.SetSensor(Sensor::Gyro, _gyro.x, _gyro.y, _gyro.z); }
    if (IsSensorEnabled(Sensor::Accelerometer))
    {
      _input_state.SetSensor(Sensor::Accelerometer, _accelerometer.x, _accelerometer.y, _accelerometer.z);
    }

    // what a frame stands for without a window, as the headless window says
    RefreshActions(_input_state, _settings_config.time_step > 0.0 ? _settings_config.time_step : frame_time);
  }

  bool Headless_InputSystem::SetScript(const InputScript &script, std::vector<std::string> &errors)
  {
    // a hold of a name the map does not have would never fire, which is
    // found here and not after forty frames of nothing
    const std::size_t before = errors.size();
    for (const auto &step : script.GetSteps())
    {
      if (step.kind != InputScript::Kind::Hold || step.action_name.empty()) { continue; }

      const InputAction *action = _map.FindAction(step.action_name);
      if (action == nullptr)
      {
        errors.push_back(std::format(
          "'hold {}' names an action the input map does not have. It has: {}", step.action_name, ActionNames()));
      } else if (!action->IsButton())
      {
        errors.push_back(std::format(
          "'hold {}' names an axis, and only a button is held. The keys of the axis are held with hold-key",
          step.action_name));
      }
    }

    if (errors.size() > before) { return false; }

    _script = script;
    _frame = 0;
    return true;
  }

  std::string Headless_InputSystem::ActionNames() const
  {
    std::string names;
    for (const auto &action : _map.GetActions())
    {
      if (!names.empty()) { names += ", "; }
      names += action.name;
    }
    return names;
  }

  void Headless_InputSystem::SetSensor(const Sensor sensor, const double x, const double y, const double z)
  {
    (sensor == Sensor::Gyro ? _gyro : _accelerometer) = {x, y, z};
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
