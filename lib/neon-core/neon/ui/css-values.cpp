#include "css-values.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>

namespace neon
{
  // Helpers of css-values.cpp, for this file alone.
  namespace
  {
    std::string Trimmed(const std::string &text)
    {
      std::size_t first = 0;
      std::size_t last = text.size();

      while (first < last && std::isspace(static_cast<unsigned char>(text[first]))) { first++; }
      while (last > first && std::isspace(static_cast<unsigned char>(text[last - 1]))) { last--; }

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

    int HexDigit(const char digit)
    {
      if (digit >= '0' && digit <= '9') { return digit - '0'; }
      if (digit >= 'a' && digit <= 'f') { return digit - 'a' + 10; }
      if (digit >= 'A' && digit <= 'F') { return digit - 'A' + 10; }
      return -1;
    }

    bool ParseHexColor(const std::string &digits, Color &color)
    {
      if (digits.size() != 3 && digits.size() != 4 && digits.size() != 6 && digits.size() != 8) { return false; }

      for (const char digit : digits)
      {
        if (HexDigit(digit) < 0) { return false; }
      }

      const bool is_short = digits.size() <= 4;
      const std::size_t channels = is_short ? digits.size() : digits.size() / 2;
      float values[4] = {0.0f, 0.0f, 0.0f, 1.0f};

      for (std::size_t i = 0; i < channels; i++)
      {
        // a single digit stands for the same digit twice: #f80 is #ff8800
        const int value = is_short
          ? HexDigit(digits[i]) * 17
          : HexDigit(digits[i * 2]) * 16 + HexDigit(digits[i * 2 + 1]);

        values[i] = static_cast<float>(value) / 255.0f;
      }

      color = {values[0], values[1], values[2], values[3]};
      return true;
    }

    /// A part of a color: a number up to `whole`, or a percentage of it.
    bool ParseChannel(const std::string &text, const float whole, float &value)
    {
      if (text.empty()) { return false; }

      float number = 0.0f;
      if (text.back() == '%')
      {
        if (!ParseCssNumber(text.substr(0, text.size() - 1), number)) { return false; }
        value = std::clamp(number / 100.0f, 0.0f, 1.0f);
        return true;
      }

      if (!ParseCssNumber(text, number)) { return false; }
      value = std::clamp(number / whole, 0.0f, 1.0f);
      return true;
    }

    bool ParseFunctionColor(const std::string &inside, Color &color)
    {
      // commas, spaces, and a slash in front of alpha all set values apart
      std::string spaced = inside;
      std::ranges::replace(spaced, ',', ' ');
      std::ranges::replace(spaced, '/', ' ');

      const auto parts = SplitCssValues(spaced);
      if (parts.size() != 3 && parts.size() != 4) { return false; }

      Color read;
      if (!ParseChannel(parts[0], 255.0f, read.r) ||
          !ParseChannel(parts[1], 255.0f, read.g) ||
          !ParseChannel(parts[2], 255.0f, read.b))
      {
        return false;
      }

      if (parts.size() == 4 && !ParseChannel(parts[3], 1.0f, read.a)) { return false; }

      color = read;
      return true;
    }
  }

  bool ParseCssNumber(const std::string &text, float &number)
  {
    const std::string trimmed = Trimmed(text);

    // Read by hand. The functions of the standard library either follow the
    // language of the machine, where the dot may be a comma, or are missing
    // from one of the compilers the engine is built with.
    std::size_t position = 0;
    bool negative = false;

    if (position < trimmed.size() && (trimmed[position] == '-' || trimmed[position] == '+'))
    {
      negative = trimmed[position] == '-';
      position++;
    }

    double whole = 0.0;
    double fraction = 0.0;
    double scale = 1.0;
    std::size_t digits = 0;

    for (; position < trimmed.size() && std::isdigit(static_cast<unsigned char>(trimmed[position])); position++)
    {
      whole = whole * 10.0 + (trimmed[position] - '0');
      digits++;
    }

    if (position < trimmed.size() && trimmed[position] == '.')
    {
      position++;
      for (; position < trimmed.size() && std::isdigit(static_cast<unsigned char>(trimmed[position])); position++)
      {
        scale *= 10.0;
        fraction = fraction * 10.0 + (trimmed[position] - '0');
        digits++;
      }
    }

    // the whole text has to be the number, "12abc" is not one
    if (digits == 0 || position != trimmed.size()) { return false; }

    const double read = whole + fraction / scale;
    number = static_cast<float>(negative ? -read : read);
    return true;
  }

  bool ParseCssColor(const std::string &text, Color &color)
  {
    const std::string value = Lowered(Trimmed(text));
    if (value.empty()) { return false; }

    if (value.front() == '#') { return ParseHexColor(value.substr(1), color); }

    if (value == "transparent")
    {
      color = {0.0f, 0.0f, 0.0f, 0.0f};
      return true;
    }

    if (value == "black")
    {
      color = {0.0f, 0.0f, 0.0f, 1.0f};
      return true;
    }

    if (value == "white")
    {
      color = {1.0f, 1.0f, 1.0f, 1.0f};
      return true;
    }

    for (const std::string name : {"rgba(", "rgb("})
    {
      if (value.starts_with(name) && value.back() == ')')
      {
        return ParseFunctionColor(value.substr(name.size(), value.size() - name.size() - 1), color);
      }
    }

    return false;
  }

  bool ParseCssLength(const std::string &text, LayoutLength &length)
  {
    const std::string value = Lowered(Trimmed(text));
    if (value.empty()) { return false; }

    if (value == "auto")
    {
      length = LayoutLength::Auto();
      return true;
    }

    float number = 0.0f;

    // what calc() comes to when it holds a percentage and pixels, as
    // ResolveCssValue() writes it: calc(100% + -20px)
    if (value.starts_with("calc(") && value.back() == ')')
    {
      const std::string inside = value.substr(5, value.size() - 6);
      const std::size_t plus = inside.find(" + ");
      if (plus == std::string::npos) { return false; }

      const std::string percent = Trimmed(inside.substr(0, plus));
      const std::string pixels = Trimmed(inside.substr(plus + 3));

      float percent_number = 0.0f;
      float pixel_number = 0.0f;

      if (percent.empty() || percent.back() != '%' || !pixels.ends_with("px") ||
          !ParseCssNumber(percent.substr(0, percent.size() - 1), percent_number) ||
          !ParseCssNumber(pixels.substr(0, pixels.size() - 2), pixel_number))
      {
        return false;
      }

      length = LayoutLength::Sum(pixel_number, percent_number);
      return true;
    }

    if (value.back() == '%')
    {
      if (!ParseCssNumber(value.substr(0, value.size() - 1), number)) { return false; }
      length = LayoutLength::Percent(number);
      return true;
    }

    const std::string digits = value.ends_with("px") ? value.substr(0, value.size() - 2) : value;
    if (digits.empty() || std::isspace(static_cast<unsigned char>(digits.back()))) { return false; }
    if (!ParseCssNumber(digits, number)) { return false; }

    length = LayoutLength::Pixels(number);
    return true;
  }

  std::vector<std::string> SplitCssValues(const std::string &text)
  {
    std::vector<std::string> values;
    std::string value;
    int depth = 0;

    for (const char letter : text)
    {
      if (letter == '(') { depth++; }
      if (letter == ')') { depth = std::max(0, depth - 1); }

      if (depth == 0 && std::isspace(static_cast<unsigned char>(letter)))
      {
        if (!value.empty()) { values.push_back(value); }
        value.clear();
        continue;
      }

      value += letter;
    }

    if (!value.empty()) { values.push_back(value); }
    return values;
  }
} // neon
