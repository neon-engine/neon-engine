#include "sdl2-input-system.hpp"

#include <algorithm>
#include <cmath>
#include <string>

#include <SDL.h>
#include <SDL_events.h>

namespace neon
{
  // Helpers of SDL2_InputSystem, for this file alone.
  namespace
  {
    // how far a stick has to be pushed to count, of 32767
    constexpr int stick_threshold = 16000;

    // below this the right stick is at rest, of 32767
    constexpr int stick_dead_zone = 8000;

    /// The key of the engine at a place of the keyboard, as the US layout
    /// has it. What an input map binds: a game is played by where the keys
    /// are, so that W, A, S, and D are the same four keys on every layout.
    Key KeyAt(const SDL_Scancode scancode)
    {
      switch (scancode)
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
        case SDL_SCANCODE_A: return Key::A;
        case SDL_SCANCODE_B: return Key::B;
        case SDL_SCANCODE_C: return Key::C;
        case SDL_SCANCODE_D: return Key::D;
        case SDL_SCANCODE_E: return Key::E;
        case SDL_SCANCODE_F: return Key::F;
        case SDL_SCANCODE_G: return Key::G;
        case SDL_SCANCODE_H: return Key::H;
        case SDL_SCANCODE_I: return Key::I;
        case SDL_SCANCODE_J: return Key::J;
        case SDL_SCANCODE_K: return Key::K;
        case SDL_SCANCODE_L: return Key::L;
        case SDL_SCANCODE_M: return Key::M;
        case SDL_SCANCODE_N: return Key::N;
        case SDL_SCANCODE_O: return Key::O;
        case SDL_SCANCODE_P: return Key::P;
        case SDL_SCANCODE_Q: return Key::Q;
        case SDL_SCANCODE_R: return Key::R;
        case SDL_SCANCODE_S: return Key::S;
        case SDL_SCANCODE_T: return Key::T;
        case SDL_SCANCODE_U: return Key::U;
        case SDL_SCANCODE_V: return Key::V;
        case SDL_SCANCODE_W: return Key::W;
        case SDL_SCANCODE_X: return Key::X;
        case SDL_SCANCODE_Y: return Key::Y;
        case SDL_SCANCODE_Z: return Key::Z;
        case SDL_SCANCODE_0: return Key::Digit0;
        case SDL_SCANCODE_1: return Key::Digit1;
        case SDL_SCANCODE_2: return Key::Digit2;
        case SDL_SCANCODE_3: return Key::Digit3;
        case SDL_SCANCODE_4: return Key::Digit4;
        case SDL_SCANCODE_5: return Key::Digit5;
        case SDL_SCANCODE_6: return Key::Digit6;
        case SDL_SCANCODE_7: return Key::Digit7;
        case SDL_SCANCODE_8: return Key::Digit8;
        case SDL_SCANCODE_9: return Key::Digit9;
        case SDL_SCANCODE_F1: return Key::F1;
        case SDL_SCANCODE_F2: return Key::F2;
        case SDL_SCANCODE_F3: return Key::F3;
        case SDL_SCANCODE_F4: return Key::F4;
        case SDL_SCANCODE_F5: return Key::F5;
        case SDL_SCANCODE_F6: return Key::F6;
        case SDL_SCANCODE_F7: return Key::F7;
        case SDL_SCANCODE_F8: return Key::F8;
        case SDL_SCANCODE_F9: return Key::F9;
        case SDL_SCANCODE_F10: return Key::F10;
        case SDL_SCANCODE_F11: return Key::F11;
        case SDL_SCANCODE_F12: return Key::F12;
        case SDL_SCANCODE_LSHIFT: return Key::LeftShift;
        case SDL_SCANCODE_RSHIFT: return Key::RightShift;
        case SDL_SCANCODE_LCTRL: return Key::LeftControl;
        case SDL_SCANCODE_RCTRL: return Key::RightControl;
        case SDL_SCANCODE_LALT: return Key::LeftAlt;
        case SDL_SCANCODE_RALT: return Key::RightAlt;
        default: return Key::Unknown;
      }
    }

