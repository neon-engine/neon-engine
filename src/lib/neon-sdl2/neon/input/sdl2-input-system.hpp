#ifndef SDL_2_INPUT_SYSTEM_HPP
#define SDL_2_INPUT_SYSTEM_HPP

#include <neon/input/input-state.hpp>
#include <neon/input/input-system.hpp>
#include <neon/window/window-context.hpp>

namespace neon
{
  // ReSharper disable once CppInconsistentNaming
  class SDL2_InputSystem final : public InputSystem {
    WindowContext* _context;
    InputState _input_state;
    bool _window_focus = false;
  public:
    explicit SDL2_InputSystem(
      const SettingsConfig &settings_config,
      WindowContext* context,
      const std::shared_ptr<Logger> &logger)
      : InputSystem(settings_config, logger), _input_state(logger)
    {
      _context = context;
    }

    void Initialize() override;

    void ProcessInput() override;

    void CleanUp() override;

    const InputState & GetInputState() override;

    void CenterAndHideCursor() override;

    void ShowCursor() override;
  };
} // neon

#endif //SDL_2_INPUT_SYSTEM_HPP
