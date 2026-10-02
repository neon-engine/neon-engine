#ifndef MOCK_INPUT_SYSTEM_HPP
#define MOCK_INPUT_SYSTEM_HPP

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <gmock/gmock.h>

#include <neon/input/input-system.hpp>

namespace neon::testing
{
  /// An input context whose state is whatever the test sets it to. Pressing
  /// a key is `input.state.SetKeyDown(Key::W)`, and `Refresh()` then works
  /// the actions of the map out, as a frame does. The map is the engine's
  /// default unless the test sets another.
  class FakeInputContext : public InputContext
  {
  public:
    InputState state;
    InputMap map = InputMap::Default();
    std::string current_state = map.GetFirstState();
    InputActions actions;
    std::vector<std::string> enabled_sensors;

    /// Whether a text is typed, and where the caret was said to be.
    bool is_text_input_started = false;
    TextInputArea text_input_area;

    explicit FakeInputContext(const std::shared_ptr<Logger> &logger) : state(logger) {}

    void StartTextInput(const TextInputArea &caret) override
    {
      is_text_input_started = true;
      text_input_area = caret;
    }

    void StopTextInput() override
    {
      is_text_input_started = false;
    }

    const InputState &GetInputState() override
    {
      return state;
    }

    /// Works the actions out from the state, as a frame does.
    void Refresh()
    {
      actions.Refresh(map, current_state, state);
    }

    const InputMap &GetInputMap() override
    {
      return map;
    }

    bool IsActionDown(const std::string &name) override
    {
      return actions.IsDown(name);
    }

    bool WasActionPressed(const std::string &name) override
    {
      return actions.WasPressed(name);
    }

    glm::vec2 ActionAxis(const std::string &name) override
    {
      return actions.GetAxis(name);
    }

    float ActionAmount(const std::string &name) override
    {
      return actions.GetAmount(name);
    }

    glm::vec3 ActionAxis3(const std::string &name) override
    {
      return actions.GetAxis3(name);
    }

    bool SetSensorEnabled(const std::string &sensor, const bool enabled) override
    {
      std::erase(enabled_sensors, sensor);
      if (enabled) { enabled_sensors.push_back(sensor); }
      return true;
    }

    bool IsSensorEnabled(const std::string &sensor) override
    {
      return std::ranges::find(enabled_sensors, sensor) != enabled_sensors.end();
    }

    bool SetState(const std::string &name) override
    {
      if (map.FindState(name) == nullptr) { return false; }
      current_state = name;
      return true;
    }

    const std::string &GetState() override
    {
      return current_state;
    }

    MOCK_METHOD(void, CenterAndHideCursor, (), (override));

    MOCK_METHOD(void, ShowCursor, (), (override));
  };

  class MockInputSystem : public InputSystem
  {
  public:
    explicit MockInputSystem(const std::shared_ptr<Logger> &logger, const SettingsConfig &settings_config = {})
      : InputSystem(settings_config, logger) {}

    MOCK_METHOD(void, Initialize, (), (override));

    MOCK_METHOD(void, ProcessInput, (), (override));

    MOCK_METHOD(void, CleanUp, (), (override));

    MOCK_METHOD(const InputState &, GetInputState, (), (override));

    MOCK_METHOD(const InputMap &, GetInputMap, (), (override));

    MOCK_METHOD(bool, IsActionDown, (const std::string &), (override));

    MOCK_METHOD(bool, WasActionPressed, (const std::string &), (override));

    MOCK_METHOD(glm::vec2, ActionAxis, (const std::string &), (override));

    MOCK_METHOD(float, ActionAmount, (const std::string &), (override));

    MOCK_METHOD(glm::vec3, ActionAxis3, (const std::string &), (override));

    MOCK_METHOD(bool, SetSensorEnabled, (const std::string &, bool), (override));

    MOCK_METHOD(bool, IsSensorEnabled, (const std::string &), (override));

    MOCK_METHOD(bool, SetState, (const std::string &), (override));

    MOCK_METHOD(const std::string &, GetState, (), (override));

    MOCK_METHOD(void, CenterAndHideCursor, (), (override));

    MOCK_METHOD(void, ShowCursor, (), (override));
  };
} // neon::testing

#endif //MOCK_INPUT_SYSTEM_HPP
