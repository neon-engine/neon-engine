#include "input-map-file.hpp"

#include <format>

#include <neon/data/data-reader.hpp>

namespace neon
{
  // Helpers of InputMapFile, for this file alone.
  namespace
  {
    const std::string what_is_read = "the input map";

    /// Every name of a kind, for a message that says what is known.
    template <typename Enum>
    std::string NamesOf(const std::size_t count)
    {
      std::string names;
      for (std::size_t i = 0; i < count; i++)
      {
        if (!names.empty()) { names += ", "; }
        names += NameOf(static_cast<Enum>(i));
      }
      return names;
    }

    /// The names of the keys start at 1, since 0 is the unknown key.
    std::string KeyNames()
    {
      std::string names;
      for (std::size_t i = 1; i < kKey_Size; i++)
      {
        if (!names.empty()) { names += ", "; }
        names += NameOf(static_cast<Key>(i));
      }
      return names;
    }

    /// Reads one name of a list under `name` into `value` with `of`, which
    /// turns a name into a value or returns false. What is not a name, or
    /// not known, is reported with its line.
    template <typename Value>
    bool ReadName(
      const DataReader &reader,
      const std::string &name,
      const DataValue &item,
      Value &value,
      const auto &of,
      const std::string &known)
    {
      // a digit is written without quotes, and a format reads it as a
      // number, which is the key it names
      std::string text;
      double digit = 0.0;
      if (item.GetNumber(digit) && digit >= 0.0 && digit <= 9.0 && digit == static_cast<int>(digit))
      {
        text = std::to_string(static_cast<int>(digit));
      } else if (!item.GetText(text))
      {
        reader.Report(item, std::format("'{}' holds {}, where a name was expected", name,
                                        DataValue::Describe(item.GetKind())));
        return false;
      }

      if (!of(text, value))
      {
        reader.Report(item, std::format("'{}' names '{}', which is not known. Known are: {}", name, text, known));
        return false;
      }

      return true;
    }

    /// Reads a list of bindings under `name` into `chords`. An item is a
    /// name, or a list of names that are held together, a chord. Every
    /// problem is reported with its line, and a chord with one is left out.
    template <typename Value>
    void ReadChords(
      const DataReader &reader,
      const std::string &name,
      std::vector<Chord<Value>> &chords,
      const auto &of,
      const std::string &known)
    {
      const DataValue *list = reader.ReadValue(name);
      if (list == nullptr) { return; }

      if (!list->IsList())
      {
        reader.Report(*list, std::format("'{}' is {}, where a list of names was expected", name,
                                         DataValue::Describe(list->GetKind())));
        return;
      }

      for (const auto &item : list->GetItems())
      {
        Chord<Value> chord;

        if (!item.IsList())
        {
          Value value{};
          if (!ReadName(reader, name, item, value, of, known)) { continue; }
          chord.parts.push_back(value);
          chords.push_back(chord);
          continue;
        }

        if (item.GetItems().empty())
        {
          reader.Report(item, std::format("'{}' holds an empty chord, where names held together were expected, "
                                          "such as [left-shift, w]", name));
          continue;
        }

        bool read = true;
        for (const auto &part : item.GetItems())
        {
          Value value{};
          if (part.IsList())
          {
            reader.Report(part, std::format("'{}' holds a list inside a chord, where a name was expected", name));
            read = false;
          } else if (!ReadName(reader, name, part, value, of, known))
          {
            read = false;
          } else
          {
            chord.parts.push_back(value);
          }
        }

        if (read) { chords.push_back(chord); }
      }
    }

