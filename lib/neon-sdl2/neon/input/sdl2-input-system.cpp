#include "sdl2-input-system.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include <SDL.h>
#include <SDL_events.h>

namespace neon
{
  namespace
  {
    // how far a stick has to be pushed to count, of 32767
    constexpr int stick_threshold = 16000;

    // below this the right stick is at rest, of 32767
    constexpr int stick_dead_zone = 8000;

    /// The key of the engine for a key of SDL. The keys that move are told
    /// by where they are, the letters by what they type, so that a
    /// shortcut is where the layout of the keyboard has its letter.
    Key KeyOf(const SDL_Keysym &keysym)
    {
      switch (keysym.scancode)
      {
        case SDL_SCANCODE_LEFT: return Key::Left;
        case SDL_SCANCODE_RIGHT: return Key::Right;
        case SDL_SCANCODE_UP: return Key::Up;
        case SDL_SCANCODE_DOWN: return Key::Down;
        case SDL_SCANCODE_HOME: return Key::Home;
        case SDL_SCANCODE_END: return Key::End;
        case SDL_SCANCODE_PAGEUP: return Key::PageUp;
        case SDL_SCANCODE_PAGEDOWN: return Key::PageDown;
        case SDL_SCANCODE_BACKSPACE: return Key::Backspace;
        case SDL_SCANCODE_DELETE: return Key::Delete;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER: return Key::Enter;
        case SDL_SCANCODE_TAB: return Key::Tab;
        case SDL_SCANCODE_ESCAPE: return Key::Escape;
        case SDL_SCANCODE_SPACE: return Key::Space;
        default: break;
      }

      switch (keysym.sym)
      {
        case SDLK_a: return Key::A;
        case SDLK_c: return Key::C;
        case SDLK_v: return Key::V;
        case SDLK_x: return Key::X;
        case SDLK_y: return Key::Y;
        case SDLK_z: return Key::Z;
        default: return Key::Unknown;
      }
    }

    KeyModifiers ModifiersOf(const Uint16 held)
    {
      KeyModifiers modifiers;
      modifiers.shift = (held & KMOD_SHIFT) != 0;
      modifiers.control = (held & KMOD_CTRL) != 0;
      modifiers.alt = (held & KMOD_ALT) != 0;
      modifiers.super = (held & KMOD_GUI) != 0;

      // what a shortcut is made with, and what moves by a word, is a
      // matter of the platform
#if defined(__APPLE__)
      modifiers.shortcut = modifiers.super;
      modifiers.word = modifiers.alt;
#else
      modifiers.shortcut = modifiers.control;
      modifiers.word = modifiers.control;
#endif
      return modifiers;
    }

    double StickValue(const int value)
    {
      if (std::abs(value) < stick_dead_zone) { return 0.0; }
      return std::clamp(static_cast<double>(value) / 32767.0, -1.0, 1.0);
    }
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

    // SDL starts with text input on where there is a keyboard. The engine
    // asks for it while a text is typed, so that an input method does not
    // take the keys a game is played with.
    SDL_StopTextInput();
    _text_input_active = false;
  }

  void SDL2_InputSystem::ReleaseKeys()
  {
    _keys.fill(false);
    _primary_down = false;
  }

  void SDL2_InputSystem::HandleEvent(const SDL_Event &event)
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

