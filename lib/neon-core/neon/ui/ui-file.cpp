#include "ui-file.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <map>

namespace neon
{
  namespace
  {
    const std::vector<std::string> scale_modes = {"fit", "width", "height", "none"};

    // in the order a later one wins over an earlier one
    const std::vector<std::string> state_names = {"focus", "hover", "active", "disabled"};

    /// The line a message is about, which follows the name of the file.
    /// 0 for a message that names none.
    std::size_t LineOf(const std::string &message, const std::string &path)
    {
      if (!message.starts_with(path + ":")) { return 0; }

      std::size_t line = 0;
      for (std::size_t i = path.size() + 1; i < message.size() && message[i] >= '0' && message[i] <= '9'; i++)
      {
        line = line * 10 + static_cast<std::size_t>(message[i] - '0');
      }
      return line;
    }

    std::string Join(const std::vector<std::string> &words)
    {
      std::string joined;
      for (const auto &word : words)
      {
        if (!joined.empty()) { joined += ", "; }
        joined += word;
      }
      return joined;
    }

    /// The states that make up each of the styles of an element, in the
    /// order of UiElement::StyleIndex.
    UiStates StatesOf(const std::size_t index)
    {
      UiStates states;

      if (index == UiElement::kStyle_Count - 1)
      {
        states.disabled = true;
        return states;
      }

      states.focus = (index & 1) != 0;
      states.hover = (index & 2) != 0;
      states.active = (index & 4) != 0;
      return states;
    }

    bool Has(const UiStates &states, const std::string &name)
    {
      if (name == "focus") { return states.focus; }
      if (name == "hover") { return states.hover; }
      if (name == "active") { return states.active; }
      return states.disabled;
    }
  }

  struct UiFile::Reading
  {
    std::string path;
    std::vector<std::string> *errors = nullptr;

    // the names of the elements, with the line each was first written at
    std::map<std::string, std::size_t> names;
  };

  UiFile::UiFile(FileSystemContext *file_system, DocumentFormat *format, const UiElementTypes *types)
  {
    _file_system = file_system;
    _format = format;
    _types = types;
  }

  std::unique_ptr<UiDocument> UiFile::Read(const std::string &path, std::vector<std::string> &errors) const
  {
    const std::size_t errors_before = errors.size();

    std::string text;
    if (!_file_system->ReadText(path, text))
    {
      errors.push_back(std::format("{}: the file cannot be read", path));
      return nullptr;
    }

    DataValue file;
    if (std::string error; !_format->Read(path, text, file, error))
    {
      errors.push_back(error);
      return nullptr;
    }

    auto document = std::make_unique<UiDocument>();
    document->path = path;

    const DataReader reader(file, path, "the user interface", errors);
    ReadTop(reader, *document);

    Reading reading;
    reading.path = path;
    reading.errors = &errors;

    if (const auto *root = reader.ReadValue("root"); root != nullptr)
    {
      document->root = ReadElement(*root, "the root", reading);
    } else
    {
      reader.Report("the user interface has no 'root', where an element was expected");
    }

    reader.Finish();

    if (errors.size() > errors_before)
    {
      // from the top of the file to its end, so that a file is corrected in
      // one pass
      std::stable_sort(
        errors.begin() + static_cast<std::ptrdiff_t>(errors_before),
        errors.end(),
        [&path](const std::string &a, const std::string &b) { return LineOf(a, path) < LineOf(b, path); });

      return nullptr;
    }

    return document;
  }