    void ReadAction(const DataReader &reader, const DataValue &written, InputAction &action)
    {
      std::size_t type = 0;
      if (reader.ReadChoice("type", {"button", "axis", "axis2", "axis3"}, type))
      {
        action.type = static_cast<InputActionType>(type);
      } else if (!reader.Has("type"))
      {
        reader.Report(written, std::format(
                        "'type' is missing for the action '{}'. It is button, axis, axis2, or axis3", action.name));
      }

      const bool is_axis2 = action.type == InputActionType::Axis2;
      const bool is_axis = action.type == InputActionType::Axis;
      const bool is_axis3 = action.type == InputActionType::Axis3;
      const std::string what_it_is = is_axis3 ? "an axis3" : is_axis2 ? "an axis2" : is_axis ? "an axis" : "a button";

      ReadChords(reader, "keys", action.keys, [](const std::string &name, Key &key)
      {
        key = KeyOf(name);
        return key != Key::Unknown;
      }, KeyNames());

      ReadChords(reader, "buttons", action.buttons, [](const std::string &name, ControllerButton &button)
      {
        return ControllerButtonOf(name, button);
      }, NamesOf<ControllerButton>(kControllerButton_Size));

      // `mouse` is a button or a way of the wheel for a button, and
      // `motion` for an axis of two
      if (std::string mouse; reader.Read("mouse", mouse))
      {
        MouseButton button;
        MouseWheel wheel;
        if (mouse == "motion" && is_axis2)
        {
          action.mouse_motion = true;
        } else if (mouse != "motion" && action.IsButton() && MouseButtonOf(mouse, button))
        {
          action.mouse_button = button;
        } else if (action.IsButton() && MouseWheelOf(mouse, wheel))
        {
          action.mouse_wheel = wheel;
        } else if (is_axis || is_axis3)
        {
          reader.Report(*reader.ReadValue("mouse"), std::format(
                          "'mouse' is for a button or an axis2, and this action is {}", what_it_is));
        } else
        {
          reader.Report(*reader.ReadValue("mouse"), is_axis2
                          ? std::format("'mouse' is '{}', where motion was expected for an axis2", mouse)
                          : std::format("'mouse' is '{}', where a button was expected. Known are: {}, {}", mouse,
                                        NamesOf<MouseButton>(kMouseButton_Size),
                                        NamesOf<MouseWheel>(kMouseWheel_Size)));
        }
      }

      if (std::string stick; reader.Read("stick", stick))
      {
        Stick which;
        if (!is_axis2)
        {
          reader.Report(*reader.ReadValue("stick"), std::format("'stick' is for an axis2, and this action is {}", what_it_is));
        } else if (!StickOf(stick, which))
        {
          reader.Report(*reader.ReadValue("stick"), std::format("'stick' is '{}', where left or right was expected", stick));
        } else
        {
          action.stick = which;
        }
      }

      if (std::string trigger; reader.Read("trigger", trigger))
      {
        ControllerTrigger which;
        if (!is_axis)
        {
          reader.Report(*reader.ReadValue("trigger"), std::format("'trigger' is for an axis, and this action is {}", what_it_is));
        } else if (!TriggerOf(trigger, which))
        {
          reader.Report(*reader.ReadValue("trigger"), std::format(
                          "'trigger' is '{}', where left or right was expected", trigger));
        } else
        {
          action.trigger = which;
        }
      }

      // a motion sensor is the one source of an axis of three, and is off
      // until the player or the action turns it on
      if (std::string sensor; reader.Read("sensor", sensor))
      {
        Sensor which;
        if (!is_axis3)
        {
          reader.Report(*reader.ReadValue("sensor"), std::format("'sensor' is for an axis3, and this action is {}", what_it_is));
        } else if (!SensorOf(sensor, which))
        {
          reader.Report(*reader.ReadValue("sensor"), std::format(
                          "'sensor' is '{}', where gyro or accelerometer was expected", sensor));
        } else
        {
          action.sensor = which;
        }
      } else if (is_axis3)
      {
        reader.Report(written, std::format("'sensor' is missing for the axis3 '{}'. It is gyro or accelerometer", action.name));
      }

      if (bool enabled = false; reader.Read("enabled", enabled))
      {
        if (!is_axis3)
        {
          reader.Report(*reader.ReadValue("enabled"), std::format(
                          "'enabled' turns a sensor on from the start, and this action is {}", what_it_is));
        } else
        {
          action.enabled = enabled;
        }
      }

      // a rate makes a stick or a trigger count per second
      if (float rate = 0.0f; reader.Read("rate", rate))
      {
        if (action.IsButton())
        {
          reader.Report(*reader.ReadValue("rate"), "'rate' is for an axis, an axis2, or an axis3, and this action is a button");
        } else if (rate <= 0.0f)
        {
          reader.Report(*reader.ReadValue("rate"), std::format("'rate' is {}, where a number above 0 was expected", rate));
        } else
        {
          action.rate = rate;
        }
      }

      // a dead zone and a curve shape a stick or a trigger, which an axis
      // and an axis2 have
      const bool has_analog = is_axis || is_axis2;
      if (float dead_zone = 0.0f; reader.Read("dead_zone", dead_zone))
      {
        if (!has_analog)
        {
          reader.Report(*reader.ReadValue("dead_zone"), std::format(
                          "'dead_zone' is for an axis or an axis2, which have a stick or a trigger, and this action is {}",
                          what_it_is));
        } else if (dead_zone < 0.0f || dead_zone >= 1.0f)
        {
          reader.Report(*reader.ReadValue("dead_zone"), std::format(
                          "'dead_zone' is {}, where a number from 0 to below 1 was expected", dead_zone));
        } else
        {
          action.dead_zone = dead_zone;
        }
      }

      if (const DataValue *curve = reader.ReadValue("curve"); curve != nullptr)
      {
        std::string word;
        double power = 0.0;
        if (!has_analog)
        {
          reader.Report(*curve, std::format(
                          "'curve' is for an axis or an axis2, which have a stick or a trigger, and this action is {}",
                          what_it_is));
        } else if (curve->GetText(word) && word == "linear")
        {
          action.curve = 1.0f;
        } else if (curve->GetNumber(power) && power > 0.0)
        {
          action.curve = static_cast<float>(power);
        } else
        {
          reader.Report(*curve, "'curve' is not linear or a power above 0, such as 2 for a stick that is gentle near "
                        "the middle");
        }
      }

      // an axis2 is put together from four, in the order up, down, left,
      // right; an axis from two, positive and negative
      const std::size_t count = is_axis2 ? 4 : 2;
      const std::string order = is_axis2 ? "up, down, left, right" : "positive, negative";
      if (is_axis3)
      {
        for (const char *name : {"keys", "buttons"})
        {
          if (const DataValue *bound = reader.ReadValue(name); bound != nullptr)
          {
            reader.Report(*bound, std::format("'{}' is for a button, an axis, or an axis2, and this action is an axis3", name));
          }
        }
      } else if (!action.IsButton())
      {
        if (!action.keys.empty() && action.keys.size() != count)
        {
          reader.Report(*reader.ReadValue("keys"), std::format(
                          "'keys' holds {} for an {}, where {} were expected: {}",
                          action.keys.size(), is_axis2 ? "axis2" : "axis", is_axis2 ? "four" : "two", order));
        }
        if (!action.buttons.empty() && action.buttons.size() != count)
        {
          reader.Report(*reader.ReadValue("buttons"), std::format(
                          "'buttons' holds {} for an {}, where {} were expected: {}",
                          action.buttons.size(), is_axis2 ? "axis2" : "axis", is_axis2 ? "four" : "two", order));
        }
      }

      reader.Finish();
    }
  }

