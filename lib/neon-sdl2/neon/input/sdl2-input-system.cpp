#include "sdl2-input-system.hpp"

#include <string>

#include <SDL.h>
#include <SDL_events.h>

namespace neon
{
  namespace
  {
    // how far a stick has to be pushed to count, of 32767
    constexpr int stick_threshold = 16000;
  }

  void SDL2_InputSystem::Initialize()
  {
    _logger->Info("Initializing SDL2 input system");

    // A game is played without a controller as well, so not having any is
    // no error.
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) != 0)
    {
      const auto error = std::string(SDL_GetError());
      _logger->Warn("Controllers cannot be used: {}", error);
    }
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
            _logger->Trace("Gained window focus");
            _context->SetWindowFocus(true);
          }
          else if (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
          {
            _window_focus = false;
            _logger->Trace("Lost window focus");
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
          break;
        }

        case SDL_CONTROLLERDEVICEADDED:
        {
          if (_controller == nullptr)
          {
            _controller = SDL_GameControllerOpen(event.cdevice.which);
            if (_controller != nullptr)
            {
              const char *name = SDL_GameControllerName(static_cast<SDL_GameController *>(_controller));
              const auto controller_name = std::string(name != nullptr ? name : "without a name");
              _logger->Info("Using the controller {}", controller_name);
            }
          }
          break;
        }

        case SDL_CONTROLLERDEVICEREMOVED:
        {
          auto *controller = static_cast<SDL_GameController *>(_controller);
          if (controller != nullptr &&
              SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller)) == event.cdevice.which)
          {
            _logger->Info("The controller was unplugged");
            SDL_GameControllerClose(controller);
            _controller = nullptr;
          }
          break;
        }

        default:
          break;
      }
    }

    ReadKeyboard();
    ReadPointer();
    ReadController();
  }

  void SDL2_InputSystem::ReadKeyboard()
  {
    const auto *state = SDL_GetKeyboardState(nullptr);

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

    // a user interface
    if (state[SDL_SCANCODE_UP]) { _input_state.SetAction(Action::Ui_Up); }
    if (state[SDL_SCANCODE_RIGHT]) { _input_state.SetAction(Action::Ui_Right); }
    if (state[SDL_SCANCODE_DOWN]) { _input_state.SetAction(Action::Ui_Down); }
    if (state[SDL_SCANCODE_LEFT]) { _input_state.SetAction(Action::Ui_Left); }

    if (state[SDL_SCANCODE_RETURN] || state[SDL_SCANCODE_KP_ENTER] || state[SDL_SCANCODE_SPACE])
    {
      _input_state.SetAction(Action::Ui_Accept);
    }

    if (state[SDL_SCANCODE_BACKSPACE]) { _input_state.SetAction(Action::Ui_Cancel); }
  }

  void SDL2_InputSystem::ReadPointer()
  {
    SDL_Window *window = SDL_GetMouseFocus();

    if (window == nullptr || _cursor_hidden || !_window_focus)
    {
      _input_state.ClearPointer();
      return;
    }

    int x = 0;
    int y = 0;
    const Uint32 buttons = SDL_GetMouseState(&x, &y);

    int window_width = 0;
    int window_height = 0;
    SDL_GetWindowSize(window, &window_width, &window_height);
    if (window_width <= 0 || window_height <= 0)
    {
      _input_state.ClearPointer();
      return;
    }

    // SDL counts in points of the window. On a display of high density
    // what is drawn to has more pixels than that.
    const auto [drawable_width, drawable_height] = _context->GetDrawableSize();
    _input_state.SetPointer(
      static_cast<double>(x) * drawable_width / window_width,
      static_cast<double>(y) * drawable_height / window_height);

    if (buttons & SDL_BUTTON_LMASK) { _input_state.SetAction(Action::Pointer_Primary); }
  }

  void SDL2_InputSystem::ReadController()
  {
    auto *controller = static_cast<SDL_GameController *>(_controller);
    if (controller == nullptr) { return; }

    const auto pressed = [controller](const SDL_GameControllerButton button)
    {
      return SDL_GameControllerGetButton(controller, button) != 0;
    };

    const int stick_x = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
    const int stick_y = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);

    // the left stick moves, as the keys W, A, S, and D do
    if (stick_y < -stick_threshold) { _input_state.SetAction(Action::L_Up); }
    if (stick_x > stick_threshold) { _input_state.SetAction(Action::L_Right); }
    if (stick_y > stick_threshold) { _input_state.SetAction(Action::L_Down); }
    if (stick_x < -stick_threshold) { _input_state.SetAction(Action::L_Left); }

    // a user interface is moved through with the pad, and with the stick
    if (pressed(SDL_CONTROLLER_BUTTON_DPAD_UP) || stick_y < -stick_threshold)
    {
      _input_state.SetAction(Action::Ui_Up);
    }
    if (pressed(SDL_CONTROLLER_BUTTON_DPAD_RIGHT) || stick_x > stick_threshold)
    {
      _input_state.SetAction(Action::Ui_Right);
    }
    if (pressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN) || stick_y > stick_threshold)
    {
      _input_state.SetAction(Action::Ui_Down);
    }
    if (pressed(SDL_CONTROLLER_BUTTON_DPAD_LEFT) || stick_x < -stick_threshold)
    {
      _input_state.SetAction(Action::Ui_Left);
    }

    // named by where the button is, which is the lower one and the right one
    if (pressed(SDL_CONTROLLER_BUTTON_A)) { _input_state.SetAction(Action::Ui_Accept); }
    if (pressed(SDL_CONTROLLER_BUTTON_B)) { _input_state.SetAction(Action::Ui_Cancel); }
  }

  void SDL2_InputSystem::CleanUp()
  {
    _logger->Info("Cleaning up SDL2 input system");

    if (_controller != nullptr)
    {
      SDL_GameControllerClose(static_cast<SDL_GameController *>(_controller));
      _controller = nullptr;
    }

    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
  }

  const InputState &SDL2_InputSystem::GetInputState()
  {
    return _input_state;
  }

  void SDL2_InputSystem::CenterAndHideCursor()
  {
    SDL_SetRelativeMouseMode(SDL_TRUE);
    _cursor_hidden = true;
  }

  void SDL2_InputSystem::ShowCursor()
  {
    SDL_SetRelativeMouseMode(SDL_FALSE);
    _cursor_hidden = false;
  }
} // neon
