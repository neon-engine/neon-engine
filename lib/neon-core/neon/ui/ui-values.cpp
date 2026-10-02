#include "ui-values.hpp"

#include <cctype>
#include <cmath>
#include <cstddef>
#include <format>

#include "css-values.hpp"

namespace neon
{
  // Helpers of UiValue, for this file alone.
  namespace
  {
    /// Letters, digits, underscores, hyphens, and dots, with a letter or an
    /// underscore in front.
    bool IsName(const std::string &text)
    {
      if (text.empty()) { return false; }

      const auto first = static_cast<unsigned char>(text.front());
      if (!std::isalpha(first) && first != '_') { return false; }

      for (const char letter : text)
      {
        const auto each = static_cast<unsigned char>(letter);
        if (!std::isalnum(each) && each != '_' && each != '-' && each != '.') { return false; }
      }
      return true;
    }

    /// The name between the brackets of `{name}` or `{!name}`, when the
    /// text is that and nothing else.
    bool ReadReference(const std::string &text, std::string &name, bool &is_opposite)
    {
      if (text.size() < 3 || text.front() != '{' || text.back() != '}') { return false; }

      std::string inside = text.substr(1, text.size() - 2);
      is_opposite = inside.front() == '!';
      if (is_opposite) { inside = inside.substr(1); }

      if (!IsName(inside)) { return false; }

      name = inside;
      return true;
    }
  }

  UiValue UiValue::Number(const double number)
  {
    UiValue value;
    value.kind = Kind::Number;
    value.number = number;
    return value;
  }

  UiValue UiValue::Text(const std::string &text)
  {
    UiValue value;
    value.kind = Kind::Text;
    value.text = text;
    return value;
  }

  UiValue UiValue::Flag(const bool flag)
  {
    UiValue value;
    value.kind = Kind::Flag;
    value.flag = flag;
    return value;
  }

  std::string UiValue::AsText(const int decimals) const
  {
    switch (kind)
    {
      case Kind::Text: return text;
      case Kind::Flag: return flag ? "true" : "false";
      default: break;
    }

    if (!std::isfinite(number)) { return std::isnan(number) ? "nan" : (number > 0 ? "inf" : "-inf"); }

    if (decimals >= 0) { return std::format("{:.{}f}", number, decimals); }

    // far from zero every number is a whole number, and is written as one
    if (number == std::round(number)) { return std::format("{:.0f}", number); }

    return std::format("{:.2f}", number);
  }

  double UiValue::AsNumber(const double otherwise) const
  {
    switch (kind)
    {
      case Kind::Number: return number;
      case Kind::Flag: return flag ? 1.0 : 0.0;
      default:
      {
        float read = 0.0f;
        return ParseCssNumber(text, read) ? static_cast<double>(read) : otherwise;
      }
    }
  }

  bool UiValue::AsFlag() const
  {
    switch (kind)
    {
      case Kind::Number: return number != 0.0;
      case Kind::Text: return !text.empty();
      default: return flag;
    }
  }

  void UiValues::Set(const std::string &name, const UiValue &value)
  {
    if (const auto found = _values.find(name); found != _values.end() && found->second == value) { return; }

    _values[name] = value;
    _revision++;

    // it may be asked for in vain again, should it ever go missing
    _missed.erase(name);
  }

  void UiValues::SetDefault(const std::string &name, const UiValue &value)
  {
    if (!_values.contains(name)) { Set(name, value); }
  }

  const UiValue *UiValues::Find(const std::string &name) const
  {
    if (const auto found = _values.find(name); found != _values.end()) { return &found->second; }

    // what is asked of the parent in vain is reported by the parent
    if (_parent != nullptr) { return _parent->Find(name); }

    if (_missed.insert(name).second) { _newly_missed.push_back(name); }
    return nullptr;
  }

  std::uint64_t UiValues::GetRevision() const
  {
    return _parent != nullptr ? _revision + _parent->GetRevision() : _revision;
  }

  bool UiValues::Has(const std::string &name) const
  {
    return _values.contains(name) || (_parent != nullptr && _parent->Has(name));
  }

  void UiValues::SetParent(const UiValues *parent)
  {
    _parent = parent != this ? parent : nullptr;
    _revision++;
  }

