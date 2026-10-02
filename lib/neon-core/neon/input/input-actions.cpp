#include "input-actions.hpp"

#include <algorithm>
#include <cmath>

namespace neon
{
  glm::vec2 InputActions::FourWays(const bool up, const bool down, const bool left, const bool right)
  {
    return {
      (right ? 1.0f : 0.0f) - (left ? 1.0f : 0.0f),
      (up ? 1.0f : 0.0f) - (down ? 1.0f : 0.0f)
    };
  }

  float InputActions::Scale(const InputAction &action, const InputState &input)
  {
    return action.rate.has_value() ? *action.rate * static_cast<float>(input.GetFrameTime()) : 1.0f;
  }

  glm::vec2 InputActions::AxisOf(const InputAction &action, const InputState &input)
  {
    glm::vec2 axis{0.0f, 0.0f};

    if (action.keys.size() == 4)
    {
      axis += FourWays(
        input.IsKeyDown(action.keys[0]),
        input.IsKeyDown(action.keys[1]),
        input.IsKeyDown(action.keys[2]),
        input.IsKeyDown(action.keys[3]));
    }

    if (action.buttons.size() == 4)
    {
      axis += FourWays(
        input.IsControllerButtonDown(action.buttons[0]),
        input.IsControllerButtonDown(action.buttons[1]),
        input.IsControllerButtonDown(action.buttons[2]),
        input.IsControllerButtonDown(action.buttons[3]));
    }

    // the sticks count down as positive, the axis counts forward
    if (action.stick.has_value())
    {
      const AxisState &stick = *action.stick == Stick::Left ? input.GetLeftStick() : input.GetRightStick();
      const float scale = Scale(action, input);
      axis.x += static_cast<float>(stick.x) * scale;
      axis.y -= static_cast<float>(stick.y) * scale;
    }

    // a stick on top of keys does not move further than either, unless the
    // mouse is in it or a rate is, which count pixels and have no end
    if (!action.mouse_motion && !action.rate.has_value())
    {
      axis.x = std::clamp(axis.x, -1.0f, 1.0f);
      axis.y = std::clamp(axis.y, -1.0f, 1.0f);
      return axis;
    }

    if (action.mouse_motion && input[Action::Mouse])
    {
      const AxisState &mouse = input[Axis::Mouse];
      axis.x += static_cast<float>(mouse.x);
      axis.y -= static_cast<float>(mouse.y);
    }

    return axis;
  }

  float InputActions::Larger(const float a, const float b)
  {
    return std::abs(b) > std::abs(a) ? b : a;
  }

  float InputActions::AmountOf(const InputAction &action, const InputState &input)
  {
    float amount = 0.0f;

    // the first key is the positive one, the second the negative one
    if (action.keys.size() == 2)
    {
      amount = Larger(amount, (input.IsKeyDown(action.keys[0]) ? 1.0f : 0.0f) -
                              (input.IsKeyDown(action.keys[1]) ? 1.0f : 0.0f));
    }

    if (action.buttons.size() == 2)
    {
      amount = Larger(amount, (input.IsControllerButtonDown(action.buttons[0]) ? 1.0f : 0.0f) -
                              (input.IsControllerButtonDown(action.buttons[1]) ? 1.0f : 0.0f));
    }

    if (action.trigger.has_value())
    {
      amount = Larger(amount, static_cast<float>(input.GetTrigger(*action.trigger)) * Scale(action, input));
    }

    return amount;
  }

  /// A gyro gives radians a second, which the frame turns into radians
  /// turned in it; an accelerometer gives what it is at the moment.
  glm::vec3 InputActions::Axis3Of(const InputAction &action, const InputState &input)
  {
    if (!action.sensor.has_value()) { return {0.0f, 0.0f, 0.0f}; }

    const SensorState &read = input.GetSensor(*action.sensor);
    float scale = action.rate.value_or(1.0f);
    if (*action.sensor == Sensor::Gyro) { scale *= static_cast<float>(input.GetFrameTime()); }

    return glm::vec3{read.x, read.y, read.z} * scale;
  }

  bool InputActions::IsButtonDown(const InputAction &action, const InputState &input)
  {
    if (std::ranges::any_of(action.keys, [&](const Key key) { return input.IsKeyDown(key); })) { return true; }

    if (std::ranges::any_of(action.buttons, [&](const ControllerButton button)
    {
      return input.IsControllerButtonDown(button);
    }))
    {
      return true;
    }

    return action.mouse_button.has_value() && input.IsMouseButtonDown(*action.mouse_button);
  }

  const InputActions::Value *InputActions::Find(const std::string &name) const
  {
    for (const auto &value : _values)
    {
      if (value.name == name) { return &value; }
    }
    return nullptr;
  }

  void InputActions::Refresh(const InputMap &map, const std::string &state, const InputState &input)
  {
    const InputMapState *live = map.FindState(state);

    // the map may have changed since the frame before, so the values follow
    // it and keep what they had for the names that are still there
    std::vector<Value> values;
    values.reserve(map.GetActions().size());

    for (const auto &action : map.GetActions())
    {
      Value value;
      value.name = action.name;

      if (const Value *before = Find(action.name); before != nullptr) { value.was_down = before->is_down; }

      if (live != nullptr && live->Has(action.name))
      {
        switch (action.type)
        {
          case InputActionType::Button:
            value.is_down = IsButtonDown(action, input) || input.IsActionHeld(action.name);
            break;
          case InputActionType::Axis:
            value.amount = AmountOf(action, input);
            value.is_down = value.amount != 0.0f;
            break;
          case InputActionType::Axis2:
            value.axis = AxisOf(action, input);
            value.is_down = value.axis.x != 0.0f || value.axis.y != 0.0f;
            break;
          case InputActionType::Axis3:
            value.axis3 = Axis3Of(action, input);
            value.is_down = value.axis3 != glm::vec3{0.0f, 0.0f, 0.0f};
            break;
        }
      }

      values.push_back(value);
    }

    _values = std::move(values);
  }

  void InputActions::Reset()
  {
    _values.clear();
  }

  bool InputActions::IsDown(const std::string &name) const
  {
    const Value *value = Find(name);
    return value != nullptr && value->is_down;
  }

  bool InputActions::WasPressed(const std::string &name) const
  {
    const Value *value = Find(name);
    return value != nullptr && value->is_down && !value->was_down;
  }

  glm::vec2 InputActions::GetAxis(const std::string &name) const
  {
    const Value *value = Find(name);
    return value != nullptr ? value->axis : glm::vec2{0.0f, 0.0f};
  }

  float InputActions::GetAmount(const std::string &name) const
  {
    const Value *value = Find(name);
    return value != nullptr ? value->amount : 0.0f;
  }

  glm::vec3 InputActions::GetAxis3(const std::string &name) const
  {
    const Value *value = Find(name);
    return value != nullptr ? value->axis3 : glm::vec3{0.0f, 0.0f, 0.0f};
  }
} // neon
