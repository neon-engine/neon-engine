#include "input-system.hpp"

namespace neon
{
  void InputSystem::RefreshActions(InputState &input, const double delta_time)
  {
    input.SetFrameTime(delta_time);
    _actions.Refresh(_map, _state, input);
  }

  void InputSystem::SetInputMap(const InputMap &map)
  {
    _map = map;
    _state = _map.GetFirstState();
    _actions.Reset();

    for (const auto &action : _map.GetActions())
    {
      if (action.sensor.has_value() && action.enabled) { SetSensorEnabled(NameOf(*action.sensor), true); }
    }
  }

  bool InputSystem::IsSensorEnabled(const Sensor sensor) const
  {
    return _sensors_enabled.test(static_cast<std::size_t>(sensor));
  }

  bool InputSystem::SetSensorEnabled(const std::string &sensor, const bool enabled)
  {
    Sensor which;
    if (!SensorOf(sensor, which))
    {
      _logger->Error("There is no sensor '{}'. There are: gyro, accelerometer", sensor);
      return false;
    }

    if (IsSensorEnabled(which) == enabled) { return true; }

    _sensors_enabled.set(static_cast<std::size_t>(which), enabled);
    const std::string state = enabled ? "on" : "off";
    _logger->Info("The {} is {}", sensor, state);
    OnSensorEnabled(which, enabled);
    return true;
  }

  bool InputSystem::IsSensorEnabled(const std::string &sensor)
  {
    Sensor which;
    return SensorOf(sensor, which) && IsSensorEnabled(which);
  }

  glm::vec3 InputSystem::ActionAxis3(const std::string &name)
  {
    return _actions.GetAxis3(name);
  }

  const InputMap &InputSystem::GetInputMap()
  {
    return _map;
  }

  bool InputSystem::IsActionDown(const std::string &name)
  {
    return _actions.IsDown(name);
  }

  bool InputSystem::WasActionPressed(const std::string &name)
  {
    return _actions.WasPressed(name);
  }

  glm::vec2 InputSystem::ActionAxis(const std::string &name)
  {
    return _actions.GetAxis(name);
  }

  float InputSystem::ActionAmount(const std::string &name)
  {
    return _actions.GetAmount(name);
  }

  bool InputSystem::SetState(const std::string &name)
  {
    if (_map.FindState(name) == nullptr)
    {
      _logger->Error("The input map has no state '{}'", name);
      return false;
    }

    _state = name;
    return true;
  }

  const std::string &InputSystem::GetState()
  {
    return _state;
  }
} // neon
