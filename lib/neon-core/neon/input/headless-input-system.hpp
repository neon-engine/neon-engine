#ifndef HEADLESS_INPUT_SYSTEM_HPP
#define HEADLESS_INPUT_SYSTEM_HPP

#include <memory>
#include <neon/input/input-state.hpp>
#include <neon/input/input-system.hpp>

namespace neon
{
  /// An input system without devices. Nothing is ever pressed or moved.
  // ReSharper disable once CppInconsistentNaming
  class Headless_InputSystem final : public InputSystem
  {
    InputState _input_state;

  public:
    explicit Headless_InputSystem(const SettingsConfig &settings_config, const std::shared_ptr<Logger> &logger)
      : InputSystem(settings_config, logger), _input_state(logger) {}

    void Initialize() override;

    void ProcessInput() override;

    void CleanUp() override;

    const InputState &GetInputState() override;

    void CenterAndHideCursor() override;

    void ShowCursor() override;
  };
} // neon

#endif //HEADLESS_INPUT_SYSTEM_HPP
