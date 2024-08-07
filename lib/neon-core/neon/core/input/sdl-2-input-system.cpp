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
    _input_state.Reset();
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
        {
          if (event.window.event == SDL_WINDOWEVENT_CLOSE)
          {
            _context->SignalToClose();
            break;
          }
          if (event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
          {
            _window_focus = true;
            std::cout << "Gained window focus" << std::endl;
            _context->SetWindowFocus(true);
          }
          else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
          {
            _window_focus = false;
            std::cout << "Lost window focus" << std::endl;
            _context->SetWindowFocus(false);
          }
          break;
        }

        case SDL_MOUSEMOTION:
        {
          if (_window_focus)
          {
            _input_state.SetAxisMotion(
              Axis::Mouse,
              event.motion.xrel,
              event.motion.yrel);
          }
        }

        default:
          break;
      }
    }

    const auto *state = SDL_GetKeyboardState(nullptr);

    // todo escape by itself should not close the game
    // we want to be able to handle input events contextually, will create an input handler later on
    if (state[SDL_SCANCODE_ESCAPE])
    {
      _context->SignalToClose();
    }

    if (state[SDL_SCANCODE_W])
    {
      _input_state.SetAction(Action::L_Up);
    }

    if (state[SDL_SCANCODE_A])
    {
      _input_state.SetAction(Action::L_Left);
    }

    if (state[SDL_SCANCODE_S])
    {
      _input_state.SetAction(Action::L_Down);
    }

    if (state[SDL_SCANCODE_D])
    {
      _input_state.SetAction(Action::L_Right);
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

  void SDL2_InputSystem::CenterAndHideCursor()
  {
    SDL_SetRelativeMouseMode(SDL_TRUE);
  }

  void SDL2_InputSystem::ShowCursor()
  {
    SDL_SetRelativeMouseMode(SDL_FALSE);
  }
} // core
