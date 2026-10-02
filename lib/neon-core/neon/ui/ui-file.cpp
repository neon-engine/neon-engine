#include "ui-file.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <map>

#include "ui-declarations.hpp"
#include "ui-properties.hpp"

namespace neon
{
  // Helpers of UiFile, for this file alone.
  namespace
  {
    const std::vector<std::string> scale_modes = {"fit", "width", "height", "none"};

    const std::vector<std::string> font_styles = {"normal", "italic"};
    const std::vector<std::string> font_renderings = {"bitmap", "sdf"};

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

    /// What stands in for the values of an element while a file is read:
    /// the units are worked out against what a browser starts with, which
    /// says whether a value can be read and not what it comes to.
    UiDeclarationContext ContextOfChecks()
    {
      UiDeclarationContext context;
      context.reads_color_names = false;
      return context;
    }

    /// Whether a description holds `${name}` somewhere, which is filled
    /// in when a copy of a template is made.
    bool HasPlaceholders(const DataValue &description)
    {
      if (std::string text; description.GetText(text)) { return text.find("${") != std::string::npos; }

      for (const auto &item : description.GetItems())
      {
        if (HasPlaceholders(item)) { return true; }
      }

      for (const auto &[name, value] : description.GetEntries())
      {
        if (HasPlaceholders(value)) { return true; }
      }

      return false;
    }

    /// The classes of an element: text with spaces between them, or a
    /// list. Returns false for anything else.
    bool ReadClasses(const DataValue &value, std::vector<std::string> &classes)
    {
      std::vector<std::string> read;

      const auto add = [&read](const std::string &text)
      {
        std::string name;
        for (const char letter : text + " ")
        {
          if (letter == ' ' || letter == '\t')
          {
            if (!name.empty()) { read.push_back(name); }
            name.clear();
          } else
          {
            name += letter;
          }
        }
      };

      if (std::string text; value.GetText(text))
      {
        add(text);
      } else if (value.IsList())
      {
        for (const auto &item : value.GetItems())
        {
          std::string each;
          if (!item.GetText(each)) { return false; }
          add(each);
        }
      } else
      {
        return false;
      }

      for (const auto &name : read)
      {
        // what a selector can name behind a dot
        const auto first = static_cast<unsigned char>(name[0]);
        if (std::isdigit(first) != 0 || name.find_first_of(".#:[]>+~,*()\"'") != std::string::npos)
        {
          return false;
        }
      }

      classes = read;
      return true;
    }
  }

  struct UiFile::Reading
  {
    std::string path;
    std::vector<std::string> *errors = nullptr;

    // the names of the elements, with the line each was first written at
    std::map<std::string, std::size_t> names;

    // copies of a template share their names, which are then not held to
    // be given once
    bool is_template = false;
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

    ReadStylesAndTemplates(reader, *document, reading);

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

  void UiFile::ReadStylesAndTemplates(const DataReader &reader, UiDocument &document, Reading &reading) const
  {

    if (const auto *styles = reader.ReadValue("styles"); styles != nullptr)
    {
      bool is_read = true;

      if (std::string one; styles->GetText(one))
      {
        document.style_paths.push_back(one);
      } else if (styles->IsList())
      {
        for (const auto &item : styles->GetItems())
        {
          std::string each;
          if (!item.GetText(each) || each.empty()) { is_read = false; }
          document.style_paths.push_back(each);
        }
      } else
      {
        is_read = false;
      }

      if (!is_read)
      {
        document.style_paths.clear();
        reader.Report(*styles, std::format(
                        "'styles' of the user interface is {}, where a list of the virtual paths of style "
                        "sheets was expected, such as [assets://ui/theme.css]",
                        styles->IsList() ? "another list" : DataValue::Describe(styles->GetKind())));
      } else
      {
        std::vector<std::string> sheet_errors;
        document.sheets.Load(document.style_paths, document.path, *_file_system, sheet_errors, document.warnings);

        // a sheet that cannot be read has no line of its own, and is
        // reported where the file names it
        for (const auto &error : sheet_errors)
        {
          const std::string front = document.path + ": ";
          reader.Report(*styles, error.starts_with(front) ? error.substr(front.size()) : error);
        }
      }
    }

    const auto read_cancel = [&]
    {
      const std::vector<std::string> cancels = {"auto", "close", "blur", "none"};
      if (std::size_t cancel = 0; reader.ReadChoice("cancel", cancels, cancel))
      {
        document.cancel = static_cast<UiCancel>(cancel);
      }
    };

    if (const auto *templates = reader.ReadValue("templates"); templates != nullptr)
    {
      if (!templates->IsMap())
      {
        reader.Report(*templates, std::format(
                        "'templates' of the user interface is {}, where a map of elements by their names "
                        "was expected",
                        DataValue::Describe(templates->GetKind())));
        read_cancel();
        return;
      }

      for (const auto &[name, description] : templates->GetEntries())
      {
        // a template is read as what it is a template of, to find what is
        // wrong with it. Its names are its own, since copies share them
        Reading of_template;
        of_template.path = reading.path;
        of_template.errors = reading.errors;
        of_template.is_template = true;

        // What holds `${name}` is filled in when a copy is made, and is
        // read then: a number that is written as `${health}` is no
        // number yet.
        if (HasPlaceholders(description))
        {
          if (!description.IsMap() || description.Find("type") == nullptr)
          {
            reader.Report(description, std::format(
                            "template '{}' has no 'type', where one of these was expected: {}",
                            name, Join(_types->GetNames())));
          } else
          {
            document.templates.emplace_back(name, description);
          }
          continue;
        }

        if (ReadElement(description, std::format("template '{}'", name), of_template) != nullptr)
        {
          document.templates.emplace_back(name, description);
        }
      }
    }

    read_cancel();
  }