    /// The key of the engine for a key event of SDL: the keys that editing
    /// a text and moving through a user interface ask for. The keys that
    /// move are told by where they are, the letters of the shortcuts by
    /// what they type, so that a shortcut is where the layout of the
    /// keyboard has its letter. A key that only types is left out, and
    /// arrives as text.
    Key KeyOf(const SDL_Keysym &keysym)
    {
      switch (keysym.sym)
      {
        case SDLK_a: return Key::A;
        case SDLK_c: return Key::C;
        case SDLK_v: return Key::V;
        case SDLK_x: return Key::X;
        case SDLK_y: return Key::Y;
        case SDLK_z: return Key::Z;
        default: break;
      }

      switch (keysym.scancode)
      {
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_HOME:
        case SDL_SCANCODE_END:
        case SDL_SCANCODE_PAGEUP:
        case SDL_SCANCODE_PAGEDOWN:
        case SDL_SCANCODE_BACKSPACE:
        case SDL_SCANCODE_DELETE:
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_KP_ENTER:
        case SDL_SCANCODE_TAB:
        case SDL_SCANCODE_ESCAPE:
        case SDL_SCANCODE_SPACE:
          return KeyAt(keysym.scancode);
        default:
          return Key::Unknown;
      }
    }

    /// The button of the engine for a button of SDL, which names them by
    /// the letters of one maker.
    bool ButtonOf(const SDL_GameControllerButton button, ControllerButton &named)
    {
      switch (button)
      {
        case SDL_CONTROLLER_BUTTON_A: named = ControllerButton::South; return true;
        case SDL_CONTROLLER_BUTTON_B: named = ControllerButton::East; return true;
        case SDL_CONTROLLER_BUTTON_X: named = ControllerButton::West; return true;
        case SDL_CONTROLLER_BUTTON_Y: named = ControllerButton::North; return true;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: named = ControllerButton::LeftShoulder; return true;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: named = ControllerButton::RightShoulder; return true;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: named = ControllerButton::LeftStick; return true;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: named = ControllerButton::RightStick; return true;
        case SDL_CONTROLLER_BUTTON_START: named = ControllerButton::Start; return true;
        case SDL_CONTROLLER_BUTTON_BACK: named = ControllerButton::Back; return true;
        case SDL_CONTROLLER_BUTTON_GUIDE: named = ControllerButton::Guide; return true;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: named = ControllerButton::DpadUp; return true;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: named = ControllerButton::DpadDown; return true;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: named = ControllerButton::DpadLeft; return true;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: named = ControllerButton::DpadRight; return true;
        default: return false;
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
    _secondary_down = false;
    _middle_down = false;
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

        if (event.motion.xrel != 0 || event.motion.yrel != 0) { _device = InputDevice::KeyboardAndMouse; }
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

        _device = InputDevice::KeyboardAndMouse;
        _pointer_inside = true;
        _pointer_x = event.button.x;
        _pointer_y = event.button.y;

        const bool is_down = event.type == SDL_MOUSEBUTTONDOWN;
        if (event.button.button == SDL_BUTTON_LEFT) { _primary_down = is_down; }
        if (event.button.button == SDL_BUTTON_RIGHT) { _secondary_down = is_down; }
        if (event.button.button == SDL_BUTTON_MIDDLE) { _middle_down = is_down; }
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
        if (is_down) { _device = InputDevice::KeyboardAndMouse; }

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

      case SDL_CONTROLLERBUTTONDOWN:
      {
        _device = InputDevice::Gamepad;
        break;
      }

      case SDL_CONTROLLERAXISMOTION:
      {
        // a stick that rests near its middle says nothing
        if (std::abs(static_cast<int>(event.caxis.value)) > stick_threshold) { _device = InputDevice::Gamepad; }
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

            // the sensors that are on are asked of this controller now
            _sensor_missing_said.reset();
            for (std::size_t i = 0; i < kSensor_Size; i++)
            {
              if (IsSensorEnabled(static_cast<Sensor>(i))) { ApplySensor(static_cast<Sensor>(i), true); }
            }
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

    _input_state.SetDevice(_device);
    ReadKeyboard();
    ReadPointer();
    ReadController();

    _input_state.SetComposition(_text_input_active ? _composition : TextComposition{});

    RefreshActions(_input_state, _context->GetDeltaTime());
  }

  void SDL2_InputSystem::ReadKeyboard()
  {
    const auto &state = _keys;

    // Escape cancels in a user interface and asks the game to pause. With
    // shift it closes the window whatever the game does, for when the game
    // has stopped listening.
    if (state[SDL_SCANCODE_ESCAPE])
    {
      if (state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT])
      {
        _logger->Info("Shift and escape were pressed, closing the window");
        _context->SignalToClose();
      } else
      {
        _input_state.SetKeyboardAction(Action::Ui_Cancel);
      }
    }

    // every key that is held, by where it is, for the input map. Escape
    // with shift is the window's, not the game's
    for (std::size_t scancode = 0; scancode < key_count; scancode++)
    {
      if (!state[scancode]) { continue; }
      if (scancode == SDL_SCANCODE_ESCAPE && (state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT])) { continue; }

      if (const Key key = KeyAt(static_cast<SDL_Scancode>(scancode)); key != Key::Unknown)
      {
        _input_state.SetKeyDown(key);
      }
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
    // the buttons are held whether or not there is a pointer, since a game
    // that hides the cursor shoots with them
    if (_window_focus)
    {
      if (_primary_down) { _input_state.SetMouseButtonDown(MouseButton::Left); }
      if (_secondary_down) { _input_state.SetMouseButtonDown(MouseButton::Right); }
      if (_middle_down) { _input_state.SetMouseButtonDown(MouseButton::Middle); }
    }

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

    ReadSensors();

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

    // every button that is held, by where it is, for the input map. The
    // triggers are buttons when pulled past the threshold
    for (int button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; button++)
    {
      ControllerButton named;
      if (pressed(static_cast<SDL_GameControllerButton>(button)) &&
          ButtonOf(static_cast<SDL_GameControllerButton>(button), named))
      {
        _input_state.SetControllerButtonDown(named);
      }
    }
    if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > stick_threshold)
    {
      _input_state.SetControllerButtonDown(ControllerButton::LeftTrigger);
    }
    if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > stick_threshold)
    {
      _input_state.SetControllerButtonDown(ControllerButton::RightTrigger);
    }