  void UiFile::ReadTop(const DataReader &reader, UiDocument &document)
  {
    reader.Read("ui", document.name);

    if (float written = 0.0f; reader.Read("version", written) && written > static_cast<float>(version))
    {
      reader.Report(*reader.ReadValue("version"), std::format(
                      "the user interface has version {}, and this engine reads up to version {}",
                      written, version));
    }

    reader.Read("modal", document.modal);

    std::size_t mode = 0;
    if (reader.ReadChoice("scale", scale_modes, mode)) { document.scale_mode = static_cast<UiScaleMode>(mode); }

    if (const auto *size = reader.ReadValue("reference_size"); size != nullptr)
    {
      float width = 0.0f;
      float height = 0.0f;

      if (size->IsList() && size->GetItems().size() == 2 &&
          size->GetItems()[0].GetNumber(width) && size->GetItems()[1].GetNumber(height) &&
          width > 0.0f && height > 0.0f)
      {
        document.reference_width = width;
        document.reference_height = height;
      } else
      {
        reader.Report(*size, std::format(
                        "'reference_size' of the user interface is {}, where a list of 2 numbers above 0 "
                        "was expected, such as [1920, 1080]",
                        size->IsList() ? "another list" : DataValue::Describe(size->GetKind())));
      }
    }

    if (const auto *fonts = reader.ReadValue("fonts"); fonts != nullptr)
    {
      if (!fonts->IsList())
      {
        reader.Report(*fonts, std::format(
                        "'fonts' of the user interface is {}, where a list was expected",
                        DataValue::Describe(fonts->GetKind())));
      } else
      {
        std::size_t number = 1;
        for (const auto &font : fonts->GetItems())
        {
          const DataReader font_reader(font, reader.GetDocument(), std::format("font {}", number), reader.GetErrors());
          number++;

          UiFontFace face;
          const bool has_family = font_reader.Read("family", face.family);
          const bool has_source = font_reader.Read("src", face.path);

          if (float weight = 400.0f; font_reader.Read("weight", weight))
          {
            if (weight >= 1.0f && weight <= 1000.0f)
            {
              face.weight = static_cast<int>(std::round(weight));
            } else
            {
              font_reader.Report(*font_reader.ReadValue("weight"), std::format(
                                   "'weight' of {} is {}, where a number from 1 to 1000 was expected",
                                   font_reader.GetWhere(), weight));
            }
          }

          if (!has_family && !font_reader.Has("family"))
          {
            font_reader.Report(std::format(
              "{} has no 'family', where the name it is asked for by was expected", font_reader.GetWhere()));
          }

          if (!has_source && !font_reader.Has("src"))
          {
            font_reader.Report(std::format(
              "{} has no 'src', where the virtual path of a font was expected", font_reader.GetWhere()));
          }

          font_reader.Finish();

          if (has_family && has_source) { document.fonts.push_back(face); }
        }
      }
    }

    if (const auto *values = reader.ReadValue("values"); values != nullptr)
    {
      if (!values->IsMap())
      {
        reader.Report(*values, std::format(
                        "'values' of the user interface is {}, where a map was expected",
                        DataValue::Describe(values->GetKind())));
      } else
      {
        for (const auto &[name, value] : values->GetEntries())
        {
          double number = 0.0;
          bool flag = false;
          std::string text;

          if (value.GetNumber(number))
          {
            document.values.emplace_back(name, UiValue::Number(number));
          } else if (value.GetBool(flag))
          {
            document.values.emplace_back(name, UiValue::Flag(flag));
          } else if (value.GetText(text))
          {
            document.values.emplace_back(name, UiValue::Text(text));
          } else
          {
            reader.Report(value, std::format(
                            "value '{}' of the user interface is {}, where a number, text, true, or false "
                            "was expected",
                            name, DataValue::Describe(value.GetKind())));
          }
        }
      }
    }
  }

