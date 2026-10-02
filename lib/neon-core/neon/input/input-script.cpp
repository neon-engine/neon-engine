#include "input-script.hpp"

#include <algorithm>
#include <charconv>
#include <format>

namespace neon
{
  // Helpers of InputScript, for this file alone.
  namespace
  {
    const std::pair<Action, const char *> action_names[] = {
      {Action::Ui_Up, "ui-up"},
      {Action::Ui_Right, "ui-right"},
      {Action::Ui_Down, "ui-down"},
      {Action::Ui_Left, "ui-left"},
      {Action::Ui_Accept, "ui-accept"},
      {Action::Ui_Cancel, "ui-cancel"},
      {Action::Pointer_Primary, "pointer-primary"}
    };

    std::string Trimmed(const std::string &text)
    {
      std::size_t first = 0;
      std::size_t last = text.size();

      while (first < last && (text[first] == ' ' || text[first] == '\t' || text[first] == '\r')) { first++; }
      while (last > first && (text[last - 1] == ' ' || text[last - 1] == '\t' || text[last - 1] == '\r')) { last--; }

      return text.substr(first, last - first);
    }

    std::vector<std::string> Words(const std::string &text)
    {
      std::vector<std::string> words;
      std::string word;

      for (const char letter : text)
      {
        if (letter == ' ' || letter == '\t')
        {
          if (!word.empty()) { words.push_back(word); }
          word.clear();
        } else
        {
          word += letter;
        }
      }

      if (!word.empty()) { words.push_back(word); }
      return words;
    }

    // Read by hand, since the functions of the standard library follow the
    // language of the machine or are missing from a compiler.
    bool ReadNumber(const std::string &text, double &number)
    {
      if (text.empty()) { return false; }

      std::size_t position = 0;
      bool negative = false;
      if (text[0] == '-' || text[0] == '+')
      {
        negative = text[0] == '-';
        position = 1;
      }

      double value = 0.0;
      double scale = 1.0;
      bool has_digit = false;
      bool after_point = false;

      for (; position < text.size(); position++)
      {
        const char letter = text[position];
        if (letter == '.' && !after_point)
        {
          after_point = true;
        } else if (letter >= '0' && letter <= '9')
        {
          has_digit = true;
          if (after_point)
          {
            scale /= 10.0;
            value += (letter - '0') * scale;
          } else
          {
            value = value * 10.0 + (letter - '0');
          }
        } else
        {
          return false;
        }
      }

      if (!has_digit) { return false; }

      number = negative ? -value : value;
      return true;
    }

    bool ReadCount(const std::string &text, std::size_t &count)
    {
      const char *last = text.data() + text.size();
      const auto [stopped_at, error] = std::from_chars(text.data(), last, count);
      return !text.empty() && error == std::errc{} && stopped_at == last;
    }

    std::string Unescaped(const std::string &text)
    {
      std::string result;

      for (std::size_t i = 0; i < text.size(); i++)
      {
        if (text[i] == '\\' && i + 1 < text.size())
        {
          i++;
          switch (text[i])
          {
            case 'n': result += '\n';
              break;
            case 't': result += '\t';
              break;
            case 's': result += ' ';
              break;
            default: result += text[i];
              break;
          }
        } else
        {
          result += text[i];
        }
      }

      return result;
    }
  }

  std::string NameOf(const Action action)
  {
    for (const auto &[known, name] : action_names)
    {
      if (known == action) { return name; }
    }
    return "";
  }

