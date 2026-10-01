#include "field-text.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <format>

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

    // Read by hand. The functions of the standard library follow the
    // language of the machine, where the dot may be a comma.
    bool ParseNumber(const std::string &written, float &number)
    {
      const std::string text = Trimmed(written);
      std::size_t position = 0;
      bool negative = false;

      if (position < text.size() && (text[position] == '-' || text[position] == '+'))
      {
        negative = text[position] == '-';
        position++;
      }

      double value = 0.0;
      double scale = 1.0;
      std::size_t digits = 0;
      bool after_point = false;

      for (; position < text.size(); position++)
      {
        const char letter = text[position];

        if (letter == '.' && !after_point)
        {
          after_point = true;
        } else if (letter >= '0' && letter <= '9')
        {
          digits++;
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

      if (digits == 0) { return false; }

      number = static_cast<float>(negative ? -value : value);
      return true;
    }

    std::string FormatNumber(const float number)
    {
      const float rounded = std::round(number * 10000.0f) / 10000.0f;
      if (rounded == 0.0f) { return "0"; }

      return std::format("{}", rounded);
    }

    /// What is set apart by spaces and commas.
    std::vector<std::string> Words(const std::string &text)
    {
      std::vector<std::string> words;
      std::string word;

      for (const char letter : text + " ")
      {
        if (std::isspace(static_cast<unsigned char>(letter)) != 0 || letter == ',')
        {
          if (!word.empty()) { words.push_back(word); }
          word.clear();
        } else
        {
          word += letter;
        }
      }

      return words;
    }

    int HexDigit(const char digit)
    {
      if (digit >= '0' && digit <= '9') { return digit - '0'; }
      if (digit >= 'a' && digit <= 'f') { return digit - 'a' + 10; }
      if (digit >= 'A' && digit <= 'F') { return digit - 'A' + 10; }
      return -1;
    }
  }

  std::string FormatFieldLength(const FieldLength &length)
  {
    if (length.is_auto) { return "auto"; }

    if (length.percent == 0.0f) { return FormatNumber(length.pixels) + "px"; }
    if (length.pixels == 0.0f) { return FormatNumber(length.percent) + "%"; }

    return "calc(" + FormatNumber(length.percent) + "% + " + FormatNumber(length.pixels) + "px)";
  }

  bool ParseFieldLength(const std::string &written, FieldLength &length)
  {
    const std::string text = Lowered(Trimmed(written));
    if (text.empty()) { return false; }

    if (text == "auto")
    {
      length = FieldLength::Auto();
      return true;
    }

    float number = 0.0f;

    if (text.starts_with("calc(") && text.back() == ')')
    {
      const std::string inside = text.substr(5, text.size() - 6);
      const std::size_t plus = inside.find(" + ");
      if (plus == std::string::npos) { return false; }

      const std::string percent = Trimmed(inside.substr(0, plus));
      const std::string pixels = Trimmed(inside.substr(plus + 3));

      float percent_number = 0.0f;
      float pixel_number = 0.0f;

      if (percent.empty() || percent.back() != '%' || !pixels.ends_with("px") ||
          !ParseNumber(percent.substr(0, percent.size() - 1), percent_number) ||
          !ParseNumber(pixels.substr(0, pixels.size() - 2), pixel_number))
      {
        return false;
      }

      length = FieldLength::Sum(pixel_number, percent_number);
      return true;
    }

    if (text.back() == '%')
    {
      if (!ParseNumber(text.substr(0, text.size() - 1), number)) { return false; }

      length = FieldLength::Percent(number);
      return true;
    }

    const std::string digits = text.ends_with("px") ? text.substr(0, text.size() - 2) : text;
    if (digits.empty() || std::isspace(static_cast<unsigned char>(digits.back())) != 0) { return false; }
    if (!ParseNumber(digits, number)) { return false; }

    length = FieldLength::Pixels(number);
    return true;
  }

  std::string FormatFieldColor(const Color &color)
  {
    const auto byte = [](const float part)
    {
      return static_cast<int>(std::lround(std::clamp(part, 0.0f, 1.0f) * 255.0f));
    };

    if (byte(color.a) == 255) { return std::format("#{:02x}{:02x}{:02x}", byte(color.r), byte(color.g), byte(color.b)); }

    return std::format("#{:02x}{:02x}{:02x}{:02x}", byte(color.r), byte(color.g), byte(color.b), byte(color.a));
  }

  bool ParseFieldColor(const std::string &written, Color &color)
  {
    const std::string text = Lowered(Trimmed(written));
    if (text.empty()) { return false; }

    if (text.front() == '#')
    {
      const std::string digits = text.substr(1);
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
        const int value = is_short
          ? HexDigit(digits[i]) * 17
          : HexDigit(digits[i * 2]) * 16 + HexDigit(digits[i * 2 + 1]);

        values[i] = static_cast<float>(value) / 255.0f;
      }

      color = {values[0], values[1], values[2], values[3]};
      return true;
    }

    for (const std::string name : {"rgba(", "rgb("})
    {
      if (!text.starts_with(name) || text.back() != ')') { continue; }

      std::string inside = text.substr(name.size(), text.size() - name.size() - 1);
      std::ranges::replace(inside, '/', ' ');

      const auto parts = Words(inside);
      if (parts.size() != 3 && parts.size() != 4) { return false; }

      float values[4] = {0.0f, 0.0f, 0.0f, 1.0f};

      for (std::size_t i = 0; i < parts.size(); i++)
      {
        std::string part = parts[i];
        const bool is_percent = part.back() == '%';
        if (is_percent) { part.pop_back(); }

        float number = 0.0f;
        if (!ParseNumber(part, number)) { return false; }

        // the colours count to 255, and alpha to 1
        const float whole = is_percent ? 100.0f : (i < 3 ? 255.0f : 1.0f);
        values[i] = std::clamp(number / whole, 0.0f, 1.0f);
      }

      color = {values[0], values[1], values[2], values[3]};
      return true;
    }

    return false;
  }

  std::string FormatField(const FieldValue &value)
  {
    if (const auto *flag = std::get_if<bool>(&value)) { return *flag ? "true" : "false"; }
    if (const auto *whole = std::get_if<int>(&value)) { return std::format("{}", *whole); }
    if (const auto *number = std::get_if<float>(&value)) { return FormatNumber(*number); }
    if (const auto *text = std::get_if<std::string>(&value)) { return *text; }

    if (const auto *vector = std::get_if<glm::vec3>(&value))
    {
      return FormatNumber(vector->x) + " " + FormatNumber(vector->y) + " " + FormatNumber(vector->z);
    }

    if (const auto *color = std::get_if<Color>(&value)) { return FormatFieldColor(*color); }

    if (const auto *texts = std::get_if<std::vector<std::string>>(&value))
    {
      std::string joined;
      for (const auto &text : *texts)
      {
        if (!joined.empty()) { joined += ", "; }
        joined += text;
      }
      return joined;
    }

    if (const auto *length = std::get_if<FieldLength>(&value)) { return FormatFieldLength(*length); }

    if (const auto *numbers = std::get_if<std::vector<float>>(&value))
    {
      std::string joined;
      for (const float number : *numbers)
      {
        if (!joined.empty()) { joined += ' '; }
        joined += FormatNumber(number);
      }
      return joined;
    }

    return "";
  }

  bool ParseField(
    const std::string &text,
    const FieldInfo &field,
    const std::string &what,
    FieldValue &value,
    std::string &error)
  {
    const auto refuse = [&](const std::string &expected)
    {
      error = std::format("{} is '{}', where {} was expected", what, text, expected);
      return false;
    };

    switch (field.kind)
    {
      case FieldKind::Bool:
      {
        const std::string word = Lowered(Trimmed(text));
        if (word != "true" && word != "false") { return refuse("true or false"); }

        value = word == "true";
        return true;
      }

      case FieldKind::Whole:
      {
        float number = 0.0f;
        if (!ParseNumber(text, number) || number != std::round(number) || std::abs(number) > 2.0e9f)
        {
          return refuse("a whole number");
        }

        value = static_cast<int>(number);
        return true;
      }

      case FieldKind::Number:
      {
        float number = 0.0f;
        if (!ParseNumber(text, number)) { return refuse("a number"); }

        value = number;
        return true;
      }

      case FieldKind::Text:
      case FieldKind::Choice:
        // which words a choice has is looked at by Check()
        value = field.kind == FieldKind::Choice ? Trimmed(text) : text;
        return true;

      case FieldKind::Vector:
      {
        const auto words = Words(text);
        glm::vec3 vector{0.0f};

        if (words.size() == 1 && field.one_number_for_all && ParseNumber(words[0], vector.x))
        {
          value = glm::vec3{vector.x};
          return true;
        }

        if (words.size() != 3 || !ParseNumber(words[0], vector.x) || !ParseNumber(words[1], vector.y) ||
            !ParseNumber(words[2], vector.z))
        {
          return refuse("three numbers, such as 1 2 3");
        }

        value = vector;
        return true;
      }

      case FieldKind::Color:
      {
        Color color;
        if (!ParseFieldColor(text, color)) { return refuse("a color such as #ff8000 or rgb(255, 128, 0)"); }

        value = color;
        return true;
      }

      case FieldKind::TextList:
      {
        std::vector<std::string> texts;
        std::string each;

        for (const char letter : text)
        {
          if (letter == ',')
          {
            texts.push_back(Trimmed(each));
            each.clear();
          } else
          {
            each += letter;
          }
        }

        if (!Trimmed(each).empty() || !texts.empty()) { texts.push_back(Trimmed(each)); }

        value = texts;
        return true;
      }

      case FieldKind::Length:
      {
        FieldLength length;
        if (!ParseFieldLength(text, length)) { return refuse("a length such as 12px, 50%, or auto"); }

        value = length;
        return true;
      }

      case FieldKind::NumberList:
      {
        std::vector<float> numbers;

        for (const auto &word : Words(text))
        {
          float number = 0.0f;
          if (!ParseNumber(word, number)) { return refuse("numbers, such as 1 2 3 4"); }

          numbers.push_back(number);
        }

        value = numbers;
        return true;
      }

      case FieldKind::Layers:
      {
        std::vector<float> layers;

        for (const auto &word : Words(text))
        {
          float layer = 0.0f;
          if (!ParseNumber(word, layer)) { return refuse("the numbers of layers, such as 1 3"); }

          layers.push_back(layer);
        }

        // which numbers are layers is looked at by Check()
        value = layers;
        return true;
      }

      case FieldKind::Group:
        error = std::format("{} is a group and holds no value of its own", what);
        return false;
    }

    return false;
  }
} // neon
