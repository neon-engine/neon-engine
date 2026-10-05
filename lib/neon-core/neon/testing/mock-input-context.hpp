#ifndef MOCK_INPUT_CONTEXT_HPP
#define MOCK_INPUT_CONTEXT_HPP

#include <string>

#include <gmock/gmock.h>

#include <neon/input/input-context.hpp>

namespace neon::testing
{
  class MockInputContext : public InputContext
  {
  public:
    MOCK_METHOD(const InputState &, GetInputState, (), (override));
    MOCK_METHOD(const InputMap &, GetInputMap, (), (override));
    MOCK_METHOD(InputDevice, GetDevice, (), (override));
    MOCK_METHOD(GamepadKind, GetGamepadKind, (), (override));
    MOCK_METHOD(bool, IsActionDown, (const std::string &name), (override));
    MOCK_METHOD(bool, WasActionPressed, (const std::string &name), (override));
    MOCK_METHOD(glm::vec2, ActionAxis2, (const std::string &name), (override));
    MOCK_METHOD(float, ActionAxis, (const std::string &name), (override));
    MOCK_METHOD(glm::vec3, ActionAxis3, (const std::string &name), (override));
    MOCK_METHOD(bool, SetSensorEnabled, (const std::string &sensor, bool enabled), (override));
    MOCK_METHOD(bool, IsSensorEnabled, (const std::string &sensor), (override));
    MOCK_METHOD(bool, SetState, (const std::string &name), (override));
    MOCK_METHOD(const std::string &, GetState, (), (override));
    MOCK_METHOD(void, CenterAndHideCursor, (), (override));
    MOCK_METHOD(void, ShowCursor, (), (override));
  };
} // neon::testing

#endif //MOCK_INPUT_CONTEXT_HPP
