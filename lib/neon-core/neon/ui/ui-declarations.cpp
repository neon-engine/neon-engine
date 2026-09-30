#include "ui-declarations.hpp"

#include <algorithm>
#include <cctype>
#include <format>

#include <neon/data/data-reader.hpp>

#include "css-values.hpp"
#include "ui-properties.hpp"

namespace neon
{
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

    std::string Lowered(std::string text)
    {
      std::ranges::transform(text, text.begin(), [](const unsigned char letter)
      {
        return static_cast<char>(std::tolower(letter));
      });
      return text;
    }

    /// Whether a property holds colours, and the names of colours in its
    /// value are to be read.
    bool HoldsColors(const std::string &css_name)
    {
      return css_name.find("color") != std::string::npos || css_name == "border" || css_name == "outline" ||
             css_name == "background" || css_name.ends_with("shadow") || css_name.find("decoration") !=
             std::string::npos;
    }

    /// Whether a property holds a path, which a style sheet writes in
    /// `url()`.
    bool IsWholly(const std::string &value, const std::string &function)
    {
      if (value.size() < function.size() + 2 || value.back() != ')') { return false; }
      if (Lowered(value.substr(0, function.size() + 1)) != function + "(") { return false; }

      // one pair of brackets, which closes at the end
      int depth = 0;
      for (std::size_t i = function.size(); i < value.size(); i++)
      {
        if (value[i] == '(') { depth++; }
        if (value[i] == ')')
        {
          depth--;
          if (depth == 0 && i + 1 < value.size()) { return false; }
        }
      }
      return depth == 0;
    }

    std::string Unquoted(const std::string &text)
    {
      const std::string trimmed = Trimmed(text);

      if (trimmed.size() >= 2 && (trimmed.front() == '"' || trimmed.front() == '\'') &&
          trimmed.back() == trimmed.front())
      {
        // one text in quotes, and not two with something between them
        const std::string inside = trimmed.substr(1, trimmed.size() - 2);
        if (inside.find(trimmed.front()) == std::string::npos) { return inside; }
      }

      return trimmed;
    }

    /// The first family of a list of families, without its quotes. The
    /// others stand in for it in CSS, which is not done here.
    std::string FirstFamily(const std::string &value)
    {
      std::string first;
      char quote = '\0';

      for (const char letter : value)
      {
        if (quote != '\0')
        {
          if (letter == quote) { quote = '\0'; }
          first += letter;
        } else if (letter == '"' || letter == '\'')
        {
          quote = letter;
          first += letter;
        } else if (letter == ',')
        {
          break;
        } else
        {
          first += letter;
        }
      }

      return Unquoted(first);
    }

    /// The value of a style sheet as a file of YAML would hold it, which
    /// is what ReadUiStyle() reads.
    DataValue AsDataValue(const std::string &css_name, const std::string &value)
    {
      if (float number = 0.0f; ParseCssNumber(value, number)) { return DataValue::Number(number); }

      if (css_name == "font-family") { return DataValue::Text(FirstFamily(value)); }

      if (IsWholly(value, "url")) { return DataValue::Text(Unquoted(value.substr(4, value.size() - 5))); }

      return DataValue::Text(Unquoted(value));
    }

    /// Works out one value. For `font_size` itself, what an `em` and a
    /// percentage refer to is the size of the font of the element above.
    bool Resolve(
      const std::string &css_name,
      const std::string &value,
      const UiDeclarationContext &context,
      std::string &resolved,
      std::string &error,
      CssValueUses *uses)
    {
      // In a text, such as a path and the name of a font, a number with
      // a unit behind it is no length.
      const UiProperty *property = UiProperties::Get().Find(css_name);
      const bool is_text = property != nullptr && property->kind == UiValueKind::Text;

      if (is_text || value.find("://") != std::string::npos)
      {
        return SubstituteCssVariables(value, context.values, resolved, error, uses);
      }

      if (css_name != "font-size") { return ResolveCssValue(value, context.values, resolved, error, uses); }

      CssValueContext of_parent = context.values;
      of_parent.font_size = context.parent_font_size;

      CssValueUses used;
      if (!ResolveCssValue(value, of_parent, resolved, error, &used)) { return false; }

      // what refers to the size of the font of the element above is
      // worked out again when that changes, as what is inherited is
      if (uses != nullptr)
      {
        uses->root_font_size = uses->root_font_size || used.root_font_size;
        uses->viewport = uses->viewport || used.viewport;
        uses->variables = uses->variables || used.variables;
      }

      if (!resolved.empty() && resolved.back() == '%')
      {
        if (float percent = 0.0f; ParseCssNumber(resolved.substr(0, resolved.size() - 1), percent))
        {
          resolved = FormatCssNumber(context.parent_font_size * percent / 100.0f) + "px";
        }
      }

      return true;
    }

