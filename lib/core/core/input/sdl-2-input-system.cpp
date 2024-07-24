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

    const auto *state = SDL_GetKeyboardState(nullptr);

    _input_state.Reset();

    // todo escape by itself should not close the game
    // we want to be able to handle input events contextually, will create an input handler later on
    if (state[SDL_SCANCODE_ESCAPE])
    {
      _context->SignalToClose();
    }

    if (state[SDL_SCANCODE_W])
    {
      _input_state.SetAction(Action::Forward);
    }

    if (state[SDL_SCANCODE_A])
    {
      _input_state.SetAction(Action::Back);
    }

    if (state[SDL_SCANCODE_S])
    {
      _input_state.SetAction(Action::Left);
    }

    if (state[SDL_SCANCODE_D])
    {
      _input_state.SetAction(Action::Right);
    }

  }

  void SDL2_InputSystem::CleanUp()
  {
    std::cout << "Cleaning up SDL2 input system" << std::endl;
  }

  const InputState &SDL2_InputSystem::GetInputState()
  {
    return _input_state;
  }
} // core
