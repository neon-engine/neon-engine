#include "editor-script.hpp"

#include <algorithm>
#include <cctype>
#include <format>

#include <neon/data/data-reader.hpp>

namespace neon
{
  // Helpers of EditorScript, for this file alone.
  namespace
  {
    std::string Trimmed(const std::string &text)
    {
      std::size_t first = 0;
      std::size_t last = text.size();
      while (first < last && std::isspace(static_cast<unsigned char>(text[first])) != 0) { first++; }
      while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1])) != 0) { last--; }
      return text.substr(first, last - first);
    }

    std::string Lowered(const std::string &text)
    {
      std::string lowered = text;
      std::ranges::transform(lowered, lowered.begin(), [](const unsigned char letter)
      {
        return static_cast<char>(std::tolower(letter));
      });
      return lowered;
    }

    /// The parts of a text set apart by a letter, each trimmed, empty ones
    /// kept so that `View > > Log` is found wrong.
    std::vector<std::string> Split(const std::string &text, const char by)
    {
      std::vector<std::string> parts;
      std::size_t start = 0;
      while (true)
      {
        const std::size_t end = text.find(by, start);
        parts.push_back(Trimmed(text.substr(start, end == std::string::npos ? std::string::npos : end - start)));
        if (end == std::string::npos) { return parts; }
        start = end + 1;
      }
    }

    /// Reads a number by hand, since the functions of the standard library
    /// follow the language of the machine or are missing from a compiler.
    bool ReadNumber(const std::string &text, float &number)
    {
      if (text.empty()) { return false; }

      std::size_t position = 0;
      const bool negative = text[0] == '-';
      if (text[0] == '-' || text[0] == '+') { position = 1; }

      double value = 0.0;
      double scale = 1.0;
      bool has_digit = false;
      bool after_point = false;
      for (; position < text.size(); position++)
      {
        const char letter = text[position];
        if (letter == '.' && !after_point) { after_point = true; }
        else if (letter >= '0' && letter <= '9')
        {
          has_digit = true;
          if (after_point)
          {
            scale /= 10.0;
            value += (letter - '0') * scale;
          } else { value = value * 10.0 + (letter - '0'); }
        } else { return false; }
      }

      if (!has_digit) { return false; }
      number = static_cast<float>(negative ? -value : value);
      return true;
    }

    /// Reads `x,y` as a place. False for anything else, which is a label.
    bool ReadPoint(const std::string &text, ToolUiPoint &point)
    {
      const std::vector<std::string> parts = Split(text, ',');
      return parts.size() == 2 && ReadNumber(parts[0], point.x) && ReadNumber(parts[1], point.y);
    }

    /// A place of a map, `{x: 200, y: 100}`.
    bool ReadPoint(const DataReader &reader, ToolUiPoint &point)
    {
      double x = 0.0;
      double y = 0.0;
      const bool has_x = reader.Read("x", x);
      const bool has_y = reader.Read("y", y);
      reader.Finish();
      if (!has_x || !has_y) { return false; }
      point = {static_cast<float>(x), static_cast<float>(y)};
      return true;
    }

    std::string Describe(const EditorScript::Target &target)
    {
      if (target.is_point) { return std::format("{},{}", target.point.x, target.point.y); }
      return target.label;
    }

    const char *known_steps =
      "menu, click, type, key, pointer, drag, wait, screenshot, expect";
  }

  std::string EditorScript::Step::Describe() const
  {
    switch (kind)
    {
      case Kind::Menu:
      {
        std::string joined;
        for (const std::string &part : path)
        {
          if (!joined.empty()) { joined += " > "; }
          joined += part;
        }
        return "menu " + joined;
      }
      case Kind::Click: return "click " + label;
      case Kind::Type: return std::format("type '{}' into {}", text, label);
      case Kind::Key: return "key " + text;
      case Kind::Pointer: return std::format("pointer {},{}", point.x, point.y);
      case Kind::Drag: return std::format("drag from {} to {}", neon::Describe(from), neon::Describe(to));
      case Kind::Wait: return std::format("wait {}", frames);
      case Kind::Screenshot: return "screenshot " + label;
      case Kind::ExpectWindow: return std::format("expect window {} {}", label, expected ? "shown" : "hidden");
      case Kind::ExpectText: return std::format("expect text '{}'", text);
      case Kind::ExpectItem: return std::format("expect item {} {}", label, expected ? "present" : "absent");
    }
    return "step";
  }

  bool EditorScript::ReadChord(const std::string &text, KeyEvent &key)
  {
    key = KeyEvent{};
    const std::vector<std::string> parts = Split(text, '+');
    if (parts.empty() || parts.back().empty()) { return false; }

    for (std::size_t i = 0; i + 1 < parts.size(); i++)
    {
      const std::string part = Lowered(parts[i]);
      if (part == "ctrl" || part == "control") { key.modifiers.control = true; }
      else if (part == "shift") { key.modifiers.shift = true; }
      else if (part == "alt" || part == "option") { key.modifiers.alt = true; }
      else if (part == "cmd" || part == "command" || part == "super") { key.modifiers.super = true; }
      else if (part == "shortcut") { key.modifiers.shortcut = true; }
      else { return false; }
    }

    key.key = KeyOf(Lowered(parts.back()));
    return key.key != Key::Unknown;
  }

  bool EditorScript::Read(
    const DataValue &document,
    const std::string &name,
    EditorScript &script,
    std::vector<std::string> &errors)
  {
    const std::size_t errors_before = errors.size();
    script = EditorScript{};

    const auto report = [&](const DataValue &value, const std::string &message)
    {
      errors.push_back(std::format("{}:{}: {}", name, value.GetLine(), message));
    };

    if (!document.IsMap())
    {
      report(document, "a script of the editor is a map with a list of steps under 'steps'");
      return false;
    }

    const DataReader reader(document, name, "the script", errors);
    reader.Read("description", script._description);
    const DataValue *steps = reader.ReadValue("steps");
    reader.Finish();

    if (steps == nullptr || !steps->IsList())
    {
      report(document, "a script of the editor has a list of steps under 'steps'");
      script = EditorScript{};
      return false;
    }

    for (const DataValue &written : steps->GetItems())
    {
      if (!written.IsMap() || written.GetEntries().size() != 1)
      {
        report(written, std::format("a step is a map of one key, one of: {}", known_steps));
        continue;
      }

      const auto &[command, value] = written.GetEntries().front();
      Step step;
      step.line = written.GetLine();

      const auto expect_text = [&](const std::string &what) -> bool
      {
        if (value.GetText(step.label) && !step.label.empty()) { return true; }
        report(value, std::format("'{}' is followed by {}, where {} was expected", command, DataValue::Describe(value.GetKind()), what));
        return false;
      };

      if (command == "menu")
      {
        step.kind = Kind::Menu;
        if (!expect_text("a path through the menus, such as View > Log")) { continue; }
        step.path = Split(step.label, '>');
        if (step.path.size() < 2 || std::ranges::any_of(step.path, [](const std::string &part) { return part.empty(); }))
        {
          report(value, std::format("'{}' is no path through the menus: a menu and an item set apart by >, such as View > Log", step.label));
          continue;
        }
        step.label.clear();
      } else if (command == "click")
      {
        step.kind = Kind::Click;
        if (!expect_text("the label of a button, a menu item, or a checkbox")) { continue; }
      } else if (command == "type")
      {
        step.kind = Kind::Type;
        if (!value.IsMap())
        {
          report(value, "'type' is followed by a map with 'field' and 'text', such as {field: Name, text: my-game}");
          continue;
        }
        const DataReader fields(value, name, "type", errors);
        const bool has_field = fields.Read("field", step.label);
        const bool has_text = fields.Read("text", step.text);
        fields.Finish();
        if (!has_field || !has_text || step.label.empty())
        {
          report(value, "'type' needs 'field', the label of the field, and 'text', what is typed into it");
          continue;
        }
      } else if (command == "key")
      {
        step.kind = Kind::Key;
        if (!value.GetText(step.text) || !ReadChord(step.text, step.key))
        {
          std::string written_text;
          if (!value.GetText(written_text)) { written_text = DataValue::Describe(value.GetKind()); }
          report(value, std::format(
            "'{}' is no key: the name of a key, such as l, enter, or f5, with Ctrl, Shift, Alt, Cmd, or Shortcut in "
            "front of it, set apart by +, such as Ctrl+L", written_text));
          continue;
        }
      } else if (command == "pointer")
      {
        step.kind = Kind::Pointer;
        if (!value.IsMap() || !ReadPoint(DataReader(value, name, "pointer", errors), step.point))
        {
          report(value, "'pointer' is followed by a place, {x: 200, y: 100}, in points of the window");
          continue;
        }
      } else if (command == "drag")
      {
        step.kind = Kind::Drag;
        std::string from;
        std::string to;
        bool read = value.IsMap();
        if (read)
        {
          const DataReader ends(value, name, "drag", errors);
          read = ends.Read("from", from) && ends.Read("to", to);
          ends.Finish();
        }
        if (!read || from.empty() || to.empty())
        {
          report(value, "'drag' needs 'from' and 'to', each a label or a place written as x,y");
          continue;
        }
        step.from.label = from;
        step.from.is_point = ReadPoint(from, step.from.point);
        step.to.label = to;
        step.to.is_point = ReadPoint(to, step.to.point);
      } else if (command == "wait")
      {
        step.kind = Kind::Wait;
        double frames = 0.0;
        if (!value.GetNumber(frames) || frames < 1.0 || frames != static_cast<double>(static_cast<std::size_t>(frames)))
        {
          report(value, "'wait' is followed by a number of frames, a whole number above 0");
          continue;
        }
        step.frames = static_cast<std::size_t>(frames);
      } else if (command == "screenshot")
      {
        step.kind = Kind::Screenshot;
        if (!expect_text("the path of the image, such as output://log-shown.png")) { continue; }
      } else if (command == "expect")
      {
        if (!value.IsMap())
        {
          report(value, "'expect' is followed by a map: {window: Log, shown: true}, {text: ...}, or {item: Close, present: true}");
          continue;
        }

        const DataReader expectation(value, name, "expect", errors);
        const bool has_window = expectation.Has("window");
        const bool has_text = expectation.Has("text");
        const bool has_item = expectation.Has("item");
        if (static_cast<int>(has_window) + static_cast<int>(has_text) + static_cast<int>(has_item) != 1)
        {
          report(value, "'expect' names one of 'window', 'text', or 'item'");
          continue;
        }

        if (has_window)
        {
          step.kind = Kind::ExpectWindow;
          expectation.Read("window", step.label);
          expectation.Read("shown", step.expected);
        } else if (has_text)
        {
          step.kind = Kind::ExpectText;
          expectation.Read("text", step.text);
        } else
        {
          step.kind = Kind::ExpectItem;
          expectation.Read("item", step.label);
          expectation.Read("present", step.expected);
        }
        expectation.Finish();

        if ((has_text && step.text.empty()) || (!has_text && step.label.empty()))
        {
          report(value, "'expect' names what is expected, and it is empty");
          continue;
        }
      } else
      {
        report(written, std::format("'{}' is not a step. Known are: {}", command, known_steps));
        continue;
      }

      script._steps.push_back(step);
    }

    if (errors.size() > errors_before)
    {
      script = EditorScript{};
      return false;
    }
    return true;
  }

  const std::string &EditorScript::GetDescription() const
  {
    return _description;
  }

  const std::vector<EditorScript::Step> &EditorScript::GetSteps() const
  {
    return _steps;
  }

  bool EditorScript::IsEmpty() const
  {
    return _steps.empty();
  }
} // neon
