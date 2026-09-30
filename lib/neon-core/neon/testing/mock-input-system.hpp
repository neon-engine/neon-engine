#ifndef MOCK_INPUT_SYSTEM_HPP
#define MOCK_INPUT_SYSTEM_HPP

#include <memory>

#include <gmock/gmock.h>

#include <neon/input/input-system.hpp>

namespace neon::testing
{
  /// An input context whose state is whatever the test sets it to. Pressing
  /// a key is `input.state.SetAction(Action::L_Up)`.
  class FakeInputContext : public InputContext
  {
  public:
    InputState state;

    explicit FakeInputContext(const std::shared_ptr<Logger> &logger) : state(logger) {}

    const InputState &GetInputState() override
    {
      return state;
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

    MOCK_METHOD(void, CenterAndHideCursor, (), (override));

    MOCK_METHOD(void, ShowCursor, (), (override));
  };
} // neon::testing

#endif //MOCK_INPUT_SYSTEM_HPP