  std::unique_ptr<UiElement> UiFile::ReadElement(
    const DataValue &value,
    const std::string &label,
    Reading &reading) const
  {
    std::string type;
    std::string name;

    if (const auto *written = value.Find("type"); written != nullptr) { (void) written->GetText(type); }
    if (const auto *written = value.Find("name"); written != nullptr) { (void) written->GetText(name); }

    // an element is called by its name in messages, and by its place in
    // the file when it has none
    const std::string kind = type.empty() ? "element" : type;
    const std::string where = name.empty() ? label : kind + " '" + name + "'";
    const DataReader reader(value, reading.path, where, *reading.errors);

    reader.Read("type", type);
    reader.Read("name", name);

    auto element = _types->CreateElement(type);
    if (element == nullptr)
    {
      if (const auto *written = reader.ReadValue("type"); written != nullptr)
      {
        reader.Report(*written, std::format(
                        "type '{}' of {} is not known. Known are: {}",
                        type, where, Join(_types->GetNames())));
      } else
      {
        reader.Report(std::format(
          "{} has no 'type', where one of these was expected: {}", where, Join(_types->GetNames())));
      }

      // what is wrong below it is still found
      if (const auto *children = value.Find("children"); children != nullptr && children->IsList())
      {
        std::size_t number = 1;
        for (const auto &child : children->GetItems())
        {
          (void) ReadElement(child, std::format("child {} of {}", number, where), reading);
          number++;
        }
      }
      return nullptr;
    }

    element->SetIdentity(type, name, value.GetLine());

    if (!name.empty())
    {
      if (const auto [known, is_new] = reading.names.emplace(name, value.GetLine()); !is_new)
      {
        reader.Report(std::format(
          "{} shares its name with the element at line {}. A name says which element an event is from, "
          "and is given once",
          where, known->second));
      }
    }

    if (const auto *hidden = reader.ReadValue("hidden"); hidden != nullptr)
    {
      if (UiFlag flag; UiFlag::Read(*hidden, flag))
      {
        element->SetHidden(flag);
      } else
      {
        std::string text;
        reader.Report(*hidden, std::format(
                        "'hidden' of {} is {}, where true, false, or a value such as \"{{paused}}\" was expected",
                        where,
                        hidden->GetText(text) ? "'" + text + "'" : DataValue::Describe(hidden->GetKind())));
      }
    }

    element->ReadAttributes(reader);
    ReadStyles(value, reader, *element, reading);

    if (element->TakesChildren())
    {
      if (const auto *children = reader.ReadValue("children"); children != nullptr)
      {
        if (!children->IsList())
        {
          reader.Report(*children, std::format(
                          "'children' of {} is {}, where a list was expected",
                          where, DataValue::Describe(children->GetKind())));
        } else
        {
          if (element->HasContent() && !children->GetItems().empty())
          {
            reader.Report(*children, std::format(
                            "{} has both 'text' and 'children', where one of them was expected", where));
          }

          std::size_t number = 1;
          for (const auto &child : children->GetItems())
          {
            if (auto read = ReadElement(child, std::format("child {} of {}", number, where), reading);
              read != nullptr)
            {
              element->AddChild(std::move(read));
            }
            number++;
          }
        }
      }
    }

    reader.Finish();
    return element;
  }

  void UiFile::ReadStyles(
    const DataValue &value,
    const DataReader &reader,
    UiElement &element,
    Reading &reading) const
  {
    // what is wrong with a property is reported once, by the readers that
    // write to the errors of the file. Every other time the properties
    // are read, the messages go nowhere
    std::vector<std::string> unheard;
    const DataReader silent(value, reading.path, reader.GetWhere(), unheard);

    const DataValue nothing = DataValue::Map();
    const DataValue *states[4] = {nullptr, nullptr, nullptr, nullptr};

    for (std::size_t i = 0; i < state_names.size(); i++)
    {
      bool is_written = false;
      const DataReader state_reader = reader.ReadMap(state_names[i], is_written);
      if (!is_written) { continue; }

      const auto *written = value.Find(state_names[i]);
      if (written == nullptr || !written->IsMap()) { continue; }

      states[i] = written;

      UiStyle checked;
      ReadUiStyle(state_reader, checked);
      state_reader.Finish();
    }

    for (std::size_t index = 0; index < UiElement::kStyle_Count; index++)
    {
      const UiStates of_index = StatesOf(index);

      // As in CSS, what the engine says about an element comes first, and
      // what the file says wins over it. Within each, a state wins over
      // the element as it is.
      UiStyle style;
      element.ApplyDefaults(style);
      element.ApplyStateDefaults(style, of_index);

      ReadUiStyle(index == 0 ? reader : silent, style);

      for (std::size_t i = 0; i < state_names.size(); i++)
      {
        if (states[i] == nullptr || !Has(of_index, state_names[i])) { continue; }

        const DataReader state_reader(*states[i], reading.path, reader.GetWhere(), unheard);
        ReadUiStyle(state_reader, style);
      }

      element.SetStyle(index, style);
    }

    // the names of the properties count as asked for by now, so that
    // Finish() of the reader reports what is left
    (void) silent;
  }
} // neon