  bool InputScript::Parse(
    const std::string &text,
    const std::string &name,
    InputScript &script,
    std::vector<std::string> &errors)
  {
    const std::size_t errors_before = errors.size();
    script = InputScript{};

    std::size_t line_number = 1;
    std::size_t frame = 1;
    std::size_t start = 0;

    while (start <= text.size())
    {
      std::size_t end = text.find_first_of("\n;", start);
      if (end == std::string::npos) { end = text.size(); }

      std::string line = text.substr(start, end - start);
      const bool ends_line = end < text.size() && text[end] == '\n';
      start = end + 1;

      const std::size_t this_line = line_number;
      if (ends_line) { line_number++; }

      // a comment runs to the end of its line
      if (const std::size_t comment = line.find('#'); comment != std::string::npos &&
                                                       Trimmed(line.substr(0, comment)).empty())
      {
        line.clear();
      }

      line = Trimmed(line);
      if (line.empty()) { continue; }

      const auto report = [&](const std::string &message)
      {
        errors.push_back(std::format("{}:{}: {}", name, this_line, message));
      };

      // the frame in front, when there is one
      if (const std::size_t colon = line.find(':'); colon != std::string::npos)
      {
        const std::string front = Trimmed(line.substr(0, colon));
        if (!front.empty() && std::ranges::all_of(front, [](const char letter)
        {
          return letter >= '0' && letter <= '9';
        }))
        {
          std::size_t written = 0;
          if (!ReadCount(front, written) || written == 0)
          {
            report(std::format("the frame is '{}', where a whole number above 0 was expected", front));
            continue;
          }

          frame = written;
          line = Trimmed(line.substr(colon + 1));
          if (line.empty()) { continue; }
        }
      }

      const std::size_t space = line.find_first_of(" \t");
      const std::string command = line.substr(0, space);
      const std::string rest = space == std::string::npos ? "" : Trimmed(line.substr(space + 1));
      const auto words = Words(rest);

      Step step;
      step.frame = frame;

      if (command == "pointer")
      {
        if (words.size() == 1 && words[0] == "none")
        {
          step.kind = Kind::PointerNone;
        } else if (words.size() == 2 && ReadNumber(words[0], step.x) && ReadNumber(words[1], step.y))
        {
          step.kind = Kind::Pointer;
        } else
        {
          report(std::format("'pointer' is followed by '{}', where two numbers or none was expected", rest));
          continue;
        }
      } else if (command == "down" || command == "up" || command == "click")
      {
        if (!words.empty())
        {
          report(std::format("'{}' is followed by '{}', where nothing was expected", command, rest));
          continue;
        }
        step.kind = command == "down" ? Kind::Down : command == "up" ? Kind::Up : Kind::Click;
      } else if (command == "key")
      {
        step.kind = Kind::Key;

        if (words.empty() || KeyOf(words[0]) == Key::Unknown)
        {
          report(std::format(
            "'key' is followed by '{}', where the name of a key was expected, such as left, enter, or a", rest));
          continue;
        }

        step.key.key = KeyOf(words[0]);

        bool read = true;
        for (std::size_t i = 1; i < words.size() && read; i++)
        {
          if (words[i] == "shift") { step.key.modifiers.shift = true; }
          else if (words[i] == "control") { step.key.modifiers.control = true; }
          else if (words[i] == "alt") { step.key.modifiers.alt = true; }
          else if (words[i] == "super") { step.key.modifiers.super = true; }
          else if (words[i] == "shortcut") { step.key.modifiers.shortcut = true; }
          else if (words[i] == "word") { step.key.modifiers.word = true; }
          else
          {
            report(std::format(
              "'{}' is held with the key, where shift, control, alt, super, shortcut, or word was expected",
              words[i]));
            read = false;
          }
        }
        if (!read) { continue; }
      } else if (command == "text")
      {
        step.kind = Kind::Text;
        step.text = Unescaped(rest);
      } else if (command == "compose")
      {
        step.kind = Kind::Compose;
        step.text = Unescaped(rest);
      } else if (command == "wheel")
      {
        step.kind = Kind::Wheel;

        const bool has_precise = words.size() == 3 && words[2] == "precise";
        if ((words.size() != 2 && !has_precise) || !ReadNumber(words[0], step.x) || !ReadNumber(words[1], step.y))
        {
          report(std::format(
            "'wheel' is followed by '{}', where two numbers were expected, and precise after them or nothing",
            rest));
          continue;
        }
        step.precise = has_precise;
      } else if (command == "hold")
      {
        step.kind = Kind::Hold;

        // one of the user interface, or else a button of the input map,
        // which is checked against the map when the script is set
        bool known = false;
        for (const auto &[action, action_name] : action_names)
        {
          if (!words.empty() && words[0] == action_name)
          {
            step.action = action;
            known = true;
          }
        }
        if (!known && !words.empty()) { step.action_name = words[0]; }

        if (words.empty() || words.size() > 2 || (words.size() == 2 && (!ReadCount(words[1], step.frames) || step.frames == 0)))
        {
          report(std::format(
            "'hold' is followed by '{}', where an action such as ui-accept or jump was expected, and a number of "
            "frames after it or nothing",
            rest));
          continue;
        }
      } else if (command == "hold-key")
      {
        step.kind = Kind::HoldKey;

        if (words.empty() || KeyOf(words[0]) == Key::Unknown || words.size() > 2 ||
            (words.size() == 2 && (!ReadCount(words[1], step.frames) || step.frames == 0)))
        {
          report(std::format(
            "'hold-key' is followed by '{}', where the name of a key was expected, such as w or space, and a "
            "number of frames after it or nothing",
            rest));
          continue;
        }
        step.key.key = KeyOf(words[0]);
      } else if (command == "hold-button")
      {
        step.kind = Kind::HoldButton;

        if (words.empty() || !ControllerButtonOf(words[0], step.button) || words.size() > 2 ||
            (words.size() == 2 && (!ReadCount(words[1], step.frames) || step.frames == 0)))
        {
          report(std::format(
            "'hold-button' is followed by '{}', where the name of a button of a controller was expected, such as "
            "south or left-shoulder, and a number of frames after it or nothing",
            rest));
          continue;
        }
      } else if (command == "stick" || command == "left-stick")
      {
        step.kind = command == "stick" ? Kind::Stick : Kind::LeftStick;

        if (words.size() < 2 || words.size() > 3 || !ReadNumber(words[0], step.x) || !ReadNumber(words[1], step.y) ||
            (words.size() == 3 && (!ReadCount(words[2], step.frames) || step.frames == 0)))
        {
          report(std::format(
            "'{}' is followed by '{}', where two numbers from -1 to 1 were expected, and a number of "
            "frames after them or nothing",
            command, rest));
          continue;
        }
      } else if (command == "look")
      {
        step.kind = Kind::Look;

        if (words.size() != 2 || !ReadNumber(words[0], step.x) || !ReadNumber(words[1], step.y))
        {
          report(std::format("'look' is followed by '{}', where two numbers were expected", rest));
          continue;
        }
      } else if (command == "device")
      {
        step.kind = Kind::Device;

        if (words.size() == 1 && words[0] == "keyboard") { step.device = InputDevice::KeyboardAndMouse; }
        else if (words.size() == 1 && words[0] == "gamepad") { step.device = InputDevice::Gamepad; }
        else
        {
          report(std::format("'device' is followed by '{}', where keyboard or gamepad was expected", rest));
          continue;
        }
      } else
      {
        report(std::format(
          "'{}' is not known. Known are: pointer, down, up, click, key, text, compose, wheel, hold, hold-key, "
          "hold-button, stick, left-stick, device, look",
          command));
        continue;
      }

      script._steps.push_back(step);
    }

    if (errors.size() > errors_before)
    {
      script = InputScript{};
      return false;
    }

    // by frame, and within a frame in the order of the script
    std::ranges::stable_sort(script._steps, [](const Step &a, const Step &b) { return a.frame < b.frame; });
    return true;
  }

