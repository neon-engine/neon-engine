#include "ryml-document-format.hpp"

#include <algorithm>
#include <format>
#include <stdexcept>

#include <ryml.hpp>
#include <ryml_std.hpp>

namespace neon
{
  namespace
  {
    /// What rapidyaml reports. It must not return to rapidyaml, so it is
    /// thrown and caught in Read.
    struct ParseError final : std::runtime_error
    {
      std::size_t line;

      ParseError(const std::string &message, const std::size_t line)
        : std::runtime_error(message), line(line) {}
    };

    [[noreturn]] void OnError(
      const char *message,
      const std::size_t length,
      const ryml::Location location,
      void *)
    {
      // rapidyaml follows the reason with the text around it, which the line
      // number stands for here
      std::string reason(message, length);
      if (const auto end = reason.find('\n'); end != std::string::npos) { reason.erase(end); }
      if (reason.starts_with("ERROR: ")) { reason.erase(0, 7); }

      throw ParseError(reason, location.line);
    }

    /// Whether a scalar is written the way a person writes a number: digits,
    /// a sign, a dot, an exponent. YAML also allows hexadecimal and octal,
    /// which are the computer's forms and are text here, so that 0x1d is
    /// never a number by surprise. A field that wants one, such as a colour,
    /// reads the text.
    bool IsDecimal(const ryml::csubstr text)
    {
      return text.first_not_of("0123456789.+-eE") == ryml::npos;
    }

    std::string ToString(const ryml::csubstr text)
    {
      return text.empty() ? std::string{} : std::string(text.str, text.len);
    }

    [[noreturn]] void Refuse(const std::string &what, const std::size_t line)
    {
      throw ParseError(what, line);
    }

    DataValue Convert(const ryml::Parser &parser, const ryml::ConstNodeRef &node)
    {
      // rapidyaml counts the lines of nodes from 0
      const std::size_t line = parser.location(node).line + 1;

      if (node.has_val_anchor() || node.has_key_anchor() || node.is_val_ref() || node.is_key_ref())
      {
        Refuse("anchors and aliases are not supported", line);
      }

      if (node.has_val_tag() || node.has_key_tag())
      {
        Refuse("tags are not supported", line);
      }

      DataValue value;

      if (node.is_map())
      {
        value = DataValue::Map();
        for (const auto child : node.children())
        {
          const auto name = ToString(child.key());
          if (value.Find(name) != nullptr)
          {
            Refuse("'" + name + "' is written twice", parser.location(child).line + 1);
          }
          value.Set(name, Convert(parser, child));
        }
      } else if (node.is_seq())
      {
        value = DataValue::List();
        for (const auto child : node.children())
        {
          value.Add(Convert(parser, child));
        }
      } else if (node.has_val())
      {
        const auto text = node.val();

        if (node.is_val_quoted())
        {
          value = DataValue::Text(ToString(text));
        } else if (node.val_is_null())
        {
          value = DataValue{};
        } else if (text == "true")
        {
          value = DataValue::Bool(true);
        } else if (text == "false")
        {
          value = DataValue::Bool(false);
        } else if (double number = 0.0; IsDecimal(text) && text.is_number() && ryml::from_chars(text, &number))
        {
          value = DataValue::Number(number);
        } else if (const char suffix = text.empty() ? '\0' : text.back(); suffix == 'f' || suffix == 'd')
        {
          // A number with a fraction that a person marked with its precision,
          // for the people who read the file: 0.5f is a float and 2.0d a
          // double. The engine never writes them, it knows the type of what
          // it writes. A whole number takes no suffix, so 2d is text.
          const auto digits = text.first(text.len - 1);
          const bool fraction = digits.first_of(".eE") != ryml::npos;
          if (fraction && IsDecimal(digits) && digits.is_number() && ryml::from_chars(digits, &number))
          {
            value = suffix == 'f' ? DataValue::Number(static_cast<float>(number)) : DataValue::PreciseNumber(number);
          } else
          {
            value = DataValue::Text(ToString(text));
          }
        } else
        {
          value = DataValue::Text(ToString(text));
        }
      }

      value.SetLine(line);
      return value;
    }

    bool IsNumberList(const DataValue &value)
    {
      return value.IsList() && !value.GetItems().empty() && std::ranges::all_of(
        value.GetItems(),
        [](const DataValue &item) { return item.GetKind() == DataValue::Kind::Number; });
    }

    std::string WriteNumber(const DataValue &value)
    {
      double number = 0.0;
      (void) value.GetNumber(number);

      // the shortest text that reads back as the same number
      return value.IsSinglePrecision()
        ? std::format("{}", static_cast<float>(number))
        : std::format("{}", number);
    }