  InputMapFile::InputMapFile(
    FileSystemContext *file_system,
    DocumentFormat *format,
    const std::shared_ptr<Logger> &logger)
  {
    _file_system = file_system;
    _format = format;
    _logger = logger;
  }

  bool InputMapFile::Read(const std::string &path, InputMap &map, std::vector<std::string> &errors) const
  {
    _logger->Info("Reading the input map from {}", path);

    std::string text;
    if (!_file_system->ReadText(path, text))
    {
      errors.push_back(std::format("{}: the input map cannot be read", path));
      return false;
    }

    DataValue document;
    if (std::string error; !_format->Read(path, text, document, error))
    {
      errors.push_back(error);
      return false;
    }

    const std::size_t before = errors.size();
    const DataReader reader(document, path, what_is_read, errors);

    // The version is the first thing read, so that a file of a later layout
    // says so before its names are reported as unknown.
    int written = 0;
    if (!reader.Read("version", written))
    {
      if (!reader.Has("version"))
      {
        reader.Report(std::format("'version' is missing. It holds the version of the layout, which is {}", version));
      }
    } else if (written > version)
    {
      reader.Report(*document.Find("version"), std::format(
                      "the input map has version {}, and this engine reads up to version {}",
                      written, version));
    }

    InputMap read;

    const DataValue *actions = reader.ReadValue("actions");
    if (actions == nullptr)
    {
      reader.Report("'actions' is missing. It holds the actions by their names, with what is bound to each");
    } else if (!actions->IsMap())
    {
      reader.Report(*actions, std::format("'actions' is {}, where a map of actions by their names was expected",
                                          DataValue::Describe(actions->GetKind())));
    } else
    {
      for (const auto &[name, value] : actions->GetEntries())
      {
        InputAction action;
        action.name = name;

        // an action that cannot be read is still known by its name, so that
        // the states that name it are not reported as well
        if (!value.IsMap())
        {
          reader.Report(value, std::format("the action '{}' is {}, where a map was expected, such as "
                                           "{{ type: button, keys: [space] }}",
                                           name, DataValue::Describe(value.GetKind())));
          read.Add(action);
          continue;
        }

        const DataReader action_reader(value, path, std::format("the action '{}'", name), errors);
        ReadAction(action_reader, value, action);
        read.Add(action);
      }

      if (actions->GetEntries().empty()) { reader.Report(*actions, "'actions' is empty, a map has at least one action"); }
    }

    const DataValue *states = reader.ReadValue("states");
    if (states == nullptr)
    {
      reader.Report("'states' is missing. It holds the states by their names, each a list of the actions that are "
        "live in it");
    } else if (!states->IsMap())
    {
      reader.Report(*states, std::format("'states' is {}, where a map of states by their names was expected",
                                         DataValue::Describe(states->GetKind())));
    } else
    {
      for (const auto &[name, value] : states->GetEntries())
      {
        if (!value.IsList())
        {
          reader.Report(value, std::format("the state '{}' is {}, where a list of actions was expected",
                                           name, DataValue::Describe(value.GetKind())));
          continue;
        }

        InputMapState state;
        state.name = name;

        for (const auto &item : value.GetItems())
        {
          std::string action;
          if (!item.GetText(action))
          {
            reader.Report(item, std::format("the state '{}' holds {}, where the name of an action was expected",
                                            name, DataValue::Describe(item.GetKind())));
          } else if (read.FindAction(action) == nullptr)
          {
            reader.Report(item, std::format("the state '{}' names the action '{}', which there is not", name, action));
          } else
          {
            state.actions.push_back(action);
          }
        }

        read.Add(state);
      }

      if (states->GetEntries().empty()) { reader.Report(*states, "'states' is empty, a map has at least one state"); }
    }

    reader.Finish();

    if (errors.size() > before) { return false; }

    map = read;
    return true;
  }
} // neon