  const std::vector<InputScript::Step> &InputScript::GetSteps() const
  {
    return _steps;
  }

  bool InputScript::IsEmpty() const
  {
    return _steps.empty();
  }

  std::size_t InputScript::GetLastFrame() const
  {
    std::size_t last = 0;
    for (const auto &step : _steps)
    {
      std::size_t end = step.frame;
      if (step.kind == Kind::Click) { end = step.frame + 1; }
      if (step.kind == Kind::Hold || step.kind == Kind::HoldKey || step.kind == Kind::HoldButton ||
          step.kind == Kind::Stick || step.kind == Kind::LeftStick)
      {
        end = step.frame + step.frames - 1;
      }
      last = std::max(last, end);
    }
    return last;
  }

  void InputScript::Apply(const std::size_t frame, InputState &state)
  {
    for (const auto &step : _steps)
    {
      // a frame that was left out still moved the pointer and held what it
      // held. What only happens in a frame is lost with it
      if (step.frame <= _applied || step.frame > frame) { continue; }

      const bool is_now = step.frame == frame;

      switch (step.kind)
      {
        case Kind::Pointer:
          _has_pointer = true;
          _pointer_x = step.x;
          _pointer_y = step.y;
          break;
        case Kind::PointerNone:
          _has_pointer = false;
          break;
        case Kind::Down:
          _is_down = true;
          _up_at = 0;
          break;
        case Kind::Up:
          _is_down = false;
          _up_at = 0;
          break;
        case Kind::Click:
          _is_down = true;
          _up_at = step.frame + 1;
          break;
        case Kind::Key:
        {
          if (!is_now) { break; }

          KeyEvent down = step.key;
          down.is_down = true;
          state.AddKeyEvent(down);

          KeyEvent up = step.key;
          up.is_down = false;
          state.AddKeyEvent(up);
          break;
        }
        case Kind::Text:
          if (is_now) { state.AddText(step.text); }
          break;
        case Kind::Compose:
          _composition = {};
          _composition.text = step.text;
          break;
        case Kind::Wheel:
          if (is_now) { state.AddWheel(step.x, step.y, step.precise); }
          break;
        case Kind::Hold:
          _held.push_back({
            .kind = Kind::Hold,
            .action = step.action,
            .action_name = step.action_name,
            .until = step.frame + step.frames - 1
          });
          break;
        case Kind::HoldKey:
          _held.push_back({.kind = Kind::HoldKey, .key = step.key.key, .until = step.frame + step.frames - 1});
          break;
        case Kind::HoldButton:
          _held.push_back({.kind = Kind::HoldButton, .button = step.button, .until = step.frame + step.frames - 1});
          break;
        case Kind::Stick:
          _stick_x = step.x;
          _stick_y = step.y;
          _stick_until = step.frame + step.frames - 1;
          break;
        case Kind::LeftStick:
          _left_stick_x = step.x;
          _left_stick_y = step.y;
          _left_stick_until = step.frame + step.frames - 1;
          break;
        case Kind::Device:
          _device = step.device;
          break;
        case Kind::Look:
          if (is_now)
          {
            state.SetAction(Action::Mouse);
            state.SetAxisMotion(Axis::Mouse, step.x, step.y);
          }
          break;
      }
    }

    _applied = frame;
    state.SetDevice(_device);

    if (_up_at != 0 && frame >= _up_at)
    {
      _is_down = false;
      _up_at = 0;
    }

    std::erase_if(_held, [frame](const Held &held) { return frame > held.until; });
    if (frame > _stick_until)
    {
      _stick_x = 0.0;
      _stick_y = 0.0;
    }
    if (frame > _left_stick_until)
    {
      _left_stick_x = 0.0;
      _left_stick_y = 0.0;
    }

    if (_has_pointer)
    {
      state.SetPointer(_pointer_x, _pointer_y);
    } else
    {
      state.ClearPointer();
    }

    // the button of the pointer is the left button of the mouse, which an
    // input map binds whether or not there is a pointer
    if (_is_down && _has_pointer) { state.SetAction(Action::Pointer_Primary); }
    if (_is_down) { state.SetMouseButtonDown(MouseButton::Left); }

    for (const auto &held : _held)
    {
      if (held.kind == Kind::HoldKey) { state.SetKeyDown(held.key); }
      else if (held.kind == Kind::HoldButton) { state.SetControllerButtonDown(held.button); }
      else if (!held.action_name.empty()) { state.HoldAction(held.action_name); }
      else { state.SetAction(held.action); }
    }

    state.SetRightStick(_stick_x, _stick_y);
    state.SetLeftStick(_left_stick_x, _left_stick_y);
    state.SetComposition(_composition);
  }
} // neon