    /// A message of a reader without the document and the line in front,
    /// and with the name of the property as CSS writes it.
    std::string AsCssMessage(const std::string &message, const std::string &yaml_name, const std::string &css_name)
    {
      std::string text = message;

      if (const std::size_t start = text.find(": "); start != std::string::npos && start < 2)
      {
        text = text.substr(start + 2);
      }

      const std::string written = "'" + yaml_name + "'";
      if (text.starts_with(written)) { text = "'" + css_name + "'" + text.substr(written.size()); }

      return text;
    }

    std::string NotKnown(const std::string &css_name, const std::string &where)
    {
      std::string message = std::format("'{}' of {} is not a property that is known", css_name, where);

      const auto similar = UiProperties::GetSimilarNames(css_name);
      if (!similar.empty())
      {
        message += ". Near to it are: ";
        for (std::size_t i = 0; i < similar.size(); i++)
        {
          if (i > 0) { message += ", "; }
          message += similar[i];
        }
      }

      return message;
    }
  }

  bool IsCssWideKeyword(const std::string &value)
  {
    const std::string word = Lowered(Trimmed(value));
    return word == "inherit" || word == "initial" || word == "unset" || word == "revert";
  }

  bool ApplyUiKeyword(
    const std::string &name,
    const std::string &keyword,
    const UiDeclarationContext &context,
    UiStyle &style)
  {
    const UiProperties &properties = UiProperties::Get();
    const UiProperty *property = properties.Find(name);
    if (property == nullptr) { return false; }

    const std::string word = Lowered(Trimmed(keyword));
    const UiStyle initial;

    for (const UiProperty *each : properties.GetLonghands(*property))
    {
      const bool inherits = word == "inherit" || (word == "unset" && each->is_inherited);

      const UiStyle *from = &initial;

      if (word == "revert")
      {
        // what the engine gives the element, which for a property it
        // says nothing about is what an element without a style has
        if (context.defaults != nullptr) { from = context.defaults; }
      } else if (inherits && context.parent != nullptr)
      {
        from = context.parent;
      }

      each->Copy(*from, style);
    }

    // `overflow` is kept next to the two it stands for
    if (property->name == "overflow") { style.overflow = style.overflow_x; }

    return true;
  }

  bool ApplyUiDeclaration(
    const std::string &name,
    const std::string &value,
    const UiDeclarationContext &context,
    const std::string &where,
    UiStyle &style,
    std::vector<std::string> &problems,
    CssValueUses *uses)
  {
    const std::string css_name = UiProperties::ToCssName(Lowered(Trimmed(name)));
    const std::string yaml_name = UiProperties::ToYamlName(css_name);

    if (!UiProperties::IsStyleName(yaml_name))
    {
      problems.push_back(NotKnown(css_name, where));
      return false;
    }

    if (IsCssWideKeyword(value))
    {
      if (!ApplyUiKeyword(css_name, value, context, style))
      {
        problems.push_back(std::format(
          "'{}' of {} is '{}', which is not known for this property: it is missing in the table of properties",
          css_name, where, Lowered(Trimmed(value))));
        return false;
      }
      return true;
    }

    std::string resolved;
    if (std::string error; !Resolve(css_name, value, context, resolved, error, uses))
    {
      problems.push_back(std::format("'{}' of {} is '{}': {}", css_name, where, Trimmed(value), error));
      return false;
    }

    if (context.reads_color_names && HoldsColors(css_name)) { resolved = ResolveCssColors(resolved); }

    DataValue map = DataValue::Map();
    map.Set(yaml_name, AsDataValue(css_name, resolved));

    std::vector<std::string> errors;
    const DataReader reader(map, "", where, errors);

    // into a copy, so that a value that cannot be read changes nothing
    UiStyle changed = style;
    ReadUiStyle(reader, changed);

    if (!errors.empty())
    {
      for (const auto &error : errors) { problems.push_back(AsCssMessage(error, yaml_name, css_name)); }
      return false;
    }

    style = std::move(changed);
    return true;
  }

  bool CheckUiDeclaration(
    const std::string &name,
    const std::string &value,
    const std::string &where,
    std::vector<std::string> &problems)
  {
    const std::string css_name = UiProperties::ToCssName(Lowered(Trimmed(name)));

    if (!UiProperties::IsStyleName(UiProperties::ToYamlName(css_name)))
    {
      problems.push_back(NotKnown(css_name, where));
      return false;
    }

    // what a custom property holds is known when the element is
    if (HasCssVariables(value)) { return true; }

    const UiDeclarationContext context;
    UiStyle style;
    return ApplyUiDeclaration(name, value, context, where, style, problems);
  }

  UiPreparedProperties PrepareUiProperties(
    const DataValue &map,
    const UiDeclarationContext &context,
    const bool checks_only,
    const std::string &document,
    const std::string &where,
    std::vector<std::string> &problems)
  {
    UiPreparedProperties prepared;
    prepared.map.SetLine(map.GetLine());

    if (!map.IsMap())
    {
      prepared.map = map;
      return prepared;
    }

    const auto report = [&](const DataValue &value, const std::string &message)
    {
      if (value.GetLine() > 0)
      {
        problems.push_back(std::format("{}:{}: {}", document, value.GetLine(), message));
      } else
      {
        problems.push_back(std::format("{}: {}", document, message));
      }
    };

    for (const auto &[name, value] : map.GetEntries())
    {
      // what is inside the element is read by itself
      if (name == "children")
      {
        auto placeholder = DataValue::List();
        placeholder.SetLine(value.GetLine());
        prepared.map.Set(name, placeholder);
        continue;
      }

      std::string text;
      const bool is_text = value.GetText(text);

      if (name.starts_with("--"))
      {
        if (float number = 0.0f; !is_text && value.GetNumber(number)) { text = FormatCssNumber(number); }
        prepared.variables.emplace_back(name, text);
        continue;
      }

      if (!is_text || !UiProperties::IsStyleName(name))
      {
        prepared.map.Set(name, value);
        continue;
      }

      if (IsCssWideKeyword(text))
      {
        if (UiProperties::Get().Find(name) == nullptr)
        {
          report(value, std::format(
                   "'{}' of {} is '{}', which is not known for this property: it is missing in the table of "
                   "properties",
                   name, where, Lowered(Trimmed(text))));
        } else
        {
          prepared.keywords.emplace_back(name, Lowered(Trimmed(text)));
        }
        continue;
      }

      if (checks_only && HasCssVariables(text))
      {
        prepared.uses.variables = true;
        continue;
      }

      std::string resolved;
      std::string error;

      if (!Resolve(UiProperties::ToCssName(name), text, context, resolved, error, &prepared.uses))
      {
        report(value, std::format("'{}' of {} is '{}': {}", name, where, text, error));
        continue;
      }

      if (context.reads_color_names && HoldsColors(UiProperties::ToCssName(name)))
      {
        resolved = ResolveCssColors(resolved);
      }

      // Text stays text, so that what a file writes in quotes is read as
      // it was before. What was worked out to a number is one.
      DataValue worked_out = DataValue::Text(resolved);

      if (resolved != text)
      {
        if (float number = 0.0f; ParseCssNumber(resolved, number)) { worked_out = DataValue::Number(number); }
      }

      worked_out.SetLine(value.GetLine());
      prepared.map.Set(name, worked_out);
    }

    return prepared;
  }

  void ApplyUiProperties(
    const UiPreparedProperties &prepared,
    const UiDeclarationContext &context,
    const std::string &document,
    const std::string &where,
    UiStyle &style,
    std::vector<std::string> &problems)
  {
    const DataReader reader(prepared.map, document, where, problems);
    ReadUiStyle(reader, style);

    for (const auto &[name, keyword] : prepared.keywords) { ApplyUiKeyword(name, keyword, context, style); }
  }
} // neon