    /// Whether text reads back as the same text when it is written without
    /// quotes.
    bool IsPlain(const std::string &text)
    {
      if (text.empty()) { return false; }
      if (text == "true" || text == "false" || text == "null" || text == "~") { return false; }
      if (text.front() == ' ' || text.back() == ' ') { return false; }
      if (ryml::to_csubstr(text).is_number()) { return false; }

      // what starts a value of another kind, or means something to YAML
      if (std::string_view("-?:,[]{}#&*!|>'\"%@`").find(text.front()) != std::string_view::npos)
      {
        return false;
      }

      for (std::size_t i = 0; i < text.size(); i++)
      {
        const char c = text[i];
        if (static_cast<unsigned char>(c) < 0x20) { return false; }
        if (c == '#' && text[i - 1] == ' ') { return false; }
        if (c == ':' && (i + 1 == text.size() || text[i + 1] == ' ')) { return false; }
      }

      return true;
    }

    std::string WriteText(const std::string &text)
    {
      if (IsPlain(text)) { return text; }

      std::string quoted = "\"";
      for (const char c : text)
      {
        switch (c)
        {
          case '"': quoted += "\\\""; break;
          case '\\': quoted += "\\\\"; break;
          case '\n': quoted += "\\n"; break;
          case '\r': quoted += "\\r"; break;
          case '\t': quoted += "\\t"; break;
          default:
            if (static_cast<unsigned char>(c) < 0x20)
            {
              quoted += std::format("\\x{:02x}", static_cast<unsigned char>(c));
            } else
            {
              quoted += c;
            }
        }
      }
      return quoted + "\"";
    }

    /// A value that fits on the line of its name, or an empty text when it
    /// needs lines of its own.
    std::string WriteInline(const DataValue &value)
    {
      switch (value.GetKind())
      {
        case DataValue::Kind::Empty: return "null";
        case DataValue::Kind::Bool:
        {
          bool result = false;
          (void) value.GetBool(result);
          return result ? "true" : "false";
        }
        case DataValue::Kind::Number: return WriteNumber(value);
        case DataValue::Kind::Text:
        {
          std::string text;
          (void) value.GetText(text);
          return WriteText(text);
        }
        case DataValue::Kind::List:
        {
          if (value.GetItems().empty()) { return "[]"; }
          if (!IsNumberList(value)) { return {}; }

          std::string line = "[";
          for (const auto &item : value.GetItems())
          {
            if (line.size() > 1) { line += ", "; }
            line += WriteNumber(item);
          }
          return line + "]";
        }
        case DataValue::Kind::Map: return value.GetEntries().empty() ? "{}" : std::string{};
      }
      return {};
    }

    void WriteBlock(const DataValue &value, std::size_t level, bool in_list, std::string &text);

    void WriteEntry(
      const std::string &prefix,
      const DataValue &value,
      const std::size_t level,
      std::string &text)
    {
      if (const auto line = WriteInline(value); !line.empty())
      {
        text += prefix + " " + line + "\n";
        return;
      }

      text += prefix + "\n";
      WriteBlock(value, level + 1, false, text);
    }

    void WriteBlock(const DataValue &value, const std::size_t level, const bool in_list, std::string &text)
    {
      const std::string indent(level * 2, ' ');

      if (value.IsMap())
      {
        bool first = true;
        for (const auto &[name, entry] : value.GetEntries())
        {
          // the first name of a map in a list follows the dash of the list
          const std::string start = first && in_list ? std::string{} : indent;

          // a blank line sets apart what is long, at the top of the document
          if (level == 0 && !first && WriteInline(entry).empty()) { text += "\n"; }

          WriteEntry(start + WriteText(name) + ":", entry, level, text);
          first = false;
        }
        return;
      }

      bool first = true;
      for (const auto &item : value.GetItems())
      {
        if (const auto line = WriteInline(item); !line.empty())
        {
          text += indent + "- " + line + "\n";
        } else if (item.IsMap())
        {
          // and the maps of a list that is right below the top, which are
          // the entities of a scene
          if (level == 1 && !first) { text += "\n"; }
          first = false;

          text += indent + "- ";
          WriteBlock(item, level + 1, true, text);
        } else
        {
          // a list in a list
          text += indent + "-\n";
          WriteBlock(item, level + 1, false, text);
        }
      }
    }
  }

  bool RYML_DocumentFormat::Read(
    const std::string &name,
    const std::string &text,
    DataValue &document,
    std::string &error)
  {
    const ryml::Callbacks callbacks(nullptr, nullptr, nullptr, OnError);

    try
    {
      ryml::EventHandlerTree handler(callbacks);
      ryml::Parser parser(&handler, ryml::ParserOptions().locations(true));
      ryml::Tree tree(callbacks);

      ryml::parse_in_arena(&parser, ryml::to_csubstr(name), ryml::to_csubstr(text), &tree);

      const auto root = tree.crootref();
      if (root.is_stream())
      {
        Refuse("several documents in one text are not supported", 1);
      }

      document = Convert(parser, root);
      return true;
    } catch (const ParseError &e)
    {
      error = std::format("{}:{}: {}", name, e.line, e.what());
      return false;
    }
  }

  std::string RYML_DocumentFormat::Write(const DataValue &document)
  {
    if (const auto line = WriteInline(document); !line.empty())
    {
      return line + "\n";
    }

    std::string text;
    WriteBlock(document, 0, false, text);
    return text;
  }
} // neon
