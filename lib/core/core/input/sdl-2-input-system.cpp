#include "sdl-2-input-system.hpp"

#include <iostream>
#include <SDL_events.h>

namespace core {
  void SDL2_InputSystem::Initialize()
  {
    std::cout << "Initializing SDL2 input system" << std::endl;
  }
  void SDL2_InputSystem::ProcessInput()
  {
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        _context->SignalToClose();
      }

      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
      {
        _context->SignalToClose();
      }
    }
  }
  void SDL2_InputSystem::CleanUp()
  {
    std::cout << "Cleaning up SDL2 input system" << std::endl;
  }
} // core