          // a key that goes up while another window has the focus is
          // never heard of
          ReleaseKeys();
        }
        else if (event.window.event == SDL_WINDOWEVENT_ENTER)
        {
          _pointer_inside = true;

          // where the pointer is when it has not moved yet
          int x = 0;
          int y = 0;
          if (SDL_GetMouseFocus() != nullptr)
          {
            SDL_GetMouseState(&x, &y);
            _pointer_x = x;
            _pointer_y = y;
          }
        }
        else if (event.window.event == SDL_WINDOWEVENT_LEAVE)
        {
          _pointer_inside = false;
        }
        break;
      }

      case SDL_MOUSEMOTION:
      {
        if (event.motion.which == SDL_TOUCH_MOUSEID) { break; }

        _pointer_inside = true;
        _pointer_x = event.motion.x;
        _pointer_y = event.motion.y;

        if (_window_focus)
        {
          _input_state.SetAxisMotion(
            Axis::Mouse,
            event.motion.xrel,
            event.motion.yrel);
        }
        break;
      }

      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
      {
        if (event.button.which == SDL_TOUCH_MOUSEID) { break; }

        _pointer_inside = true;
        _pointer_x = event.button.x;
        _pointer_y = event.button.y;

        if (event.button.button == SDL_BUTTON_LEFT) { _primary_down = event.type == SDL_MOUSEBUTTONDOWN; }
        break;
      }

      case SDL_MOUSEWHEEL:
      {
        if (!_window_focus) { break; }

        // SDL counts up the page and to the right. A platform that turns
        // the wheel around says so
        const float flipped = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f;
        const float x = event.wheel.preciseX * flipped;
        const float y = -event.wheel.preciseY * flipped;

        // a wheel reports whole notches, a trackpad parts of them
        const bool precise = x != std::round(x) || y != std::round(y);
        _input_state.AddWheel(x, y, precise);
        break;
      }

      case SDL_KEYDOWN:
      case SDL_KEYUP:
      {
        const bool is_down = event.type == SDL_KEYDOWN;
        const auto scancode = static_cast<std::size_t>(event.key.keysym.scancode);
        if (scancode < key_count) { _keys[scancode] = is_down; }

        if (const Key key = KeyOf(event.key.keysym); key != Key::Unknown)
        {
          _input_state.AddKeyEvent({
            key,
            is_down,
            is_down && event.key.repeat != 0,
            ModifiersOf(event.key.keysym.mod)
          });
        }
        break;
      }

      case SDL_TEXTINPUT:
      {
        _input_state.AddText(event.text.text);

        // what was put together is part of the text now
        _composition = {};
        break;
      }

      case SDL_TEXTEDITING:
      {
        _composition.text = event.edit.text;
        _composition.cursor = event.edit.start;
        _composition.selection_length = event.edit.length;
        break;
      }

      case SDL_TEXTEDITING_EXT:
      {
        // a text too long for the event itself, which SDL hands over to be
        // freed
        if (event.editExt.text != nullptr)
        {
          _composition.text = event.editExt.text;
          SDL_free(event.editExt.text);
        } else
        {
          _composition.text.clear();
        }
        _composition.cursor = event.editExt.start;
        _composition.selection_length = event.editExt.length;
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

  void SDL2_InputSystem::ProcessInput()
  {
    _input_state.Reset();
    SDL_Event event;

    while (SDL_PollEvent(&event)) { HandleEvent(event); }

    ReadKeyboard();
    ReadPointer();
    ReadController();

    _input_state.SetComposition(_text_input_active ? _composition : TextComposition{});
  }

  void SDL2_InputSystem::ReadKeyboard()
  {
    const auto &state = _keys;

    // we want to be able to handle input events contextually, will create an input handler later on
    if (state[SDL_SCANCODE_ESCAPE])
    {
      _context->SignalToClose();
    }

    if (state[SDL_SCANCODE_W])
    {
      _input_state.SetKeyboardAction(Action::L_Up);
    }

    if (state[SDL_SCANCODE_A])
    {
      _input_state.SetKeyboardAction(Action::L_Left);
    }

    if (state[SDL_SCANCODE_S])
    {
      _input_state.SetKeyboardAction(Action::L_Down);
    }

    if (state[SDL_SCANCODE_D])
    {
      _input_state.SetKeyboardAction(Action::L_Right);
    }

    // a user interface
    if (state[SDL_SCANCODE_UP]) { _input_state.SetKeyboardAction(Action::Ui_Up); }
    if (state[SDL_SCANCODE_RIGHT]) { _input_state.SetKeyboardAction(Action::Ui_Right); }
    if (state[SDL_SCANCODE_DOWN]) { _input_state.SetKeyboardAction(Action::Ui_Down); }
    if (state[SDL_SCANCODE_LEFT]) { _input_state.SetKeyboardAction(Action::Ui_Left); }

    if (state[SDL_SCANCODE_RETURN] || state[SDL_SCANCODE_KP_ENTER] || state[SDL_SCANCODE_SPACE])
    {
      _input_state.SetKeyboardAction(Action::Ui_Accept);
    }

    if (state[SDL_SCANCODE_BACKSPACE]) { _input_state.SetKeyboardAction(Action::Ui_Cancel); }
  }

  void SDL2_InputSystem::ReadPointer()
  {
    if (!_pointer_inside || _cursor_hidden || !_window_focus)
    {
      _input_state.ClearPointer();
      return;
    }

    // SDL counts in points of the window. On a display of high density
    // what is drawn to has more pixels than that.
    const WindowMetrics metrics = _context->GetMetrics();
    if (metrics.point_width <= 0 || metrics.point_height <= 0)
    {
      _input_state.ClearPointer();
      return;
    }

    _input_state.SetPointer(metrics.ToPixelsX(_pointer_x), metrics.ToPixelsY(_pointer_y));

    if (_primary_down) { _input_state.SetAction(Action::Pointer_Primary); }
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

    // the right stick scrolls what is under the focus
    _input_state.SetRightStick(
      StickValue(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX)),
      StickValue(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY)));
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

  void SDL2_InputSystem::StartTextInput(const TextInputArea &caret)
  {
    // SDL counts in points of the window, the engine in pixels
    const WindowMetrics metrics = _context->GetMetrics();

    SDL_Rect area;
    area.x = static_cast<int>(std::floor(metrics.ToPointsX(caret.x)));
    area.y = static_cast<int>(std::floor(metrics.ToPointsY(caret.y)));
    area.w = std::max(1, static_cast<int>(std::ceil(metrics.ToPointsX(caret.width))));
    area.h = std::max(1, static_cast<int>(std::ceil(metrics.ToPointsY(caret.height))));

    // where an input method shows its candidates, which is set before and
    // after, since platforms differ in when they read it
    SDL_SetTextInputRect(&area);

    if (!_text_input_active)
    {
      // shows the keyboard of the screen where the platform has one
      SDL_StartTextInput();
      _text_input_active = true;
    }

    SDL_SetTextInputRect(&area);
  }

  void SDL2_InputSystem::StopTextInput()
  {
    if (!_text_input_active) { return; }

    SDL_StopTextInput();
    _text_input_active = false;
    _composition = {};
  }
} // neon