  std::vector<std::string> UiValues::TakeMissed() const
  {
    std::vector<std::string> missed;
    missed.swap(_newly_missed);
    return missed;
  }

  bool UiTemplate::Parse(const std::string &text, UiTemplate &result, std::string &error)
  {
    std::vector<Part> parts;
    std::string plain;

    const auto end_plain = [&]
    {
      if (!plain.empty()) { parts.push_back({false, plain, -1}); }
      plain.clear();
    };

    for (std::size_t i = 0; i < text.size(); i++)
    {
      const char letter = text[i];
      const bool is_doubled = i + 1 < text.size() && text[i + 1] == letter;

      if ((letter == '{' || letter == '}') && is_doubled)
      {
        plain += letter;
        i++;
        continue;
      }

      if (letter == '}')
      {
        error = "a '}' has no '{' in front of it. Write '}}' for the bracket itself";
        return false;
      }

      if (letter != '{')
      {
        plain += letter;
        continue;
      }

      const std::size_t close = text.find('}', i);
      if (close == std::string::npos)
      {
        error = "a '{' is never closed. Write '{{' for the bracket itself";
        return false;
      }

      std::string name = text.substr(i + 1, close - i - 1);
      int decimals = -1;

      if (const std::size_t colon = name.find(':'); colon != std::string::npos)
      {
        const std::string digits = name.substr(colon + 1);
        name = name.substr(0, colon);

        if (digits.size() != 1 || !std::isdigit(static_cast<unsigned char>(digits.front())))
        {
          error = std::format(
            "'{{{}:{}}}' asks for '{}' digits behind the point, where a number from 0 to 9 was expected",
            name, digits, digits);
          return false;
        }
        decimals = digits.front() - '0';
      }

      if (!IsName(name))
      {
        error = std::format(
          "'{{{}}}' is not the name of a value. A name is made of letters, digits, '_', '-', and '.'",
          name);
        return false;
      }

      end_plain();
      parts.push_back({true, name, decimals});
      i = close;
    }

    end_plain();
    result._parts = parts;
    return true;
  }

  std::string UiTemplate::Format(const UiValues &values) const
  {
    std::string text;

    for (const auto &[is_value, part, decimals] : _parts)
    {
      if (!is_value)
      {
        text += part;
        continue;
      }

      if (const UiValue *value = values.Find(part); value != nullptr)
      {
        text += value->AsText(decimals);
      } else
      {
        text += "{" + part + "}";
      }
    }

    return text;
  }

  bool UiTemplate::HasValues() const
  {
    for (const auto &part : _parts)
    {
      if (part.is_value) { return true; }
    }
    return false;
  }

  UiNumber::UiNumber(const double number)
  {
    _number = number;
  }

  bool UiNumber::Read(const DataValue &value, UiNumber &result)
  {
    if (double number = 0.0; value.GetNumber(number))
    {
      result = UiNumber(number);
      return true;
    }

    std::string text;
    std::string name;
    bool is_opposite = false;

    if (!value.GetText(text) || !ReadReference(text, name, is_opposite) || is_opposite) { return false; }

    result = UiNumber();
    result._name = name;
    return true;
  }

  double UiNumber::Get(const UiValues &values, const double otherwise) const
  {
    if (_name.empty()) { return _number; }

    const UiValue *value = values.Find(_name);
    return value != nullptr ? value->AsNumber(otherwise) : otherwise;
  }

  UiFlag::UiFlag(const bool flag)
  {
    _flag = flag;
  }

  bool UiFlag::Read(const DataValue &value, UiFlag &result)
  {
    if (bool flag = false; value.GetBool(flag))
    {
      result = UiFlag(flag);
      return true;
    }

    std::string text;
    std::string name;
    bool is_opposite = false;

    if (!value.GetText(text) || !ReadReference(text, name, is_opposite)) { return false; }

    result = UiFlag();
    result._name = name;
    result._is_opposite = is_opposite;
    return true;
  }

  bool UiFlag::Get(const UiValues &values, const bool otherwise) const
  {
    if (_name.empty()) { return _flag; }

    const UiValue *value = values.Find(_name);
    if (value == nullptr) { return otherwise; }

    return value->AsFlag() != _is_opposite;
  }
} // neon
