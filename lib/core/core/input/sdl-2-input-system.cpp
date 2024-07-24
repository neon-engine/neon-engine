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
      switch(event.type)
      {
        case SDL_QUIT:
        {
          _context->SignalToClose();
          break;
        }

        case SDL_WINDOWEVENT:
          if (event.window.event == SDL_WINDOWEVENT_CLOSE)
          {
            _context->SignalToClose();
            break;
          }
        default:
          break;
      }
    }

    _move_direction = glm::vec3(0.0f, 0.0f, 0.0f);
    const auto *state = SDL_GetKeyboardState(nullptr);

    // todo escape by itself should not close the game
    // we want to be able to handle input events contextually, will create an input handler later on
    if (state[SDL_SCANCODE_ESCAPE])
    {
      _context->SignalToClose();
    }

  }
  void SDL2_InputSystem::CleanUp()
  {
    std::cout << "Cleaning up SDL2 input system" << std::endl;
  }

  glm::vec3 SDL2_InputSystem::GetMovementDirection()
  {
    return _move_direction;
  }
} // core