  std::unique_ptr<UiElement> UiFile::CreateElement(
    const DataValue &description,
    const std::string &document,
    std::vector<std::string> &errors) const
  {
    const std::size_t errors_before = errors.size();

    Reading reading;
    reading.path = document;
    reading.errors = &errors;
    reading.is_template = true;

    auto element = ReadElement(description, "the element", reading);

    if (errors.size() > errors_before) { return nullptr; }
    return element;
  }

  std::unique_ptr<UiElement> UiFile::CreateElementFromText(
    const std::string &text,
    const std::string &document,
    std::vector<std::string> &errors) const
  {
    DataValue description;
    if (std::string error; !_format->Read(document, text, description, error))
    {
      errors.push_back(error);
      return nullptr;
    }

    return CreateElement(description, document, errors);
  }

  bool UiFile::ReloadStyles(UiDocument &document, std::vector<std::string> &errors) const
  {
    UiStyleSheets sheets;
    sheets.SetEnvironment(document.sheets.GetEnvironment());

    std::vector<std::string> warnings;
    if (!sheets.Load(document.style_paths, document.path, *_file_system, errors, warnings)) { return false; }

    document.sheets = std::move(sheets);
    document.warnings = warnings;
    return true;
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

          if (std::size_t style = 0; font_reader.ReadChoice("style", font_styles, style))
          {
            face.options.is_italic = style == 1;
          }

          if (std::size_t rendering = 0; font_reader.ReadChoice("rendering", font_renderings, rendering))
          {
            face.options.rendering = rendering == 1 ? GlyphRendering::DistanceField : GlyphRendering::Bitmap;
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

    // The values of the properties are worked out before they are read:
    // units, and calc(). What holds a custom property is left for when the
    // element is known, and so is what takes its value from elsewhere.
    UiPreparedProperties prepared = PrepareUiProperties(
      value, ContextOfChecks(), true, reading.path, where, *reading.errors);

    for (const auto &state : state_names)
    {
      const DataValue *written = value.Find(state);
      if (written == nullptr || !written->IsMap()) { continue; }

      prepared.map.Set(state, PrepareUiProperties(
                         *written, ContextOfChecks(), true, reading.path,
                         std::format("'{}' of {}", state, where), *reading.errors).map);
    }

    const DataReader reader(prepared.map, reading.path, where, *reading.errors);

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

    if (!name.empty() && !reading.is_template)
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
    ReadStyles(prepared.map, reader, *element, reading);

    // what the cascade reads the properties from, without what is inside
    {
      DataValue written = DataValue::Map();
      written.SetLine(value.GetLine());

      if (value.IsMap())
      {
        for (const auto &[each, held] : value.GetEntries())
        {
          if (each != "children") { written.Set(each, held); }
        }
      }

      element->SetWritten(written, reading.path);
    }

    if (const auto *classes = reader.ReadValue("class"); classes != nullptr)
    {
      if (std::vector<std::string> read; ReadClasses(*classes, read))
      {
        element->SetClasses(read);
      } else
      {
        std::string text;
        reader.Report(*classes, std::format(
                        "'class' of {} is {}, where names of classes were expected, as text with spaces "
                        "between them or as a list",
                        where,
                        classes->GetText(text) ? "'" + text + "'" : DataValue::Describe(classes->GetKind())));
      }
    }

    if (std::string title; reader.Read("title", title)) { element->SetTitle(title); }

    if (bool draggable = false; reader.Read("draggable", draggable)) { (void) draggable; }

    // an element for every row of a list of the game
    if (const auto *list = reader.ReadValue("for_each"); list != nullptr)
    {
      std::string text;
      const bool names_a_list = list->GetText(text) && text.size() > 2 && text.front() == '{' &&
                                text.back() == '}' && text.find(' ') == std::string::npos;

      if (!names_a_list)
      {
        reader.Report(*list, std::format(
                        "'for_each' of {} is {}, where the name of a list was expected, such as \"{{items}}\"",
                        where,
                        list->GetText(text) ? "'" + text + "'" : DataValue::Describe(list->GetKind())));
      }

      std::string template_name;
      if (!reader.Read("template", template_name) && !reader.Has("template"))
      {
        reader.Report(*list, std::format(
                        "{} has 'for_each' and no 'template', where the name of the template its rows are "
                        "made from was expected",
                        where));
      }

      if (!element->TakesChildren())
      {
        reader.Report(*list, std::format("{} takes nothing inside, and cannot have 'for_each'", where));
      }
    } else if (const auto *lonely = reader.ReadValue("template"); lonely != nullptr)
    {
      reader.Report(*lonely, std::format(
                      "{} has 'template' and no 'for_each', where the list its rows are made for was expected",
                      where));
    }

    if (int tab_index = 0; reader.Read("tab_index", tab_index)) { element->SetTabIndex(tab_index); }

    if (element->TakesChildren())
    {
      // what is inside is read from the file as it is
      if (const auto *children = reader.ReadValue("children") != nullptr ? value.Find("children") : nullptr;
        children != nullptr)
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
    // What is wrong with a property is found here. What a property comes
    // to is worked out by the cascade, when the element is known: the
    // style sheets, what is above it, and the state it is in.
    for (const auto &state : state_names)
    {
      bool is_written = false;
      const DataReader state_reader = reader.ReadMap(state, is_written);
      if (!is_written) { continue; }

      const auto *written = value.Find(state);
      if (written == nullptr || !written->IsMap()) { continue; }

      UiStyle of_state;
      ReadUiStyle(state_reader, of_state);
      state_reader.Finish();
    }

    UiStyle checked;
    element.ApplyDefaults(checked);
    ReadUiStyle(reader, checked);
  }
} // neon