    // the triggers as they are, from 0 to 1, for an axis of the input map
    _input_state.SetTrigger(
      ControllerTrigger::Left,
      std::clamp(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 32767.0, 0.0, 1.0));
    _input_state.SetTrigger(
      ControllerTrigger::Right,
      std::clamp(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 32767.0, 0.0, 1.0));

    // the sticks as they are, for the input map; the right one also scrolls
    // what is under the focus
    _input_state.SetLeftStick(StickValue(stick_x), StickValue(stick_y));
    _input_state.SetRightStick(
      StickValue(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX)),
      StickValue(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY)));
  }

  void SDL2_InputSystem::ApplySensor(const Sensor sensor, const bool enabled)
  {
    auto *controller = static_cast<SDL_GameController *>(_controller);
    if (controller == nullptr) { return; }

    const SDL_SensorType type = sensor == Sensor::Gyro ? SDL_SENSOR_GYRO : SDL_SENSOR_ACCEL;
    if (!SDL_GameControllerHasSensor(controller, type))
    {
      // said once per controller, and the action reads zero
      if (enabled && !_sensor_missing_said.test(static_cast<std::size_t>(sensor)))
      {
        const std::string name = NameOf(sensor);
        _logger->Warn("The controller has no {}, what is bound to it reads zero", name);
        _sensor_missing_said.set(static_cast<std::size_t>(sensor));
      }
      return;
    }

    // off, the controller does not report it, so it costs nothing
    if (SDL_GameControllerSetSensorEnabled(controller, type, enabled ? SDL_TRUE : SDL_FALSE) != 0)
    {
      const auto error = std::string(SDL_GetError());
      const std::string name = NameOf(sensor);
      const std::string state = enabled ? "on" : "off";
      _logger->Warn("The {} cannot be turned {}: {}", name, state, error);
    }
  }

  void SDL2_InputSystem::OnSensorEnabled(const Sensor sensor, const bool enabled)
  {
    ApplySensor(sensor, enabled);
  }

  void SDL2_InputSystem::ReadSensors()
  {
    auto *controller = static_cast<SDL_GameController *>(_controller);

    for (std::size_t i = 0; i < kSensor_Size; i++)
    {
      const auto sensor = static_cast<Sensor>(i);
      if (!IsSensorEnabled(sensor)) { continue; }

      // SDL's axes: x to the right, y up, z toward the player. The gyro is
      // radians a second about them, the accelerometer metres a second
      // squared along them
      const SDL_SensorType type = sensor == Sensor::Gyro ? SDL_SENSOR_GYRO : SDL_SENSOR_ACCEL;
      float data[3] = {0.0f, 0.0f, 0.0f};
      if (SDL_GameControllerHasSensor(controller, type) &&
          SDL_GameControllerGetSensorData(controller, type, data, 3) == 0)
      {
        _input_state.SetSensor(sensor, data[0], data[1], data[2]);
      }
    }
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
