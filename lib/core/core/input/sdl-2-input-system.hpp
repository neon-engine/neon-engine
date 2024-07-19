#ifndef SDL_2_INPUT_SYSTEM_HPP
#define SDL_2_INPUT_SYSTEM_HPP
#include "input-system.hpp"
#include "sdl-2-window-context.hpp"

namespace core
{
  // ReSharper disable once CppInconsistentNaming
  class SDL2_InputSystem final : public InputSystem {
    SDL2_WindowContext* _context;
  public:
    explicit SDL2_InputSystem(const SettingsConfig &settings_config, SDL2_WindowContext* context)
      : InputSystem(settings_config)
    {
      _context = context;
    }

    void Initialize() override;

    void ProcessInput() override;

    void CleanUp() override;
  };
} // core

#endif //SDL_2_INPUT_SYSTEM_HPP
